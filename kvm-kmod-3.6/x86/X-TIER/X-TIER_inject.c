/*
 * X-TIER_inject.c
 *
 *  Created on: Mar 26, 2012
 *      Author: Sebastian Vogl <vogls@sec.in.tum.de>
 */

#include "X-TIER_inject.h"
#include "X-TIER_memory.h"
#include "X-TIER_debug.h"
#include "X-TIER_kvm.h"

#include <linux/module.h>

#include <asm/vmx.h>
#include "kvm_cache_regs.h"


/*
 * DEFINES
 */

// We use a dummy PID for all module allocations.
#define XTIER_INJECTION_PID 13333333337

/*
 * The shellcode requires space on the stack to function.
 * This is fixed for now. If you change the size of STACK_AREA_SC
 * you have to modify the shellcode.
 */
#define STACK_AREA_SC (800)
#define STACK_OFFSET (4000 - (STACK_AREA_SC))

// TIME
#define NSEC_PER_SEC 1000000000L
#define NSEC_PER_MSEC 1000000L

/*
 * GLOBALS
 */
// Structure that can be used to save the current VM State such
// that it can be restored after the injection.
struct XTIER_inject_vm_state _XTIER_inject;
EXPORT_SYMBOL_GPL(_XTIER_inject);

// Struct that stores the current XTIER_inject struct
struct injection _XTIER_inject_current_injection;
EXPORT_SYMBOL_GPL(_XTIER_inject_current_injection);

// Performance Measurement
struct XTIER_performance _XTIER_performance;
EXPORT_SYMBOL_GPL(_XTIER_performance);

struct timespec _starttime;
struct timespec _endtime;
struct timespec _hypercall_start;

u32 _injection_faults = 0;

/*
 * FUNCTIONS
 */

/*
 * Initialize the injection structures
 */
void XTIER_inject_init(void)
{
	// Init current injection structure
	_XTIER_inject_current_injection.code = 0;
	_XTIER_inject_current_injection.code_len = 0;
	_XTIER_inject_current_injection.auto_inject = 0;

	// Init XTIER state
	_XTIER_inject.external_function_return_rip = 0;
	_XTIER_inject.qemu_hypercall = 0;
	_XTIER_inject.new_module = 0;
	_XTIER_inject.injection_fault = 0;
}

/*
 * Init/Reset the performance measurement for a new module
 */
void XTIER_inject_init_performance_struct(void)
{
	// All times are stored in ns
	_XTIER_performance.total_module_load_time = 0;
	_XTIER_performance.total_module_exec_time = 0;
	_XTIER_performance.total_module_temp_removal_time = 0;
	_XTIER_performance.total_module_temp_resume_time = 0;
	_XTIER_performance.total_module_unload_time = 0;
	_XTIER_performance.total_module_hypercall_time = 0;
	_XTIER_performance.injections = 0;
	_XTIER_performance.temp_removals = 0;
	_XTIER_performance.hypercalls = 0;
}

/*
 * Begin to measure time for a certain event.
 */
void XTIER_inject_begin_time_measurement(struct timespec *ts)
{
	// Take time
	getnstimeofday(ts);
}

/*
 * End the time measurement of an event that has been started
 * with @see XTIER_inject_begin_time_measurement.
 *
 * Notice this function modifies the global _endtime variable.
 *
 * @param begin A pointer to the timespec that contains the start time
 * @param add_to Add the elapsed time to the given pointer
 */
void XTIER_inject_end_time_measurement(struct timespec *begin,
										 u64 *add_to)
{
	getnstimeofday(&_endtime);
	(*add_to) += (timespec_to_ns(&_endtime) - timespec_to_ns(begin));
}

/*
 * Save the current VM state into the global _XTIER_inject struct.
 */
void saveVMState(struct kvm_vcpu *vcpu)
{
	kvm_arch_vcpu_ioctl_get_regs(vcpu, &(_XTIER_inject.regs));
	kvm_arch_vcpu_ioctl_get_sregs(vcpu, &(_XTIER_inject.sregs));
	_XTIER_inject.external_function_return_rip = 0;
}

