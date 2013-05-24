/*
 * X-TIER_kvm.c
 *
 *  Created on: Oct 31, 2011
 *      Author: Sebastian Vogl <vogls@sec.in.tum.de>
 */

#include "X-TIER_kvm.h"
#include "X-TIER_debug.h"
#include "X-TIER_inject.h"

#include <linux/kvm_host.h>
#include <linux/interrupt.h>
#include <linux/module.h>

#include <asm/vmx.h>

#include "kvm_cache_regs.h"
#include "x86.h"
#include "mmu.h"

/*
 * KERNEL / USER SPACE
 */
#define LINUX_PAGE_OFFSET_32 0xc0000000
#define LINUX_PAGE_OFFSET_64 0xffff880000000000
#define WINDOWS_PAGE_OFFSET_32 0x80000000


/*
 * STRUCTURES
 */


/*
 * GLOBALS
 */
u8 _initialized = 0;

struct XTIER_state _XTIER;
EXPORT_SYMBOL_GPL(_XTIER);

// Store the original exception bitmap in case of exception existing.
u32 _exception_bitmap_bkp = 0;
u32 _rflags_bkp = 0;
u32 _idtr_limit_bkp = 0;

// Structures to queue interrupts
struct _queued_interrupt
{
	u32 interrupt_info;
	struct XTIER_list_element list;
};

struct XTIER_list *_queued_interrupts;

struct timespec *_timespec_on_entry = NULL;

// Last EXIT reason
u64 _last_exit_reason = 0;


/*
 * FUNCTIONS
 */

// All init tasks
void _init_XTIER(struct kvm_vcpu *vcpu)
{
	if(_initialized)
		return;
	else
		_initialized = 1;


	// Init state
	_XTIER.mode = 0;
	_XTIER.os = XTIER_OS_UNKNOWN;

	// Init Injection
	XTIER_inject_init();
}


/*
 * Taken from vmx.c
 */
static void __vmcs_writel(unsigned long field, unsigned long value)
{
	u8 error;

	asm volatile (__kvm_handle_fault_on_reboot(ASM_VMX_VMWRITE_RAX_RDX) "; setna %0"
		       : "=q"(error) : "a"(value), "d"(field) : "cc");
}

void XTIER_write_vmcs(u32 offset, u32 data)
{
	PRINT_DEBUG_FULL("Setting VMCS offset 0x%x to 0x%x\n", offset, data);

	__vmcs_writel(offset, data);
}

void XTIER_write_vmcs64(u32 offset, u64 data)
{
	__vmcs_writel(offset, data);
	__vmcs_writel(offset+1, data >> 32);
}

/*
 * Taken from vmx.c
 */
static unsigned long __vmcs_readl(unsigned long field)
{
	unsigned long value = 0;

	asm volatile (__kvm_handle_fault_on_reboot(ASM_VMX_VMREAD_RDX_RAX)
		      : "+a"(value) : "d"(field) : "cc");
	return value;
}


void XTIER_read_vmcs(u32 offset, u32 *result)
{
	(*result) = __vmcs_readl(offset);
}

void XTIER_read_vmcs64(u32 offset, u64 *result)
{
	(*result) = __vmcs_readl(offset) | ((u64)__vmcs_readl(offset+1) << 32);
}


/*
 * This function is used by XTIER_inject. Currently XTIER inject may enable and disable
 * this feature as it sees fit. Keep this in mind if you use this function from anywhere else.
 */
void XTIER_enable_hlt_exiting(void)
{
	u32 cpu_exec_controls;

	PRINT_DEBUG("Enabling HLT exiting...\n");

	// Enable HLT EXITING
	XTIER_read_vmcs(CPU_BASED_VM_EXEC_CONTROL, &cpu_exec_controls);
	cpu_exec_controls |= (CPU_BASED_HLT_EXITING);
	XTIER_write_vmcs(CPU_BASED_VM_EXEC_CONTROL, cpu_exec_controls);

	// SET mode
	_XTIER.mode |= XTIER_HLT_EXIT;
}

/*
 * This function is used by XTIER_inject. Currently XTIER inject may enable and disable
 * this feature as it sees fit. Keep this in mind if you use this function from anywhere else.
 */
