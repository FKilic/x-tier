/*
 * X-TIER_event_hanlder.h
 *
 * Contains functions to handle event-based modules that require host scripts
 * to function.
 *
 *  Created on: May 23, 2013
 *      Author: Sebastian Vogl <vogls@sec.in.tum.de>
 */

#ifndef XTIER_EVENT_HANDLER_H_
#define XTIER_EVENT_HANDLER_H_

#include "X-TIER_inject.h"

#include <asm/kvm.h>

/**
 * Check whether a print hypercall contains commands of a specific module.
 */
int XTIER_event_handler_print_dispatch(CPUState *state, struct kvm_regs *vm_regs,
		                               struct XTIER_x86_call_registers *call_regs);



#endif /* XTIER_EVENT_HANDLER_H_ */