/*
 * Restore the VM state from the global _XTIER_inject struct.
 */
void restoreVMState(struct kvm_vcpu *vcpu)
{
	int ret = 0;
	u64 phys_stack = 0;
	struct x86_exception error;
	struct kvm_regs regs;

	// Get actual regiszer state.
	kvm_arch_vcpu_ioctl_get_regs(vcpu, &regs);

	if(_XTIER_inject.event_based && !_XTIER_inject.injection_fault)
	{
		// In case of event-based injection we will not restore RAX
		_XTIER_inject.regs.rax = regs.rax;

		// Set the returning RIP to the saved EIP on the stack
		// Set the returning ESP to the its original value + saved EIP
		phys_stack = vcpu->arch.mmu.gva_to_gpa(vcpu, regs.rsp, 0, &error);

		switch(_XTIER.os)
		{
			case XTIER_OS_UBUNTU_64:
				ret = kvm_read_guest(vcpu->kvm, phys_stack, &_XTIER_inject.regs.rip, 8);
				_XTIER_inject.regs.rsp = regs.rsp + 8;
				break;
			case XTIER_OS_WINDOWS_7_32:
				/* Fall through*/
			case XTIER_OS_UBUNTU_32:
				ret = kvm_write_guest(vcpu->kvm, phys_stack, &_XTIER_inject.regs.rip, 4);
				_XTIER_inject.regs.rsp = regs.rsp + 4;
				break;
			default:
				PRINT_ERROR("OS type is unknown! Cannot restore state!\n");
				return;
		}

		PRINT_DEBUG("EIP will be set to 0x%llx (ESP: 0x%llx, RAX: 0x%llx, FLAGS: 0x%llx)\n", _XTIER_inject.regs.rip,
																				_XTIER_inject.regs.rsp,
																				_XTIER_inject.regs.rax,
																				_XTIER_inject.regs.rflags);
	}

	// Update return value in case userspace wants to use it.
	_XTIER_performance.return_value = regs.rax;

	kvm_arch_vcpu_ioctl_set_regs(vcpu, &(_XTIER_inject.regs));
//	kvm_arch_vcpu_ioctl_set_sregs(vcpu, &(_XTIER_inject.sregs));

	_XTIER_inject.external_function_return_rip = 0;
}

u64 XTIER_inject_reserve_additional_memory(struct kvm_vcpu *vcpu, u32 size)
{
	PRINT_DEBUG("Trying to reserve addition memory of %u bytes!\n", size);
	return XTIER_memory_establish_mapping(vcpu, XTIER_INJECTION_PID, _XTIER_inject.sregs.cr3, size);
}


void XTIER_inject_enable_hook(struct kvm_vcpu *vcpu, struct injection *inject)
{
	unsigned long dr7 = 0;

	PRINT_DEBUG("Setting injection hook on address %p...\n", inject->event_address);

	// Disable - Important to avoid bluescreens
	vcpu->guest_debug &= ~KVM_GUESTDBG_USE_HW_BP;

	// DR0: EXEC
	kvm_set_dr(vcpu, XTIER_INJECT_HOOK_DR, (unsigned long)inject->event_address);

	// Set up DR7
	kvm_get_dr(vcpu, 7, &dr7);
	// Enable DR0-2
	dr7 |= (1UL << (2 * XTIER_INJECT_HOOK_DR + 1));
	// Set LEN to 4-byte and R/W to instruction execution
	dr7 &= 0xfff0ff03;
	// Go
	kvm_set_dr(vcpu, 7, dr7);
	// Reset DR6
	kvm_set_dr(vcpu, 6, 0);

	// Enable
	vcpu->guest_debug |= KVM_GUESTDBG_USE_HW_BP;
}

