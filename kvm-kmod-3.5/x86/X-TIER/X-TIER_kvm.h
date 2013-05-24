/*
 * X-TIER_kvm.h
 *
 *  Created on: Oct 31, 2011
 *      Author: Sebastian Vogl <vogls@sec.in.tum.de>
 */

#ifndef XTIER_KVM_H_
#define XTIER_KVM_H_

#include <linux/kvm_host.h>
#include <linux/time.h>

#include "../../X-TIER/X-TIER.h"
#include "X-TIER_list.h"

/**
 * Global state.
 */
extern struct XTIER_state _XTIER;

/**
 * Read a 32-bit value from the given vmcs offset.
 *
 * @param offset The offset to read from.
 * @param result A pointer to the variable that will store the result of the read.
 */
void XTIER_read_vmcs(u32 offset, u32 *result);

/**
 * Read a 64-bit value from the given vmcs offset.
 *
 * @param offset The offset to read from.
 * @param result A pointer to the variable that will store the result of the read.
 */
void XTIER_read_vmcs64(u32 offset, u64 *result);

/**
 * Write a 32-bit value to the given vmcs offset.
 *
 * @param offset The offset to write to.
 * @param data The data to be written.
 */
void XTIER_write_vmcs(u32 offset, u32 data);

/**
 * Write a 64-bit value to the given vmcs offset.
 *
 * @param offset The offset to write to.
 * @param data The data to be written.
 */
void XTIER_write_vmcs64(u32 offset, u64 data);

/**
 * Enable HLT Exiting.
 * Every HLT instruction executed within the VM will then lead to a VM Exit.
 * This is used by X-TIER to detect when an injected module has finished its
 * execution.
 */
void XTIER_enable_hlt_exiting(void);

/**
 * Dsiable HLT Exiting.
 * @see XTIER_enable_hlt_exiting()
 */
void XTIER_disable_hlt_exiting(void);

/**
 * Enable Interrupt Exiting.
 * When Interrupt Exiting is enabled, every interrupt within the VM leads to
 * VM Exit.
 */
void XTIER_enable_interrupt_exiting(struct kvm_vcpu *vcpu);

/**
 * Disable Interrupt Exiting.
 * @see XTIER_enable_interrupt_exiting()
 */
void XTIER_disable_interrupt_exiting(struct kvm_vcpu *vcpu);

/**
 * Enqueue an interrupt.
 * This function is used by X-TIER in case Interrupt Exiting @see XTIER_enable_interrupt_exiting()
 * is enabled and an interrupt occurs. X-TIER will save the interrupt and then inject it into the
 * VM once the module execution is finished.
 */
void XTIER_enqueue_intr(u32 entry_intr_info);

/**
 * Handle an EPT violation.
 * X-TIER uses the EPT to protect a module during external function calls.
 */
int XTIER_handle_ept_violation(struct kvm_vcpu *vcpu, u64 gpa);

/**
 * Handle the occurrence of a HLT instruction.
 * A HLT instruction may be an indication that an injected module finished its execution.
 * @see XTIER_enable_hlt_exiting()
 */
int XTIER_handle_hlt(struct kvm_vcpu *vcpu);

/**
 * Handle an exception.
 * This function handles all exceptions that occur in case @see XTIER_enable_interrupt_exiting() is enabled.
 */
int XTIER_handle_exception(struct kvm_vcpu *vcpu, u32 interrupt_info);

/**
 * Notify X-TIER in case the FPU state has changed.
 */
void XTIER_fpu_changed(struct kvm_vcpu *vcpu);

/**
 * Inject the given module into the VM.
 *
 * @param inject The injection structure that describes the module and its arguments.
 */
void XTIER_inject(struct kvm_vcpu *vcpu, struct injection *inject);

/**
 * Enable auto injection for the current module.
 *
 * @param auto_inject The number of times the current module should be automatically injected.
 */
void XTIER_set_auto_inject(struct kvm_vcpu *vcpu, u32 auto_inject);

/**
 * Enable timed injection for the current module.
 *
 * @param time_inject The time in seconds that should pass before the current module is reinjected.
 */
void XTIER_set_time_inject(struct kvm_vcpu *vcpu, u32 time_inject);

/**
 * Check whether the current module should be reinjected.
 */
int XTIER_reinject(struct kvm_vcpu *vcpu);

/**
 * Update the global X-TIER state.
 */
void XTIER_set_global_state(struct kvm_vcpu *vcpu, struct XTIER_state *state);

/**
 * Take the time and store it in the given timespec struct before we enter the VM.
 * Notice that this function can only handle a _SINGLE_ timespec pointer. Subsequent
 * calls to this function will override the previous timespec pointer wihtou a warning.
 *
 * @param ts A pointer to a timespec struct that will be used to store the time when
 *           control is switched to the VM.
 */
void XTIER_take_time_on_entry(struct timespec *ts);

/**
 * This function is called before the VMX entry procedure.
 */
void XTIER_pre_vmx_run(struct kvm_vcpu *vcpu);

/**
 * This function is called shortly before the VM Entry.
 */
void XTIER_vmx_on_entry(struct kvm_vcpu *vcpu);


#endif /* XTIER_KVM_H_ */