void XTIER_disable_hlt_exiting(void)
{
	u32 cpu_exec_controls;

	PRINT_DEBUG("Disabling HLT exiting...\n");

	// Disable HLT EXITING
	XTIER_read_vmcs(CPU_BASED_VM_EXEC_CONTROL, &cpu_exec_controls);
	cpu_exec_controls &= ~(CPU_BASED_HLT_EXITING);
	XTIER_write_vmcs(CPU_BASED_VM_EXEC_CONTROL, cpu_exec_controls);

	// SET mode
	_XTIER.mode &= ~((u64)XTIER_HLT_EXIT);
}

/*
 * Set the exception bitmap mask such that EVERY exception leads to a VM Exit.
 * Also Modifies the IF flag of the EFLAGS register such that the VM ignores external interrupts.
 * In addition the IDTR limit is modified such that all software interrupts will lead to a GP.
 * This function is currently only used by XTIER_inject and may therefore be
 * may be arbitrarily enabled/disabled by injection related functions.
 */
void XTIER_enable_interrupt_exiting(struct kvm_vcpu *vcpu)
{
	PRINT_DEBUG("Enabling Exception Exiting...\n");

	// Store the original value
	XTIER_read_vmcs(EXCEPTION_BITMAP, &_exception_bitmap_bkp);
	// Set for all - except PFs - We do not handle page faults at the moment
	XTIER_write_vmcs(EXCEPTION_BITMAP, 0xffffffff);
	//XTIER_write_vmcs(EXCEPTION_BITMAP, 0xffffffff & ~(1u << PF_VECTOR));

	// Get rflags
	XTIER_read_vmcs(GUEST_RFLAGS, &_rflags_bkp);
	// _rflags_bkp = kvm_x86_ops->get_rflags(vcpu);
	// It seems like we do not need to do this, since we intercept all exceptions and interrupts.
	// Despite this, we still leave it on for now, since it does not hurt either.
	// Remove IF
	XTIER_write_vmcs(GUEST_RFLAGS, (_rflags_bkp & ~(1UL << 9)));
	//kvm_x86_ops->set_rflags(vcpu, _rflags_bkp & ~(1UL << 9));

	// Get IDTR Limit
	XTIER_read_vmcs(GUEST_IDTR_LIMIT, &_idtr_limit_bkp);
	// Set the IDTR Limit to the first 32 entries
	XTIER_write_vmcs(GUEST_IDTR_LIMIT, (32 * 8));

	// SET mode
	_XTIER.mode |= XTIER_EXCEPTION_EXIT;
}

/*
 * Reset the exception bitmap, the IF and the IDTR limit to its original value.
 * This function is currently only used by XTIER_inject and may therefore be
 * may be arbitrarily enabled/disabled by injection related functions.
 */
void XTIER_disable_interrupt_exiting(struct kvm_vcpu *vcpu)
{
	PRINT_DEBUG("Disabling Exception Exiting...\n");

	// Reset
	XTIER_write_vmcs(EXCEPTION_BITMAP, _exception_bitmap_bkp);
	XTIER_write_vmcs(GUEST_RFLAGS, (_rflags_bkp | (1UL << 9)));
	//kvm_x86_ops->set_rflags(vcpu, _rflags_bkp);
	XTIER_write_vmcs(GUEST_IDTR_LIMIT, _idtr_limit_bkp);

	// SET mode
	_XTIER.mode &= ~((u64)XTIER_EXCEPTION_EXIT);
}

/*
 * Enqueue an interrupt that was not delivered while interrupt exiting was enabled.
 * Currently we only save the last interrupt.
 */
void XTIER_enqueue_intr(u32 entry_intr_info)
{
	struct _queued_interrupt *intr;

	PRINT_DEBUG("Enqueuing interrupt: 0x%x\n", entry_intr_info);

	// Does the interrupt exist?
	if (!_queued_interrupts)
		_queued_interrupts = XTIER_list_create();

	// Create a new entry
	intr = kzalloc(sizeof(struct _queued_interrupt), GFP_KERNEL);

	if (!intr)
	{
		PRINT_ERROR("Could not reserve memory to enqueue the interrupt 0x%x!\n", entry_intr_info);
		return;
	}

	// Set Interrupt Info
	intr->interrupt_info = entry_intr_info;

	// Add to list
	XTIER_list_add(_queued_interrupts, &intr->list);
}

EXPORT_SYMBOL_GPL(XTIER_enqueue_intr);

/*
 * Handle a EPT Violation that may for instance be caused by CR3 shadowing or by the return
 * of an external function call in case of module injection.
 *
 * @returns 1 or greater if the Violation could be handled, 0 otherwise.
 *
 * In case of 1 the corresponding vmx function will update the RIP automatically.
 * Otherwise the RIP will remain untouched.
 */