void XTIER_inject_disable_hook(struct kvm_vcpu *vcpu)
{
	unsigned long dr7 = 0;

	PRINT_DEBUG("Removing injection hook...\n");

	// Disable - Important to avoid bluescreens
	vcpu->guest_debug &= ~KVM_GUESTDBG_USE_HW_BP;

	// Set up DR7
	kvm_get_dr(vcpu, 7, &dr7);
	// Disable DR0-3
	dr7 &= 0xffffff00;
	// Go
	kvm_set_dr(vcpu, 7, dr7);

	// Reset DRs
	kvm_set_dr(vcpu, XTIER_INJECT_HOOK_DR, 0);
	kvm_set_dr(vcpu, 6, 0);

	// Enable
	vcpu->guest_debug |= KVM_GUESTDBG_USE_HW_BP;
}

/**
 * Prepares the stack for a 64-bit guest and places the arguments in the correct
 * register / stack locations.
 *
 * Arguments on 64-bit systems:
 * 		1st ARG: %RDI
 * 		2nd ARG: %RSI
 * 		3rd ARG: %RDX
 * 		4th ARG: %RCX
 * 		5th ARG: %R8
 * 		6th ARG: %R9
 * 		7th ARG - nth ARG: on stack from right to left
 *
 * @param inject The injection structure of the module that is injected.
 * @param virt_stack A pointer to the virtual address of the memory area
 *                   that was reserved for the stack of the module.
 */
void prepareStack64(struct kvm_vcpu *vcpu, struct injection *inject, u64 *virt_stack)
{
	u64 phys_stack = 0;
	struct x86_exception error;
	struct injection_arg *arg = NULL;
	unsigned int i = 0;
	int ret = 0;
	enum kvm_reg reg;

	// Do we actually have arguments?
	if (inject->args)
	{
		// Move all data to the stack that cannot be directly passed as an argument
		// such as strings and structures.
		for (i = 0; i < inject->args->argc; ++i)
		{
			arg = get_next_arg(inject, arg);

			if (!is_immediate(arg))
			{
				// Copy the data to the stack
				PRINT_DEBUG("Writing data of argument %d with type %d and size %d to 0x%llx\n",
							i, arg->type, arg->size, *virt_stack - arg->size);

				// Update address
				(*virt_stack) -= arg->size;
				arg->data_on_stack = (void *)(*virt_stack);

				// Write
				phys_stack = vcpu->arch.mmu.gva_to_gpa(vcpu, (*virt_stack), 0, &error);
				ret = kvm_write_guest(vcpu->kvm, phys_stack, get_arg_data(inject, arg), arg->size);

				if(ret < 0)
				{
					PRINT_ERROR("An error (code: %d) occurred while writing the argument %d to memory!\n",
								ret, i);
					return;
				}
			}
		}

		// Place arguments into the correct register / stack locations
		arg = NULL;
		for (i = inject->args->argc; i > 0 ; --i)
		{
			arg = get_prev_arg(inject, arg);

			if (i >= 7)
			{
				// Arg goes on the stack
				// ToDo: We just fix this to 8 byte here, but the size of the arg
				// may actually be shorter
				(*virt_stack) -= 8;
				phys_stack = vcpu->arch.mmu.gva_to_gpa(vcpu, (*virt_stack), 0, &error);

				if (is_immediate(arg))
				{
					PRINT_DEBUG("Writing argument %d with type %d and size %d to the stack 0x%llx\n",
								 i, arg->type, arg->size, *virt_stack);

					ret = kvm_write_guest(vcpu->kvm, phys_stack, get_arg_data(inject, arg), arg->size);
				}
				else
				{
					PRINT_DEBUG("Writing pointer 0x%lx to argument %d with type %d and size %d to the stack 0x%llx\n",
								(unsigned long)arg->data_on_stack, i, arg->type, arg->size, *virt_stack);
					ret = kvm_write_guest(vcpu->kvm, phys_stack, &arg->data_on_stack, 8);
				}

				if(ret < 0)
				{
					PRINT_ERROR("An error (code: %d) occurred while writing the argument %d "
								"to the stack!\n",
								ret, i);
					return;
				}
			}
			else
			{
				// Arg goes into a register
				switch (i)
				{
					case 1:
						reg = VCPU_REGS_RDI;
						break;
					case 2:
						reg = VCPU_REGS_RSI;
						break;
					case 3:
						reg = VCPU_REGS_RDX;
						break;
					case 4:
						reg = VCPU_REGS_RCX;
						break;
					case 5:
						reg = VCPU_REGS_R8;
						break;
					case 6:
						reg = VCPU_REGS_R9;
						break;
					default:
						PRINT_ERROR("Argument is not between one and six!\n");
				}

				if (is_immediate(arg))
				{
					PRINT_DEBUG("Writing argument %d with value 0x%lx, type %d, and size %d to register %d\n",
								 i, (unsigned long)arg->data, arg->type, arg->size, reg);
					kvm_register_write(vcpu, reg, *((unsigned long *)get_arg_data(inject, arg)));
				}
				else
				{
					PRINT_DEBUG("Writing pointer 0x%lx to argument %d with type %d and size %d to register %d\n",
								(unsigned long)arg->data_on_stack, i, arg->type, arg->size, reg);
					kvm_register_write(vcpu, reg, (unsigned long)arg->data_on_stack);
				}
			}
		}
	}

	// Add Offset to stack so the shellcode can operate
	(*virt_stack) -= STACK_AREA_SC ;

	// Place the original kernel pointer on the stack
	(*virt_stack) -= 8;

	// Write address of the original kernel stack on the new stack
	phys_stack = vcpu->arch.mmu.gva_to_gpa(vcpu, (*virt_stack), 0, &error);

	ret = kvm_write_guest(vcpu->kvm, phys_stack, &_XTIER_inject.regs.rsp, 8);
	kvm_register_write(vcpu, VCPU_REGS_RSP, (*virt_stack));
}

