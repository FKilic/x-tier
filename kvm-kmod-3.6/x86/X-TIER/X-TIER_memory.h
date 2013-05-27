/*
 * X-TIER_memory.h
 *
 *  Created on: Jan 27, 2012
 *      Author: Sebastian Vogl <vogls@sec.in.tum.de>
 */

#ifndef XTIER_MEMORY_H_
#define XTIER_MEMORY_H_

#include <linux/kvm_host.h>

/**
 * Establish a virtual memory mapping for a particular process.
 *
 * @param pid The PID of the process.
 * @param cr3 The value of the CR3 register that the process uses.
 * @param size The size of the memory region that should be mapped.
 *
 * @returns The virtual address of the newly created mapping or 0
 *          if the mapping could not be established.
 */
u64 XTIER_memory_establish_mapping(struct kvm_vcpu *vcpu, u64 pid, u64 cr3, u32 size);

/**
 * Remove all virtual memory mappings for a process that have been established
 * using @see XTIER_memory_establish_mapping().
 *
 * @param pid The PID of the process whos mappings will be deleted.
 */
void XTIER_memory_remove_mappings_pid(struct kvm_vcpu *vcpu, u64 pid);

/**
 * Mark the whole X-TIER memory region as not present within the EPTs. All accesses
 * to this memory region will then result in an EPT violation.
 */
void XTIER_memory_remove_access(struct kvm_vcpu *vcpu);

/**
 * Set the whole X-TIER memory region to be present within the EPTs. This function
 * reverts the changes conducted by @see XTIER_memory_remove_access().
 */
void XTIER_memory_reallow_access(struct kvm_vcpu *vcpu);


#endif /* XTIER_MEMORY_H_ */