int XTIER_handle_ept_violation(struct kvm_vcpu *vcpu, u64 gpa)
{
	PRINT_DEBUG_FULL("Trying to handle GPA violation @ 0x%llx\n", gpa);

	// Return of an external function call of an injected module?
	if((_XTIER.mode & XTIER_CODE_INJECTION) == XTIER_CODE_INJECTION &&
			kvm_rip_read(vcpu) == _XTIER_inject.external_function_return_rip)
	{
		// Yep.
		return XTIER_inject_resume_module_execution(vcpu);
	}

	return 0;
}

EXPORT_SYMBOL_GPL(XTIER_handle_ept_violation);

/*
 * Handle an Exit due to a halt instruction.
 */
int XTIER_handle_hlt(struct kvm_vcpu *vcpu)
{
	// Check for code injection
	if((_XTIER.mode & XTIER_CODE_INJECTION) == XTIER_CODE_INJECTION)
	{
		return XTIER_inject_handle_hlt(vcpu);
	}

	// Let vmx handle it.
	return -1;
}
EXPORT_SYMBOL_GPL(XTIER_handle_hlt);

/*
 * Handle an exception that lead to a VM Exit due to the Exception Bitmap or due to an interrupt.
 *
 * If the function returns 1, execution will continue.
 * If the function returns 0, execution will be paused.
 * If the function returns -1, the exception should be handled by vmx.c
 */
int XTIER_handle_exception(struct kvm_vcpu *vcpu, u32 interrupt_info)
{
	u64 addr;
	u64 phys_addr;
	u32 instruction_len;
	u32 instruction;
	u8 interrupt_number = 0;

	struct kvm_regs regs;
	struct x86_exception error;

	// Obtain the faulting instruction
	XTIER_read_vmcs64(GUEST_RIP, &addr);
	phys_addr = vcpu->arch.mmu.gva_to_gpa(vcpu, addr, 0, &error);
	kvm_read_guest(vcpu->kvm, phys_addr, &instruction, 4);

	switch((interrupt_info & 0x700))
	{
		case INTR_TYPE_EXT_INTR:
			PRINT_DEBUG("Handling External Interrupt. Faulting Instruction: 0x%x (Interrupt Number: %d)\n",
					instruction, (interrupt_info & 0xff));
			break;
		case INTR_TYPE_NMI_INTR:
			PRINT_DEBUG("Handling NMI.\n");
			break;
		case INTR_TYPE_HARD_EXCEPTION:
			PRINT_DEBUG("Handling Hardware Exception. Faulting Instruction: 0x%x (Exception Vector: %d, RIP: 0x%lx)\n",
					instruction, (interrupt_info & 0xff), kvm_rip_read(vcpu));

			// Device not available should be handled by vmx.c
			if((interrupt_info & 0xff) == 7)
				return -1;

			// We currently have no page fault handler. Future Work.
			if((interrupt_info & 0xff) == 14)
			{
				// We handle this condition so do not inject any interrupts into the guest
				kvm_clear_exception_queue(vcpu);
				kvm_clear_interrupt_queue(vcpu);

				// Notify the XTIER component about the fault
				XTIER_inject_fault(vcpu);

				// Continue
				return 1;
			}

			break;
		case INTR_TYPE_SOFT_EXCEPTION:
			PRINT_DEBUG("Handling Software Exception. Faulting Instruction: 0x%x.\n", instruction);
			break;
		case INTR_TYPE_SOFT_INTR:
			PRINT_DEBUG("Handling Software Interrupt.\n");
			break;
		default:
			PRINT_DEBUG("Handling UNKNOWN Exception Type! (Interrupt Info 0x%x)\n", interrupt_info);
	}

	// Was this an interrupt instruction?
	if((instruction & 0xff) == 0xcd)
	{
		// Obtain interrupt number
		interrupt_number = (u8)((instruction >> 8) & 0xff);

		PRINT_DEBUG("Exit due to software interrupt with number %d\n", interrupt_number);
	}


	// Check for code injection
	// Last Condition disables all commands during an external function call.
	if(_XTIER.mode & XTIER_CODE_INJECTION &&
			interrupt_number == XTIER_HYPERCALL_INTERRUPT &&
			!_XTIER_inject.external_function_return_rip)
	{
		// Update Instruction Pointer to skip the faulting instruction
		XTIER_read_vmcs(VM_EXIT_INSTRUCTION_LEN, &instruction_len);
		kvm_rip_write(vcpu, addr + instruction_len);

		// We handle this condition so do not inject any interrupts into the guest
		kvm_clear_exception_queue(vcpu);
		kvm_clear_interrupt_queue(vcpu);

		// Get registers
		kvm_arch_vcpu_ioctl_get_regs(vcpu, &regs);

		// Is this an external function call?
		if(regs.rax == XTIER_HYPERCALL_EXTERNAL_FUNCTION_CALL)
		{
			// Yep.
			return XTIER_inject_temporarily_remove_module(vcpu);

			// DEBUG
			/*
			vcpu->run->exit_reason = XTIER_EXIT_REASON_DEBUG;
			_last_exit_reason = XTIER_EXIT_REASON_DEBUG;
			XTIER_inject_temporarily_remove_module(vcpu);
			return 0;
			*/
		}

		// Let qemu handle this one
		XTIER_inject_hypercall_begin(vcpu);

		// No external function call. All other cases are handled by the userspace component.
		vcpu->run->exit_reason = XTIER_EXIT_REASON_INJECT_COMMAND;
		_last_exit_reason = XTIER_EXIT_REASON_INJECT_COMMAND;
		return 0;
	}

	// Stop execution.
	/*
	vcpu->run->exit_reason = XTIER_EXIT_REASON_INJECT_FAULT;
	_last_exit_reason = XTIER_EXIT_REASON_INJECT_FAULT;
	return 0;
	*/

	// Continue execution
	return 1;
}
EXPORT_SYMBOL_GPL(XTIER_handle_exception);