/**
 * Inject the given code into the VM.
 *
 * @param inject A pointer to the XTIER_inject structure used for injection.
 *               It contains a pointer to the code and the length of the code.
 */
void XTIER_inject_code(struct kvm_vcpu *vcpu, struct injection *inject)
{
	u64 virt_code = 0;
	u64 virt_stack = 0;
	u64 phys_code = 0;
	u64 phys_stack = 0;
	struct x86_exception error;

	int ret = 0;

	u32 state = 0;

	PRINT_DEBUG("Injecting code...\n");

	// Get Time
	XTIER_inject_begin_time_measurement(&_starttime);

	if(_XTIER_inject.new_module)
	{
		// This is the first injection, reset the timer
		XTIER_inject_init_performance_struct();

		// Reset
		_XTIER_inject.new_module = 0;
	}

	// Reduce auto injection if enabled
	if(inject->auto_inject > 0)
		inject->auto_inject--;

	// Disable hooks to avoid exceptions during injection
	if(inject->event_based)
		XTIER_inject_disable_hook(vcpu);

	// Reset the fault variable
	_XTIER_inject.injection_fault = 0;

	// Save VM state
	saveVMState(vcpu);

	// Get a mapping
	// We currently can only reserve space for a little less than 4MB.
	if(inject->code_len > (512 * 4096))
	{
		/*
		virt_code = XTIER_memory_establish_mapping(vcpu, XTIER_INJECTION_PID, _XTIER_inject.sregs.cr3, 512 * 4096);
		XTIER_memory_establish_mapping(vcpu, XTIER_INJECTION_PID, _XTIER_inject.sregs.cr3, inject->code_len - (512 * 4096));
		*/
		PRINT_ERROR("The module that should be injected is to large!\n");
		return;
	}

	// Code
	virt_code = XTIER_memory_establish_mapping(vcpu, XTIER_INJECTION_PID, _XTIER_inject.sregs.cr3, inject->code_len);

	// Stack
	// Do NOT modify the stack pointer in case of event based injection
	if(!inject->event_based)
	{
		// Reserve space for the args and the stack itself
		virt_stack = XTIER_memory_establish_mapping(vcpu, XTIER_INJECTION_PID, _XTIER_inject.sregs.cr3,
				                      inject->args_size + 4096);

		if (virt_stack)
		{
			// Currently virt_stack points to the end of the stack
			// Fix that
			virt_stack += inject->args_size + 4000; // We leave 96 bytes free

			// Prepare Stack
			switch(_XTIER.os)
			{
				case XTIER_OS_UBUNTU_64:
					prepareStack64(vcpu, inject, &virt_stack);
					break;
				case XTIER_OS_WINDOWS_7_32:
					/* Fall through*/
				case XTIER_OS_UBUNTU_32:
					virt_stack -= 4;

					// Write address of the original kernel stack on the new stack
					phys_stack = vcpu->arch.mmu.gva_to_gpa(vcpu, virt_stack, 0, &error);

					ret = kvm_write_guest(vcpu->kvm, phys_stack, &_XTIER_inject.regs.rsp, 4);
					kvm_register_write(vcpu, VCPU_REGS_RSP, virt_stack);

					if(inject->args_size > 0)
						PRINT_WARNING("Module arguments for 32-bit OSs are currently not supported!\n");

					break;
				default:
					PRINT_ERROR("OS type is unknown! Cannot inject module!\n");
					XTIER_memory_remove_mappings_pid(vcpu, XTIER_INJECTION_PID);
					return;
			}

		}
	}
	else
	{
		// Event based injection

		// Set the stack to the original stack - The SC offset
		virt_stack = _XTIER_inject.regs.rsp - STACK_AREA_SC;

		// Prepare Stack
		switch(_XTIER.os)
		{
			case XTIER_OS_UBUNTU_64:
				// Place the original kernel pointer on the stack
				virt_stack -= 8;

				// Write address of the original kernel stack on the new stack
				phys_stack = vcpu->arch.mmu.gva_to_gpa(vcpu, virt_stack, 0, &error);

				ret = kvm_write_guest(vcpu->kvm, phys_stack, &_XTIER_inject.regs.rsp, 8);
				kvm_register_write(vcpu, VCPU_REGS_RSP, virt_stack);
				break;
			case XTIER_OS_WINDOWS_7_32:
				/* Fall through*/
			case XTIER_OS_UBUNTU_32:
				virt_stack -= 4;

				// Write address of the original kernel stack on the new stack
				phys_stack = vcpu->arch.mmu.gva_to_gpa(vcpu, virt_stack, 0, &error);

				ret = kvm_write_guest(vcpu->kvm, phys_stack, &_XTIER_inject.regs.rsp, 4);
				kvm_register_write(vcpu, VCPU_REGS_RSP, virt_stack);
				break;
			default:
				PRINT_ERROR("OS type is unknown! Cannot inject module!\n");
				XTIER_memory_remove_mappings_pid(vcpu, XTIER_INJECTION_PID);
				return;
		}

		if(inject->args_size > 0)
			PRINT_WARNING("Module arguments are not supported in the case of event-based injection!\n");
	}

	// Verify that the memory was reserved
	if(virt_code == 0 || virt_stack == 0)
	{
		PRINT_ERROR("Could not establish the mappings for code injection. Aborting.\n");
		return;
	}

	// Get Physical address
	phys_code = vcpu->arch.mmu.gva_to_gpa(vcpu, virt_code, 0, &error);

	// Write Data
	ret = kvm_write_guest(vcpu->kvm, phys_code, inject->code, inject->code_len);

	if(ret < 0)
	{
		PRINT_ERROR("An error (code: %d) occurred while writing the binary to memory!\n", ret);

		// Remove Mappings
		XTIER_memory_remove_mappings_pid(vcpu, XTIER_INJECTION_PID);

		// Reenable hook
		if(inject->event_based)
				XTIER_inject_enable_hook(vcpu, inject);

		return;
	}

	// Set params
	_XTIER_inject.event_based = inject->event_based;
	_XTIER_inject.exit_after_injection = inject->exit_after_injection;

	// Increase injections
	_XTIER_performance.injections++;

	PRINT_INFO("Running shellcode @ 0x%llx (ESP: 0x%llx, KERNEL ESP: 0x%llx, SEIP: 0x%llx, CR3: 0x%llx)\n",
				virt_code, virt_stack, _XTIER_inject.regs.rsp, _XTIER_inject.regs.rip, _XTIER_inject.sregs.cr3);

	// Set Mode
	_XTIER.mode |= XTIER_CODE_INJECTION;

	// Set HALT Exiting
	XTIER_enable_hlt_exiting();

	// Set Exception Exiting
	XTIER_enable_interrupt_exiting(vcpu);

	// Set EIP
	kvm_rip_write(vcpu, virt_code);

	// Flush TLB
	//kvm_x86_ops->tlb_flush(vcpu);

	// Get Time
	XTIER_inject_end_time_measurement(&_starttime, &_XTIER_performance.total_module_load_time);

	// Take Time for execution just before we enter the VM
	XTIER_take_time_on_entry(&_starttime);

	// Make sure the VM is not halted
	XTIER_read_vmcs(GUEST_ACTIVITY_STATE, &state);
	if(state == GUEST_ACTIVITY_HLT)
		XTIER_write_vmcs(GUEST_ACTIVITY_STATE, GUEST_ACTIVITY_ACTIVE);
}

