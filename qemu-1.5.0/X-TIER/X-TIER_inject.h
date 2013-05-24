/*
 * X-TIER_inject.h
 *
 *  Created on: Apr 3, 2012
 *      Author: Sebastian Vogl <vogls@sec.in.tum.de>
 */

#ifndef XTIER_INJECT_H_
#define XTIER_INJECT_H_

#include "X-TIER_external_command.h"
#include "../../X-TIER/X-TIER.h"

#include "qemu-common.h"

/* EVENT BASED INJECTION */
#define XTIER_INJECT_EVENT_MODULE_ACE 0
#define XTIER_INJECT_EVENT_MODULE_VIRUSTOTAL 1

/*
 * STRUCTS
 */
/**
 * Structure to translate function calls.
 */
struct XTIER_x86_call_registers
{
	// By placing the ESP regs on top of the other regs,
	// they will be on the stack first.
	// Thus if an printk requires more function arguments,
	// they will be naturally there where printk will look for them :).
	u64 esp0; // First esp
	u64 esp1;
	u64 esp2;
	u64 esp3;
	u64 esp4; // Last esp

	u64 rdi; // First arg
	u64 rsi;
	u64 rdx;
	u64 rcx;
	u64 r8;
	u64 r9; // Last arg
};

/**
 * Handle a hypercall from an injected module.
 */
void XTIER_inject_handle_interrupt(CPUState *env,
		                           struct XTIER_external_command_redirect *redirect);

#endif /* XTIER_INJECT_H_ */
