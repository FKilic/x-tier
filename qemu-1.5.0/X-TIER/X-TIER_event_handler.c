/*
 * X-TIER_event_handler.c
 *
 *  Created on: May 23, 2013
 *      Author: Sebastian Vogl <vogls@sec.in.tum.de>
 */

#include "X-TIER_event_handler.h"
#include "X-TIER_qemu.h"
#include "X-TIER_debug.h"

#include "../include/sysemu/kvm.h"

u32 _start_pos = 0;
u32 _current_pos = 0;
u32 _benign_hash_cache[80000];

/*
static int XTIER_inject_virustotal(CPUX86State *env, u32 a, u32 b, u32 c, u32 d, u32 e)
{
	char cmd[2048];

	sprintf(cmd, "/tmp/scripts/virustotal/virustotal.py %08x%08x%08x%08x%08x",
			a, b, c, d, e);

	return WEXITSTATUS(system(cmd));
}
*/

static int XTIER_event_handler_clamav_simple(CPUState *state, u32 a, u32 b, u32 c, u32 d, u32 e)
{
	u32 i = 0;
	char cmd[2048];
	int result = 0;

	// Try to find in cache of benign binaries
	for(i = _start_pos; i != _current_pos; i += 5)
	{
		// Fix if necessary
		if(i == 80000)
			i = 0;

		if(_benign_hash_cache[i] == a && _benign_hash_cache[i+1] == b &&
				_benign_hash_cache[i+2] == c && _benign_hash_cache[i+3] == d &&
				_benign_hash_cache[i+4] == e)
		{
			PRINT_DEBUG("Found hash %08x%08x%08x%08x%08x in cache!\n", a, b, c, d, e);
			return 0;
		}
	}

	// Else call script
	sprintf(cmd, "/tmp/scripts/clamav/clamav_simple.sh %08x%08x%08x%08x%08x",
			a, b, c, d, e);

	result = WEXITSTATUS(system(cmd));

	// Cache if benign
	if(result == 0)
	{
		if(_current_pos + 5 >= 80000)
		{
			_current_pos = 0;
			PRINT_DEBUG("Hash cache full starting over!\n");
		}

		// Insert
		_benign_hash_cache[_current_pos] = a;
		_benign_hash_cache[_current_pos + 1] = b;
		_benign_hash_cache[_current_pos + 2] = c;
		_benign_hash_cache[_current_pos + 3] = d;
		_benign_hash_cache[_current_pos + 4] = e;

		// Update
		if(_current_pos < _start_pos)
		{
			_start_pos += 5;

			if(_start_pos == 80000)
				_start_pos = 0;
		}

		_current_pos += 5;
	}

	return result;
}

int XTIER_event_handler_print_dispatch(CPUState *state, struct kvm_regs *vm_regs,
		                               struct XTIER_x86_call_registers *call_regs)
{
	char *msg = (char *)call_regs->rdi;

	if(!msg)
		return 0;

	if(msg[0] == 'S' && msg[1] == 'H' && msg[2] == 'A')
	{
		// This is a virus total command
		/*
		vm_regs->rax = XTIER_event_handler_virustotal(state, (u32)call_regs->rsi,
													  (u32)call_regs->rdx,
													  (u32)call_regs->rcx,
													  (u32)call_regs->r8,
													  (u32)call_regs->r9);

		PRINT_DEBUG("Virustotal script returned %lld\n", vm_regs->rax);
		*/

		// This is a clamav command
		vm_regs->rax = XTIER_event_handler_clamav_simple(state, (u32)call_regs->rsi,
															  (u32)call_regs->rdx,
															  (u32)call_regs->rcx,
															  (u32)call_regs->r8,
															  (u32)call_regs->r9);

		PRINT_DEBUG("ClamAV script returned %lld\n", vm_regs->rax);

		// Set the registers
		XTIER_ioctl(KVM_SET_REGS, vm_regs);
		// Set the ENV register such that the run loop will not overwrite
		// the new EAX value!
		((CPUX86State *)state)->regs[R_EAX] = vm_regs->rax;

		return 1;
	}

	return 0;
}