/*
 * Handle a halt instruction that can indicate that the executing code just finished.
 */
int XTIER_inject_handle_hlt(struct kvm_vcpu *vcpu)
{
	// Stop execution time
	XTIER_inject_end_time_measurement(&_starttime, &_XTIER_performance.total_module_exec_time);

	// Get Time
	XTIER_inject_begin_time_measurement(&_starttime);

	PRINT_INFO("Handling HLT Exit...\n");

	//PRINT_DEBUG("RAX: 0x%llx\n", kvm_register_read(vcpu, VCPU_REGS_RAX));

	// Remove Mappings
	XTIER_memory_remove_mappings_pid(vcpu, XTIER_INJECTION_PID);

	// Disable Exit
	XTIER_disable_hlt_exiting();

	// Disable Exception Exiting
	XTIER_disable_interrupt_exiting(vcpu);

	// Restore state
	restoreVMState(vcpu);

	// Reset Reinjection
	_XTIER_inject.reinject = 0;

	// Reset faults
	if(!_XTIER_inject.injection_fault)
		_injection_faults = 0;

	// Set Mode
	if(!_XTIER_inject.event_based)
	{
		_XTIER.mode &= ~((u64)XTIER_CODE_INJECTION);
	}
	else
	{
		// Reenable hook if the was no injection fault
		if(!_XTIER_inject.injection_fault)
			XTIER_inject_enable_hook(vcpu, &_XTIER_inject_current_injection);
	}

	// Get Removal Time
	XTIER_inject_end_time_measurement(&_starttime, &_XTIER_performance.total_module_unload_time);

	// Pause execution ?
	if(_XTIER_inject.exit_after_injection &&
	   !_XTIER_inject_current_injection.auto_inject &&
	   !_XTIER_inject_current_injection.time_inject &&
	   !_XTIER_inject_current_injection.event_based)
	{
		PRINT_DEBUG("Exit after injection is set! Returning to userspace...\n");
		vcpu->run->exit_reason = XTIER_EXIT_REASON_INJECT_FINISHED;
		return 0;
	}
	else
	{
		//vcpu->run->exit_reason = XTIER_EXIT_REASON_INJECT_FINISHED;
		//return 0;
		PRINT_DEBUG("Exit after injection is _NOT_ set! Resuming...\n");
		return 1;
	}
}