void XTIER_fpu_changed(struct kvm_vcpu *vcpu)
{
	if(vcpu->fpu_active)
		PRINT_DEBUG("FPU was activated!\n");
	else
		PRINT_DEBUG("FPU was deactivated!\n");

	if(_XTIER_inject.event_based && !vcpu->fpu_active)
	{
		PRINT_DEBUG("Updating Exception Bitmap since FPU has been deactivated!\n");
		_exception_bitmap_bkp = _exception_bitmap_bkp | (1u << NM_VECTOR);
	}
	else if(_XTIER_inject.event_based && vcpu->fpu_active)
	{
		// PRINT_DEBUG("Updating Exception Bitmap since FPU has been activated!\n");
		//_exception_bitmap_bkp &= ~(1u << NM_VECTOR);
		// XTIER_write_vmcs(EXCEPTION_BITMAP, _exception_bitmap_bkp);
	}

	// Do not inject in case of fpu change
	//_XTIER_inject.exception_exit = 1;
}

EXPORT_SYMBOL_GPL(XTIER_fpu_changed);

void XTIER_inject(struct kvm_vcpu *vcpu, struct injection *inject)
{
	u32 rflags = 0;

	// Init XTIER
	_init_XTIER(vcpu);

	// Is this a request to stop event based execution?
	if(!inject)
	{
		// Yep.
		if(!_XTIER_inject.event_based)
		{
			PRINT_ERROR("Event Injection cannot be disabled since it is not enabled!\n");
			return;
		}

		// Disable
		_XTIER_inject_current_injection.event_based = 0;

		// Disable XTIER mode
		_XTIER.mode &= ~((u64)XTIER_CODE_INJECTION);

		// Disable hook
		XTIER_inject_disable_hook(vcpu);
	}
	else if(inject->event_based)
	{
		// Start event based execution
		_XTIER.mode |= XTIER_CODE_INJECTION;

		// Enable the hook
		XTIER_inject_enable_hook(vcpu, inject);

		// Do not set event_based here! It will be set on the first injection!
	}
	else
	{
		// Nope. Normal code injection.
		XTIER_read_vmcs(GUEST_RFLAGS, &rflags);

		// Check whether the IF flags is cleared
		// 1. If interrupts are disabled the system may be in a critical region
		//    and we postpone the injection.
		// 2. Check if there are pending interrupts. If so do not inject, but handle
		//    the interrupts first.
		// 3. Make sure FPU is currently not active
		if ((rflags & (1UL << 9)) &&
		    (!_queued_interrupts || _queued_interrupts->length == 0) &&
		    !vcpu->fpu_active)
			XTIER_inject_code(vcpu, inject);
		else
		{
			PRINT_DEBUG("Postponing Injection!\n");
			_XTIER_inject.reinject = 2;
		}
	}
}

