/*
 * X-TIER_inject.h
 *
 *  Created on: Mar 26, 2012
 *      Author: Sebastian Vogl <vogls@sec.in.tum.de>
 */

#ifndef XTIER_INJECT_H_
#define XTIER_INJECT_H_

#include "X-TIER_kvm.h"

#include <linux/kvm_host.h>

struct XTIER_inject_vm_state
{
	struct kvm_regs regs;
	struct kvm_sregs sregs;
	// The RIP that will be returned to after an external function call
	u64 external_function_return_rip;
	// Is this an event based injection?
	u8 event_based;
	// Exit after the injection is done?
	u8 exit_after_injection;
	// Was the injected module changed?
	u8 new_module;
	// Does qemu currently handle a hypercall
	u8 qemu_hypercall;
	// Was there a fault during last injection?
	u8 injection_fault;
	// Should the module be reinjected?
	u8 reinject;
	// Was the last exit due to an exception?
	u8 exception_exit;
};

/*
 * Global structures.
 */
extern struct XTIER_inject_vm_state _XTIER_inject;
extern struct injection _XTIER_inject_current_injection;
extern struct XTIER_performance _XTIER_performance;

/**
 * Initialize the injection structures.
 * This function must be called before code can be injected.
 */
void XTIER_inject_init(void);

/**
 * Inject code into a VM.
 *
 * @param inject The injection structure that contains the code as well as the arguments.
 */
void XTIER_inject_code(struct kvm_vcpu *vcpu, struct injection *inject);

/**
 * Handle a HLT instruction, which may indicate that a module finished its execution.
 */
int XTIER_inject_handle_hlt(struct kvm_vcpu *vcpu);

/**
 * Reserve additional memory for the injected module.
 */
u64 XTIER_inject_reserve_additional_memory(struct kvm_vcpu *vcpu, u32 size);

/**
 * Temporarily remove a module from the guest to execute an external function call.
 */
int XTIER_inject_temporarily_remove_module(struct kvm_vcpu *vcpu);

/**
 * Resume the modules execution after an external function call.
 */
int XTIER_inject_resume_module_execution(struct kvm_vcpu *vcpu);

/**
 * Set a code hook for event based injection.
 *
 * @param inject A pointer to the injection structure that contains the address of the hook.
 */
void XTIER_inject_enable_hook(struct kvm_vcpu *vcpu, struct injection *inject);

/*
 * Disable the current code hook.
 * @see  XTIER_enable_injection_hook()
 */
void XTIER_inject_disable_hook(struct kvm_vcpu *vcpu);

/**
 * This function is called in case a hypercall is executed by an injected module.
 */
void XTIER_inject_hypercall_begin(struct kvm_vcpu *vcpu);

/**
 * This function is called after a hypercall was completed.
 */
void XTIER_inject_hypercall_end(struct kvm_vcpu *vcpu);

/**
 * Check whether the current module should be reinjected.
 */
u8 XTIER_inject_reinject(struct kvm_vcpu *vcpu);

/**
 * Handle an execption that cannot be enqueued and injected later on. E.g. a page fault.
 */
void XTIER_inject_fault(struct kvm_vcpu *vcpu);

#endif /* XTIER_INJECT_H_ */