/*
 * Take care of an external function call.
 */
int XTIER_inject_temporarily_remove_module(struct kvm_vcpu *vcpu)
{
	int ret = 0;
	u64 phys_stack = 0;

	struct kvm_regs regs;
	struct x86_exception error;

	struct timespec begin;

	PRINT_INFO("The injected module will be temporarily removed due to an external function call!\n");

	// Take time
	// Since the execution is still running we do not use starttime!
	XTIER_inject_begin_time_measurement(&begin);

	// Increase the number of removals
	_XTIER_performance.temp_removals++;

	// Get registers
	kvm_arch_vcpu_ioctl_get_regs(vcpu, &regs);

	// Protect module from read and write access
	XTIER_memory_remove_access(vcpu);

	// Disable Exit
	XTIER_disable_hlt_exiting();

	// Disable Exception Exiting
	XTIER_disable_interrupt_exiting(vcpu);

	//disable_if(vcpu);

	// Save the old RIP such that it points to the next instruction after
	// the interrupt
	_XTIER_inject.external_function_return_rip = kvm_rip_read(vcpu);
	// RBX contains the target instruction address
	PRINT_DEBUG("RET EIP will be set to 0x%llx\n", _XTIER_inject.external_function_return_rip);
	PRINT_DEBUG("CURRENT EIP will be set to 0x%llx\n", regs.rbx);
	PRINT_DEBUG("CR3: 0x%lx\n", kvm_read_cr3(vcpu));
	kvm_rip_write(vcpu, regs.rbx);

	// Push the Return Address on stack
	switch(_XTIER.os)
	{
		case XTIER_OS_UBUNTU_64:
			// Get stack addresses
			PRINT_DEBUG("RSP will be set to 0x%llx\n", regs.rsp - 8);
			phys_stack = vcpu->arch.mmu.gva_to_gpa(vcpu, regs.rsp - 8, 0, &error);
			ret = kvm_write_guest(vcpu->kvm, phys_stack, &_XTIER_inject.external_function_return_rip, 8);
			kvm_register_write(vcpu, VCPU_REGS_RSP, (regs.rsp - 8));
			break;
		case XTIER_OS_WINDOWS_7_32:
			/* Fall through*/
		case XTIER_OS_UBUNTU_32:
			// Get stack addresses
			PRINT_DEBUG("RSP will be set to 0x%llx\n", regs.rsp - 4);
			phys_stack = vcpu->arch.mmu.gva_to_gpa(vcpu, regs.rsp - 4, 0, &error);
			ret = kvm_write_guest(vcpu->kvm, phys_stack, &_XTIER_inject.external_function_return_rip, 4);
			kvm_register_write(vcpu, VCPU_REGS_RSP, (regs.rsp - 4));
			break;
		default:
			PRINT_ERROR("OS type is unknown! Cannot remove module!\n");
			return -1;
	}

	if(ret < 0)
	{
		PRINT_ERROR("An error (code: %d) occurred while pushing the return address!\n", ret);
		PRINT_ERROR("GVA to GPA resolution returned error code %u\n", error.error_code);
		return -1;
	}

	// Take time
	XTIER_inject_end_time_measurement(&begin, &_XTIER_performance.total_module_temp_removal_time);

	return 1;
}