void XTIER_set_auto_inject(struct kvm_vcpu *vcpu, u32 auto_inject)
{
	PRINT_DEBUG("Setting Auto Injection to %u...\n", auto_inject);

	_init_XTIER(vcpu);

	// Begin a new time measurement
	_XTIER_inject.new_module = 1;

	if(!_XTIER_inject_current_injection.event_based &&
		_XTIER_inject_current_injection.code_len)
		_XTIER_inject_current_injection.auto_inject = auto_inject;
}

void XTIER_set_time_inject(struct kvm_vcpu *vcpu, u32 time_inject)
{
	PRINT_DEBUG("Setting Timed-Injection to %u...\n", time_inject);

	_init_XTIER(vcpu);

	// Begin a new time measurement
	_XTIER_inject.new_module = 1;

	if(!_XTIER_inject_current_injection.event_based &&
		_XTIER_inject_current_injection.code_len)
		_XTIER_inject_current_injection.time_inject = time_inject;
}

// This function is called if an event of event based injection is encountered.
int XTIER_reinject(struct kvm_vcpu *vcpu)
{

	// Is event based injection active?
	if (!_XTIER_inject_current_injection.event_based)
	{
		PRINT_WARNING("XTIER reinject called even so this is no event-based injection!\n");
		return 1;
	}

	// Init XTIER
	_init_XTIER(vcpu);

	// Inject the module
	XTIER_inject_code(vcpu, &_XTIER_inject_current_injection);

	// Remove pending exceptions
	kvm_clear_exception_queue(vcpu);
	kvm_clear_interrupt_queue(vcpu);

	return 1;
}

EXPORT_SYMBOL_GPL(XTIER_reinject);



/*
 * Change the Global XTIER mode
 */
void XTIER_set_global_state(struct kvm_vcpu *vcpu, struct XTIER_state *state)
{
	// Init
	_init_XTIER(vcpu);

	PRINT_DEBUG("Setting global XTIER mode to 0x%llx\n", state->mode);
	_XTIER.mode = state->mode;

	// Set the OS if this has not been done yet
	if(_XTIER.os == XTIER_OS_UNKNOWN || _XTIER.os != state->os)
	{
		PRINT_INFO("The OS was changed to %d!\n", state->os);
		_XTIER.os = state->os;
	}
}

void XTIER_take_time_on_entry(struct timespec *ts)
{
	_timespec_on_entry = ts;
}

/**
 * This function is called immediately before the guest
 * switch is prepared by x86.c.
 */
void XTIER_pre_vmx_run(struct kvm_vcpu *vcpu)
{
	// Reinject a module?
	if((!_queued_interrupts || _queued_interrupts->length == 0) && XTIER_inject_reinject(vcpu))
	{
		if(_XTIER_inject.event_based)
			XTIER_inject_enable_hook(vcpu, &_XTIER_inject_current_injection);
		else
			XTIER_inject(vcpu, &_XTIER_inject_current_injection);
	}
}

/*
 * This function is called directly before VM Entry.
 */
void XTIER_vmx_on_entry(struct kvm_vcpu *vcpu)
{
	u32 rflags = 0;
	struct _queued_interrupt *intr;

	PRINT_DEBUG_FULL("VM Entry!\n");

	if((_XTIER.mode & XTIER_EXCEPTION_EXIT) != XTIER_EXCEPTION_EXIT &&
	   (_XTIER.mode & XTIER_CODE_INJECTION) != XTIER_CODE_INJECTION &&
		_queued_interrupts && _queued_interrupts->length > 0)
	{
		// Get rflags
		XTIER_read_vmcs(GUEST_RFLAGS, &rflags);

		if ((rflags & (1UL << 9)))
		{
			// If this condition is valid exception exiting has just been disabled, and the IF
			// flag is set. We inject the last interrupt in order to resume the machine in case
			// we prohibited a timer related interrupt.

			// Unleash the first queued interrupt
			intr = container_of(_queued_interrupts->first,
					            struct _queued_interrupt,
					            list);

			XTIER_write_vmcs(VM_ENTRY_INTR_INFO_FIELD, intr->interrupt_info);

			// Remove from list
			XTIER_list_remove(_queued_interrupts, &intr->list);

			// Free
			kfree(intr);
		}
	}

	// Has qemu just handled an userspace hypercall?
	if(_XTIER_inject.qemu_hypercall)
	{
		XTIER_inject_hypercall_end(vcpu);
	}

	// Should we take the time on entry?
	if (_timespec_on_entry)
	{
		getnstimeofday(_timespec_on_entry);
		_timespec_on_entry = NULL;
	}

}

EXPORT_SYMBOL_GPL(XTIER_vmx_on_entry);