/*
 * Resume execution after an external function call
 */
int XTIER_inject_resume_module_execution(struct kvm_vcpu *vcpu)
{
	struct timespec begin;

	// Did the function return?
	if(kvm_rip_read(vcpu) == _XTIER_inject.external_function_return_rip)
	{
		// Take time
		// Since the execution is still running we do not use starttime!
		XTIER_inject_begin_time_measurement(&begin);

		PRINT_INFO("External function returned. Execution of the injected module will be resumed!\n");
		PRINT_DEBUG("EIP: 0x%lx, RSP: 0x%lx, CR3: 0x%lx\n", kvm_rip_read(vcpu), kvm_register_read(vcpu, VCPU_REGS_RSP), kvm_read_cr3(vcpu));


		// Make the module accessible again
		XTIER_memory_reallow_access(vcpu);

		// Restore RIP
		kvm_rip_write(vcpu, _XTIER_inject.external_function_return_rip);
		_XTIER_inject.external_function_return_rip = 0;

		// Set HALT Exiting
		XTIER_enable_hlt_exiting();

		// Set Exception Exiting
		XTIER_enable_interrupt_exiting(vcpu);

		// Take time
		XTIER_inject_end_time_measurement(&begin, &_XTIER_performance.total_module_temp_resume_time);

		// Return but do not update RIP
		return 2;
	}
	else
	{
		PRINT_WARNING("External function tried to access the protected memory area @ 0x%lx!\n Malware?\n", kvm_rip_read(vcpu));
		return 0;
	}

	return 0;
}

void XTIER_inject_hypercall_begin(struct kvm_vcpu *vcpu)
{
	XTIER_inject_begin_time_measurement(&_hypercall_start);

	// Count hypercalls
	_XTIER_performance.hypercalls++;

	// Hypercall begins
	_XTIER_inject.qemu_hypercall = 1;
}

void XTIER_inject_hypercall_end(struct kvm_vcpu *vcpu)
{
	XTIER_inject_end_time_measurement(&_hypercall_start, &_XTIER_performance.total_module_hypercall_time);

	// Hypercall finished
	_XTIER_inject.qemu_hypercall = 0;
}

void XTIER_inject_fault(struct kvm_vcpu *vcpu)
{
	// Handling Fault
	PRINT_INFO("Injection Fault: An error occurred during the module execution. Retrying...\n");

	// Fault
	_XTIER_inject.injection_fault = 1;

	// Remove the module
	// We set _XTIER_inject.reinject in 'XTIER_inject_handle_hlt' so it must
	// be called beore we manipulate _XTIER_inject.reinject within this funciton.
	XTIER_inject_handle_hlt(vcpu);

	// Increase the fault counter
	if(!_XTIER_inject.event_based)
		_injection_faults++;

	// Decrease injections due to the fault
	// We still keep the time. This means the execution time will still reflect
	// the attempt to inject the module. By decreasing the injection we however
	// increase the average execution time.
	//if(_XTIER_performance.injections > 0)
	//	_XTIER_performance.injections--;

	// Give up if it does not seem to work
	if(_injection_faults >= 10)
	{
		PRINT_ERROR("Injection of the module failed for the %d time! Giving up now...\n", _injection_faults);
		return;
	}

	// Otherwise try to reinject the module on the next entry
	_XTIER_inject.reinject = 1;
}

u8 XTIER_inject_reinject(struct kvm_vcpu *vcpu)
{
	struct timespec now;
	u32 rflags = 0;

	if(!_XTIER_inject_current_injection.code_len || !_XTIER_inject_current_injection.code)
		return 0;

	PRINT_DEBUG_FULL("Checking for reinjection...\n");

	// Do not reinject in case interrupts are disabled, pending or the fpu is active
	XTIER_read_vmcs(GUEST_RFLAGS, &rflags);

	if(!(rflags & (1UL << 9)) || vcpu->fpu_active)
	{
		return 0;
	}

	// Check for a request to reinject the module.
	if(_XTIER_inject.reinject == 1)
	{
		// Inject on the next entry
		PRINT_DEBUG("Will reinject on the next entry!\n");
		_XTIER_inject.reinject = 2;
	}
	else if(_XTIER_inject.reinject == 2)
	{
		// Reset and Reeinject
		PRINT_DEBUG("Will reinject now!\n");
		_XTIER_inject.reinject = 0;
		return 1;
	}

	// Check for auto_injection
	if(!(_XTIER.mode & XTIER_CODE_INJECTION) &&
			!_XTIER_inject_current_injection.event_based &&
		    _XTIER_inject_current_injection.auto_inject > 0)
	{
		return 1;
	}

	// Check for time-based injection
	getnstimeofday(&now);

	if(!(_XTIER.mode & XTIER_CODE_INJECTION) &&
	    !_XTIER_inject_current_injection.event_based &&
		_XTIER_inject_current_injection.time_inject &&
		_XTIER_inject_current_injection.time_inject <=
		((timespec_to_ns(&now) - timespec_to_ns(&_endtime)) / NSEC_PER_SEC))
	{
		return 1;
	}

	return 0;
}

