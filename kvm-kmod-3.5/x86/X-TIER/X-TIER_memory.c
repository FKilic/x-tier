/*
 * X-TIER_memory.c
 *
 *  Created on: Jan 27, 2012
 *      Author: Sebastian Vogl <vogls@sec.in.tum.de>
 */

#include "X-TIER_memory.h"
#include "X-TIER_debug.h"
#include "X-TIER_list.h"
#include "X-TIER_kvm.h"

#include "kvm_cache_regs.h"
#include "x86.h"
#include "mmu.h"

/*
 * Memory Regions
 *
 * The memory reservation is done in user space. See _XTIER_init in X-TIER/XTIER.c of qemu.
 */

// The number of pages that will be reserved for the page table pool. That is the pages
// that will be used to contain page table structures. Consecutive pages at the beginning
// of the X-TIER memory are will be used for this purpose.
#define XTIER_PAGE_TABLE_POOL 12

/*
 * PAGING MODES
 */
#define PAGING_MODE_UNKNOWN 0
#define PAGING_MODE_32bit 1
#define PAGING_MODE_PAE 2
#define PAGING_MODE_IA32e 3

// The number of entries in the page tables for each paging mode.
#define PT_ENTRIES_PAE 512
#define PT_ENTRIES_32bit 1024
#define PT_ENTRIES_IA32e 512

// The size of the page table entries for each paging mode in bytes.
#define PTE_SIZE_PAE 8
#define PTE_SIZE_I32e 8
#define PTE_SIZE_32bit 4

/*
 * Constants
 */
#define PAGE_TABLE_MASK 0xffffffffff000


/*
 * Structures
 */

/**
 * This structure represents an entry within a page table.
 */
struct pageTableEntry
{
	int index;
	u64 table;
	u64 value;
};

/**
 * A memory mapping that was established in the context of a particular process.
 */
struct mapping
{
	// Navigation
	struct XTIER_list_element list;

	u64 pid;
	u64 cr3;
	u64 virt_address;
	u64 phys_address;
	u32 pages;

	// The top level page Table entry for the current paging mode.
	struct pageTableEntry top_level_page_table;
};

/*
 * Globals
 */
struct XTIER_list *_mappings;

u8 _memory_intialized = 0;
u8 _paging_mode = 0;

// Free pages available for reservation
u32 _free_pages_left = 0;
// The total free pages that are available if no pages have been reserved
u32 _free_pages_total = 0;
// The pages that have been freed, but cannot be reused, since the pages
// before them are still in use
u32 _freed_pages = 0;
// The page table pool
u8 _page_table_pool[XTIER_PAGE_TABLE_POOL];

// The number of page table entries in a page table.
// This depends on the paging mode.
u32 _page_table_entries = 0;
// The size of a page table entry in bytes.
// This depends on the paging mode.
u8 _page_table_entry_size = 0;

/*
 * Functions
 */

/*
 * This function initializes the paging data structures based on the
 * paging mode.
 */
void __init_paging_data(struct kvm_vcpu *vcpu)
{
	struct kvm_sregs sregs;
	u64 cr4,cr0;
	u64 efer;

	// Determine paging mode - TAKEN from nitro
	kvm_arch_vcpu_ioctl_get_sregs(vcpu, &sregs);

	cr0 = sregs.cr0;
	cr4 = sregs.cr4;
	efer = sregs.efer;

	if(!(cr0 & 0x80000000))
	{
		_paging_mode = PAGING_MODE_UNKNOWN;
		return;
	}

	if(cr4 & 0x00000020)
	{
		if(efer & EFER_LME)
		{
			// IA32-E
			_paging_mode =  PAGING_MODE_IA32e;
		}
		else
		{
			// PAE
			_paging_mode = PAGING_MODE_PAE;
		}
	}
	else
	{
		// 32-bit
		_paging_mode = PAGING_MODE_32bit;
	}

	switch(_paging_mode)
	{
		case PAGING_MODE_32bit:
			_page_table_entries = PT_ENTRIES_32bit;
			_page_table_entry_size = PTE_SIZE_32bit;
			break;
		case PAGING_MODE_PAE:
			_page_table_entries = PT_ENTRIES_PAE;
			_page_table_entry_size = PTE_SIZE_PAE;
			break;
		case PAGING_MODE_IA32e:
			_page_table_entries = PT_ENTRIES_IA32e;
			_page_table_entry_size = PTE_SIZE_I32e;
			break;
	}
}

// Init
void __memory_init(struct kvm_vcpu *vcpu)
{
	int i;

	if(_memory_intialized != 0)
		return;

	// We reserve some pages to contain page tables
	_free_pages_total = XTIER_MEMORY_AREA_PAGES - XTIER_PAGE_TABLE_POOL;
	_free_pages_left = _free_pages_total;

	// Init the page table pool
	for(i = 0; i < XTIER_PAGE_TABLE_POOL; i++)
		_page_table_pool[i] = 0;

	// Create mapping list
	_mappings = XTIER_list_create();

	// Determine paging mode
	__init_paging_data(vcpu);

	// Initialisation complete
	_memory_intialized = 1;
}

u64 __getFreePageFromPageTablePool(void)
{
	int i;

	for(i = 0; i < XTIER_PAGE_TABLE_POOL; i++)
	{
		if(_page_table_pool[i] == 0)
		{
			// Assign the free page
			_page_table_pool[i] = 1;

			PRINT_DEBUG_FULL("Claimed page %d from the page table pool.\n", i);

			// Set base
			return (XTIER_MEMORY_AREA_ADDRESS + (XTIER_MEMORY_AREA_PAGE_SIZE * i));
		}
	}

	PRINT_ERROR("No more free pages in the page table pool!\n");

	return 0;
}

void __releasePageFromPageTablePool(u64 phys_address)
{
	int i = (phys_address - XTIER_MEMORY_AREA_ADDRESS) / XTIER_MEMORY_AREA_PAGE_SIZE;

	_page_table_pool[i] = 0;

	PRINT_DEBUG_FULL("Released page %d from the page table pool.\n", i);
}

u32 __freePagesInPageTablePool(void)
{
	int i = 0;
	u32 result = 0;

	for(i = 0; i < XTIER_PAGE_TABLE_POOL; i++)
	{
		if(_page_table_pool[i] == 0)
		{
			result++;
		}
	}

	return result;
}

void __setPageToZero(struct kvm_vcpu *vcpu, u64 phys_address)
{
	char *zero = NULL;

	// Malloc
	zero = kzalloc(XTIER_MEMORY_AREA_PAGE_SIZE, GFP_KERNEL);

	memset(zero, 0, XTIER_MEMORY_AREA_PAGE_SIZE);

	kvm_write_guest(vcpu->kvm, phys_address, zero, XTIER_MEMORY_AREA_PAGE_SIZE);
}

/*
 * Find a entry in the given page table that fulfills the given flags, starting from the first entry.
 */
void __findEntryWithFlags(struct kvm_vcpu *vcpu, struct pageTableEntry *entry, u64 flags)
{
	int i;

	// Continue search from last position
	for(i = entry->index; i < _page_table_entries; i++)
	{
		kvm_read_guest(vcpu->kvm, entry->table + (i * _page_table_entry_size), &(entry->value), _page_table_entry_size);

		if((flags != 0 && (entry->value & flags) == flags) ||
			(flags == 0 && (entry->value & 0x1) == flags))
		{
			PRINT_DEBUG("Entry %d (0x%llx) fulfills flags 0x%llx!\n", i, entry->value, flags);
			entry->index = i;
			return;
		}

		//debug_print("Entry %d (0x%llx) does NOT fulfill flags 0x%llx!\n", i, entry->value, flags);
	}

	entry->index = -1;
}

/*
 * Find a entry in the given page table that fulfills the given flags, starting from the last entry.
 */
void __findEntryWithFlagsReverse(struct kvm_vcpu *vcpu, struct pageTableEntry *entry, u64 flags)
{
	int i;

	// Continue search from last position
	for(i = (_page_table_entries - 1); i >= entry->index; i--)
	{
		kvm_read_guest(vcpu->kvm, entry->table + (i * _page_table_entry_size), &(entry->value), _page_table_entry_size);

		if((flags != 0 && (entry->value & flags) == flags) ||
			(flags == 0 && (entry->value & 0x1) == flags))
		{
			PRINT_DEBUG("Entry %d (0x%llx) fulfills flags 0x%llx!\n", i, entry->value, flags);
			entry->index = i;
			return;
		}

		//debug_print("Entry %d (0x%llx) does NOT fulfill flags 0x%llx!\n", i, entry->value, flags);
	}

	entry->index = -1;
}


/*
 * Checks if the page table is empty.
 */
u8 __pageTableIsEmpty(struct kvm_vcpu *vcpu, u64 phys_address)
{
	struct pageTableEntry e;
	e.table = phys_address;
	e.index = 0;
	e.value = 0;

	__findEntryWithFlags(vcpu, &e, 0x1);

	if (e.index == -1)
		return 1;
	else
		return 0;
}

/*
 * Find a free spot in the given page tables for the given number of 4Kb pages.
 * The flags of the traversed present entries are fixed to 0x7 (+W, +S, and Present).
 * If no such entry can be found, a new entry will be created.
 *
 * @param level The current page table level within the entries array that the function is working on.
 * @param entries An array of page table entries. Each entry reprsents a level within the paging hierarchy.
 * @param flags The flags that the page table entry must fulfill for which we are looking for.
 * @param pages The number of pages that we want to map.
 * @param start_level The level of the page hierarchy on which we start on.
 * @param stop_level The level of the page hierarchy on which we stop. This means if we reach this level we found a mapping.
 * @returns 1 On success, 0 otherwise.
 */
int __findMapping(struct kvm_vcpu *vcpu, int level, struct pageTableEntry *entries[], int flags,
						int pages, u8 start_level, u8 stop_level)
{

	int i = 0;
	int found_pages = 0;
	u64 tmp = 0;

	PRINT_DEBUG("Trying to find a mapping on level %d (table @ 0x%llx)...\n", level, entries[level]->table);

	__findEntryWithFlags(vcpu, entries[level], flags);

	if(entries[level]->index != -1)
	{
		if(level < stop_level)
		{
			if(flags == 0)
			{
				PRINT_DEBUG("Found a free Page Table Entry!\n");
				PRINT_DEBUG("Reserving a page and updating the higher level page table...\n");

				// Get new physical address and set flags
				entries[level]->value = (__getFreePageFromPageTablePool() | 0x67);

				// Write entry
				kvm_write_guest(vcpu->kvm, entries[level]->table + (entries[level]->index * _page_table_entry_size),
								&(entries[level]->value), _page_table_entry_size);

				__setPageToZero(vcpu, (entries[level]->value & PAGE_TABLE_MASK));
			}

			// prepare next level
			entries[level + 1]->index = 0;

			entries[(level + 1)]->table = (entries[level]->value & PAGE_TABLE_MASK);

			PRINT_DEBUG("Established mapping on level %d (0x%llx).\n", level, entries[level]->value);

			if(flags == 0 || level == (stop_level - 1))
				return __findMapping(vcpu, ++level, entries, 0x0, pages, start_level, stop_level);
			else
				return __findMapping(vcpu, ++level, entries, 0x7, pages, start_level, stop_level);
		}
		else
		{
			// Find the desired number of pages
			for(i = 0; entries[level]->index + i < _page_table_entries; i++)
			{
				kvm_read_guest(vcpu->kvm, entries[level]->table + ((entries[level]->index + i) * _page_table_entry_size),
								&(tmp), _page_table_entry_size);

				if((tmp & 0x1) == 0x0)
				{
					found_pages++;

					if(found_pages == pages)
						return 1;
				}
				else
				{
					break;
				}
			}

			return __findMapping(vcpu, --level, entries, 0x7, pages, start_level, stop_level);
		}
	}
	else
	{
		// We did not find a free entry
		if(level == start_level && flags == 0)
			return 0;
		else
		{
			if(flags == 0)
			{
				PRINT_DEBUG("Found no mapping on level %d. Going one level up.\n", level);
				return __findMapping(vcpu, --level, entries, 0x7, pages, start_level, stop_level);
			}
			else
			{
				PRINT_DEBUG("Found no used pte on level %d. Searching now for a free pte.\n", level);
				return __findMapping(vcpu, --level, entries, 0x0, pages, start_level, stop_level);
			}
		}
	}
}

/*
 * Find a free PML4e entry for this mapping.
 * All mapping in the same virtual address space will use the SAME PML4e.
 */
u8 __findFreePML4e_IA32e(struct kvm_vcpu *vcpu, struct mapping *map)
{
	// We need to create the pml4e entry that will be used for ALL mappings.

	// Init structure
	map->top_level_page_table.index = 1;
	map->top_level_page_table.table = map->cr3 & PAGE_TABLE_MASK;
	map->top_level_page_table.value = 0;

	// Lets find a free entry
	__findEntryWithFlags(vcpu, &(map->top_level_page_table), 0);

	if(map->top_level_page_table.index == -1)
	{
		// No free entries
		PRINT_ERROR("There are no free entries in the PML4!\n");
		return 0;
	}

	PRINT_INFO("Found a free PML4 entry (%d).\n", map->top_level_page_table.index);
	PRINT_INFO("This entry will be used for all further page reservations of virtual address space 0x%llx.\n",
				map->cr3);
	return 1;
}

/*
 * Find a free PD entry for this mapping.
 * All mapping in the same virtual address space will use the SAME PDE.
 */
u8 __findFreePDE_PAE(struct kvm_vcpu *vcpu, struct mapping *map)
{
	// We need to create the pml4e entry that will be used for ALL mappings.

	// Init structure
	map->top_level_page_table.index = 0;
	kvm_read_guest(vcpu->kvm, ((map->cr3 & 0xffffffe0) + 24), &(map->top_level_page_table.table), _page_table_entry_size);
	map->top_level_page_table.table &= PAGE_TABLE_MASK;
	map->top_level_page_table.value = 0;

	PRINT_INFO("PDE located @ 0x%llx\n", map->top_level_page_table.table);

	// Lets find a free entry
	__findEntryWithFlags(vcpu, &(map->top_level_page_table), 0);

	if(map->top_level_page_table.index == -1)
	{
		// No free entries
		PRINT_ERROR("There are no free entries in the PD!\n");
		return 0;
	}

	PRINT_INFO("Found a free PD entry (%d).\n", map->top_level_page_table.index);
	PRINT_INFO("This entry will be used for all further page reservations of virtual address space 0x%llx.\n",
				map->cr3);
	return 1;
}

/*
 * Find a free PD entry for this mapping.
 * All mapping in the same virtual address space will use the SAME PDE.
 */
u8 __findFreePDE_32bit(struct kvm_vcpu *vcpu, struct mapping *map)
{
	// We need to create the pml4e entry that will be used for ALL mappings.

	// Init structure
	map->top_level_page_table.index = 0;
	map->top_level_page_table.table = map->cr3 & PAGE_TABLE_MASK;
	map->top_level_page_table.value = 0;

	// Lets find a free entry
	__findEntryWithFlagsReverse(vcpu, &(map->top_level_page_table), 0);

	if(map->top_level_page_table.index == -1)
	{
		// No free entries
		PRINT_ERROR("There are no free entries in the PD!\n");
		return 0;
	}

	PRINT_INFO("Found a free PD entry (%d).\n", map->top_level_page_table.index);
	PRINT_INFO("This entry will be used for all further page reservations of virtual address space 0x%llx.\n",
				map->cr3);
	return 1;
}

/*
 * Establish a mapping. Notice that we currently support only a _single_ VM. SO if there are
 * two or more VMs running in parallel we may run into problems.
 */
u32 __establishMapping_32bit(struct kvm_vcpu *vcpu, struct mapping *map)
{
	int i;
	int ret;
	u32 virt_address;
	u64 tmp = 0;
	struct mapping *mtmp;

	// define structs
	struct pageTableEntry pte;
	struct pageTableEntry *entries[] = {&(map->top_level_page_table), &pte};

	/*
	 * Check if a PAE mapping has already been reserved for the given CR3.
	 * If so, validate that it was not modified. If not, reserve one.
	 */
	mtmp = XTIER_list_get_element(_mappings, struct mapping, list,
									 cr3, map->cr3, NULL);

	if(mtmp != NULL)
	{
		// There is already a mapping for this CR3.

		// Verify that the PDE is still valid.
		kvm_read_guest(vcpu->kvm, mtmp->top_level_page_table.table + (mtmp->top_level_page_table.index * _page_table_entry_size),
						&tmp, _page_table_entry_size);

		if((tmp & PAGE_TABLE_MASK) != (mtmp->top_level_page_table.value & PAGE_TABLE_MASK))
		{
			PRINT_ERROR("Oh oh - Reserved PDE has been modified (0x%llx -> 0x%llx)!\n",
					(tmp & PAGE_TABLE_MASK), (map->top_level_page_table.value & PAGE_TABLE_MASK));
			return 0;
		}
		else
		{
			// Set PDE
			map->top_level_page_table.index = mtmp->top_level_page_table.index;
			map->top_level_page_table.table = mtmp->top_level_page_table.table;
			map->top_level_page_table.value = mtmp->top_level_page_table.value;

			// Establish Mapping
			pte.index = 0;
			pte.value = 0;
			pte.table = (map->top_level_page_table.value & PAGE_TABLE_MASK);

			PRINT_DEBUG("Trying to find a mapping for 0x%llx (CR3: 0x%llx, PT: 0x%llx, FLAGS: 0x7)...\n",
						map->phys_address, map->cr3, pte.table);
			ret = __findMapping(vcpu, 1, entries, 0x0, map->pages, 1, 1);
		}
	}
	else
	{
		// There is no mapping for this CR3 yet.
		// Create one.
		if(__findFreePDE_32bit(vcpu, map))
		{
			// Reserve
			// Get new physical address and set flags
			/*
			 * Flags untested so far.
			 * Currently:
			 */
			map->top_level_page_table.value = (__getFreePageFromPageTablePool() | 0x7);

			// Write entry
			kvm_write_guest(vcpu->kvm, map->top_level_page_table.table + (map->top_level_page_table.index * _page_table_entry_size),
							&(map->top_level_page_table.value), _page_table_entry_size);

			// Clear page
			__setPageToZero(vcpu, (map->top_level_page_table.value & PAGE_TABLE_MASK));

			// Establish Mapping
			pte.index = 0;
			pte.value = 0;
			pte.table = (map->top_level_page_table.value & PAGE_TABLE_MASK);

			PRINT_DEBUG("Trying to find a mapping for 0x%llx (CR3: 0x%llx, PTE: 0x%llx, FLAGS: 0x0)...\n",
						map->phys_address, map->cr3, pte.table);
			ret = __findMapping(vcpu, 1, entries, 0x0, map->pages, 1, 1);
		}
		else
		{
			PRINT_ERROR("Could not find an unused PDE!\n");
			return 0;
		}
	}

	if(ret == 0)
	{
		PRINT_ERROR("Could not establish a mapping!\n");
		return 0;
	}
	else
	{
		PRINT_DEBUG("Found PTE %d (0x%llx)\n", pte.index, pte.value);
	}

	// Set pte
	pte.value = map->phys_address;

	// Flags
	/*
	 * Notice that bit 8 (Global) could be important if we trace more than one process!
	 */
	pte.value |= 0x7;


	for(i = 0; i < map->pages; i++)
	{
		tmp = pte.value + (i * XTIER_MEMORY_AREA_PAGE_SIZE);

		PRINT_DEBUG("Setting PT Entry %d to 0x%llx.\n", pte.index + i, tmp);
		kvm_write_guest(vcpu->kvm, pte.table + ((pte.index + i) * _page_table_entry_size),
						&(tmp), _page_table_entry_size);

		__setPageToZero(vcpu, (tmp & PAGE_TABLE_MASK));

		kvm_read_guest(vcpu->kvm, pte.table + ((pte.index + i) * _page_table_entry_size),
						&tmp, _page_table_entry_size);
		PRINT_DEBUG("PT Entry %d set to 0x%llx.\n", pte.index + i, tmp);
	}


	virt_address = map->top_level_page_table.index;
	virt_address = (virt_address << 10); // The first 10 bits select the pde
	virt_address += pte.index;
	virt_address = (virt_address << 12); // Last 12 bits of the phys address
	virt_address += (map->phys_address & ((1UL << 12) - 1));

	PRINT_DEBUG("Virtual address is 0x%x (pde=%d, pte=%d, phys=0x%llx)\n",
				virt_address, map->top_level_page_table.index, pte.index, map->phys_address);

	return virt_address;
}

/*
 * Establish a mapping. Notice that we currently support only a _single_ VM. SO if there are
 * two or more VMs running in parallel we may run into problems.
 */
u32 __establishMapping_PAE(struct kvm_vcpu *vcpu, struct mapping *map)
{
	int i;
	int ret;
	u32 virt_address;
	u64 tmp;
	struct mapping *mtmp;

	// define structs
	struct pageTableEntry pte;
	struct pageTableEntry *entries[] = {&(map->top_level_page_table), &pte};

	/*
	 * Check if a PAE mapping has already been reserved for the given CR3.
	 * If so, validate that it was not modified. If not, reserve one.
	 */
	mtmp = XTIER_list_get_element(_mappings, struct mapping, list,
									 cr3, map->cr3, NULL);

	if(mtmp != NULL)
	{
		// There is already a mapping for this CR3.

		// Verify that the PDPTE is still valid
		kvm_read_guest(vcpu->kvm, (map->cr3 + 24), &tmp, _page_table_entry_size);

		if((tmp & PAGE_TABLE_MASK) != (mtmp->top_level_page_table.table & PAGE_TABLE_MASK))
		{
			PRINT_ERROR("Oh oh - Reserved PDPTE has been modified (0x%llx -> 0x%llx)!\n",
								(tmp & PAGE_TABLE_MASK), (map->top_level_page_table.table & PAGE_TABLE_MASK));
			return 0;
		}

		// Verify that the PDE is still valid.
		kvm_read_guest(vcpu->kvm, mtmp->top_level_page_table.table + (mtmp->top_level_page_table.index * _page_table_entry_size),
						&tmp, _page_table_entry_size);

		if((tmp & PAGE_TABLE_MASK) != (mtmp->top_level_page_table.value & PAGE_TABLE_MASK))
		{
			PRINT_ERROR("Oh oh - Reserved PDE has been modified (0x%llx -> 0x%llx)!\n",
					(tmp & PAGE_TABLE_MASK), (map->top_level_page_table.value & PAGE_TABLE_MASK));
			return 0;
		}
		else
		{
			// Set PDE
			map->top_level_page_table.index = mtmp->top_level_page_table.index;
			map->top_level_page_table.table = mtmp->top_level_page_table.table;
			map->top_level_page_table.value = mtmp->top_level_page_table.value;

			// Establish Mapping
			pte.index = 0;
			pte.value = 0;
			pte.table = (map->top_level_page_table.value & PAGE_TABLE_MASK);

			PRINT_DEBUG("Trying to find a mapping for 0x%llx (CR3: 0x%llx, PT: 0x%llx, FLAGS: 0x7)...\n",
						map->phys_address, map->cr3, pte.table);
			ret = __findMapping(vcpu, 1, entries, 0x0, map->pages, 1, 1);
		}
	}
	else
	{
		// There is no mapping for this CR3 yet.
		// Create one.
		if(__findFreePDE_PAE(vcpu, map))
		{
			// Reserve
			// Get new physical address and set flags
			/*
			 * Flags untested so far.
			 */
			map->top_level_page_table.value = (__getFreePageFromPageTablePool() | 0x7);

			// Write entry
			kvm_write_guest(vcpu->kvm, map->top_level_page_table.table + (map->top_level_page_table.index * _page_table_entry_size),
							&(map->top_level_page_table.value), _page_table_entry_size);

			// Clear page
			__setPageToZero(vcpu, (map->top_level_page_table.value & PAGE_TABLE_MASK));

			// Establish Mapping
			pte.index = 0;
			pte.value = 0;
			pte.table = (map->top_level_page_table.value & PAGE_TABLE_MASK);

			PRINT_DEBUG("Trying to find a mapping for 0x%llx (CR3: 0x%llx, PTE: 0x%llx, FLAGS: 0x0)...\n",
						map->phys_address, map->cr3, pte.table);
			ret = __findMapping(vcpu, 1, entries, 0x0, map->pages, 1, 1);
		}
		else
		{
			PRINT_ERROR("Could not find an unused PDE!\n");
			return 0;
		}
	}

	if(ret == 0)
	{
		PRINT_ERROR("Could not establish a mapping!\n");
		return 0;
	}
	else
	{
		PRINT_DEBUG("Found PTE %d (0x%llx)\n", pte.index, pte.value);
	}

	// Set pte
	pte.value = map->phys_address;

	// Flags
	/*
	 * Notice that bit 8 (Global) could be important if we trace more than one process!
	 */
	pte.value |= 0x00000007;


	for(i = 0; i < map->pages; i++)
	{
		tmp = pte.value + (i * XTIER_MEMORY_AREA_PAGE_SIZE);

		PRINT_DEBUG("Setting PT Entry %d to 0x%llx.\n", pte.index + i, tmp);
		kvm_write_guest(vcpu->kvm, pte.table + ((pte.index + i) * _page_table_entry_size),
						&(tmp), _page_table_entry_size);

		kvm_read_guest(vcpu->kvm, pte.table + ((pte.index + i) * _page_table_entry_size),
						&tmp, _page_table_entry_size);
		PRINT_DEBUG("PT Entry %d set to 0x%llx.\n", pte.index + i, tmp);
	}

	// The PDPTE is fixed to the last PDPTE, which references kernel space
	virt_address = 3;
	virt_address = (virt_address << 9); // Second 9 bits select the pde
	virt_address += map->top_level_page_table.index;
	virt_address = (virt_address << 9); // Third 9 bits select the pte
	virt_address += pte.index;
	virt_address = (virt_address << 12); // Last 12 bits of the phys address
	virt_address += (map->phys_address & ((1UL << 12) - 1));

	PRINT_DEBUG("Virtual address is 0x%x (pde=%d, pte=%d, phys=0x%llx)\n",
				virt_address, map->top_level_page_table.index, pte.index, map->phys_address);

	return virt_address;
}

/*
 * Establish a mapping. Notice that we currently support only a _single_ VM. SO if there are
 * two or more VMs running in parallel we may run into problems.
 */
u64 __establishMapping_IA32e(struct kvm_vcpu *vcpu, struct mapping *map)
{
	int i;
	int ret;
	u64 virt_address;
	u64 tmp;
	struct mapping *mtmp;

	// define structs
	struct pageTableEntry pdpte;
	struct pageTableEntry pdte;
	struct pageTableEntry pte;
	struct pageTableEntry *entries[] = {&(map->top_level_page_table), &pdpte, &pdte, &pte};

	/*
	 * Check if a PML4e mapping has already been reserved for the given CR3.
	 * If so, validate that it was not modified. If not, reserve one.
	 */
	mtmp = XTIER_list_get_element(_mappings, struct mapping, list,
									 cr3, map->cr3, NULL);

	if(mtmp != NULL)
	{
		// There is already a mapping for this CR3.

		// Verify that the PML4e is still valid.
		kvm_read_guest(vcpu->kvm, mtmp->top_level_page_table.table + (mtmp->top_level_page_table.index * _page_table_entry_size),
						&tmp, _page_table_entry_size);

		if((tmp & PAGE_TABLE_MASK) != (mtmp->top_level_page_table.value & PAGE_TABLE_MASK))
		{
			PRINT_ERROR("Oh oh - Reserved PML4e has been modified (0x%llx -> 0x%llx)!\n",
					(tmp & PAGE_TABLE_MASK), (map->top_level_page_table.value & PAGE_TABLE_MASK));
			return 0;
		}
		else
		{
			// Set PML4e
			map->top_level_page_table.index = mtmp->top_level_page_table.index;
			map->top_level_page_table.table = mtmp->top_level_page_table.table;
			map->top_level_page_table.value = mtmp->top_level_page_table.value;

			// Establish Mapping
			pdpte.index = 0;
			pdpte.value = 0;
			pdpte.table = (map->top_level_page_table.value & PAGE_TABLE_MASK);

			PRINT_DEBUG("Trying to find a mapping for 0x%llx (CR3: 0x%llx, PDPT: 0x%llx, FLAGS: 0x7)...\n",
						map->phys_address, map->cr3, pdpte.table);
			ret = __findMapping(vcpu, 1, entries, 0x7, map->pages, 1, 3);
		}
	}
	else
	{
		// There is no mapping for this CR3 yet.
		// Create one.
		if(__findFreePML4e_IA32e(vcpu, map))
		{
			// Reserve
			// Get new physical address and set flags
			map->top_level_page_table.value = (__getFreePageFromPageTablePool() | 0x67);

			// Write entry
			kvm_write_guest(vcpu->kvm, map->top_level_page_table.table + (map->top_level_page_table.index * _page_table_entry_size),
							&(map->top_level_page_table.value), _page_table_entry_size);

			// Clear page
			__setPageToZero(vcpu, (map->top_level_page_table.value & PAGE_TABLE_MASK));

			// Establish Mapping
			pdpte.index = 0;
			pdpte.value = 0;
			pdpte.table = (map->top_level_page_table.value & PAGE_TABLE_MASK);

			PRINT_DEBUG("Trying to find a mapping for 0x%llx (CR3: 0x%llx, PDPT: 0x%llx, FLAGS: 0x0)...\n",
						map->phys_address, map->cr3, pdpte.table);
			ret = __findMapping(vcpu, 1, entries, 0x0, map->pages, 1, 3);
		}
		else
		{
			PRINT_ERROR("Could not find an unused PML4e!\n");
			return 0;
		}
	}

	if(ret == 0)
	{
		PRINT_ERROR("Could not establish a mapping!\n");
		return 0;
	}
	else
	{
		PRINT_DEBUG("Found PTE %d (0x%llx)\n", pte.index, pte.value);
	}

	// Set pte
	pte.value = map->phys_address;

	// Flags
	pte.value |= 0x0000000000000067;
	//pte.value &= 0x00000FFFFFE00067;


	for(i = 0; i < map->pages; i++)
	{
		tmp = pte.value + (i * XTIER_MEMORY_AREA_PAGE_SIZE);

		PRINT_DEBUG("Setting PT Entry %d to 0x%llx.\n", pte.index + i, tmp);
		kvm_write_guest(vcpu->kvm, pte.table + ((pte.index + i) * _page_table_entry_size),
						&(tmp), _page_table_entry_size);

		kvm_read_guest(vcpu->kvm, pte.table + ((pte.index + i) * _page_table_entry_size),
						&tmp, _page_table_entry_size);
		PRINT_DEBUG("PT Entry %d set to 0x%llx.\n", pte.index + i, tmp);
	}

	virt_address = map->top_level_page_table.index;
	virt_address = (virt_address << 9); // First 9 bits select the pml4e
	virt_address += pdpte.index;
	virt_address = (virt_address << 9); // Second 9 bits select the pdpte
	virt_address += pdte.index;
	virt_address = (virt_address << 9); // Third 9 bits select the pdte
	virt_address += pte.index;
	virt_address = (virt_address << 12); // Last 12 bits of the phys address
	virt_address += (map->phys_address & ((1UL << 12) - 1));

	PRINT_DEBUG("Virtual address is 0x%llx (pml4e=%d, pdpte=%d, pdte=%d, pte=%d, phys=0x%llx)\n",
				virt_address, map->top_level_page_table.index, pdpte.index, pdte.index, pte.index, map->phys_address);

	return virt_address;
}

void __freeUnmappedPages(u32 pages)
{
	// Update freed pages
	_freed_pages += pages;

	// Can we reclaim the memory?
	if(_freed_pages + _free_pages_left == _free_pages_total)
	{
		// All pages are free, so we take the whole range back
		_free_pages_left = _free_pages_total;
		_freed_pages = 0;
	}
}


u64 XTIER_memory_establish_mapping(struct kvm_vcpu *vcpu, u64 pid, u64 cr3, u32 size)
{
	struct mapping *map;
	u32 pages;

	// Init?
	if(_memory_intialized == 0)
		__memory_init(vcpu);

	// Do we have enough free pages left?
	if(size % XTIER_MEMORY_AREA_PAGE_SIZE == 0)
		pages = size / (XTIER_MEMORY_AREA_PAGE_SIZE);
	else
		pages = (size / (XTIER_MEMORY_AREA_PAGE_SIZE)) + 1;

	if(pages > _free_pages_left)
	{
		PRINT_ERROR("There are not enough (%d) free pages left!\n", pages);
		return 0;
	}

	// Create new mapping
	map = kmalloc(sizeof(struct mapping), GFP_KERNEL);
	map->cr3 = cr3;
	map->pid = pid;
	map->pages = pages;
	map->virt_address = 0;
	map->phys_address = XTIER_MEMORY_AREA_ADDRESS + (XTIER_MEMORY_AREA_PAGE_SIZE * (XTIER_MEMORY_AREA_PAGES - _free_pages_left));

	// Get paging mode
	switch(_paging_mode)
	{
		case PAGING_MODE_32bit:
			PRINT_INFO("Paging mode is 32-bit.\n");
			PRINT_INFO("Trying to establish a mapping for PID %lld (CR3: 0x%llx) of size %d (%d pages)\n", pid, cr3, size, pages);

			map->virt_address = __establishMapping_32bit(vcpu, map);
			break;
		case PAGING_MODE_PAE:
			PRINT_INFO("Paging mode is PAE.\n");
			PRINT_INFO("Trying to establish a mapping for PID %lld (CR3: 0x%llx) of size %d\n (%d pages)", pid, cr3, size, pages);

			map->virt_address = __establishMapping_PAE(vcpu, map);
			break;
		case PAGING_MODE_IA32e:
			PRINT_INFO("Paging mode is IA-32e.\n");
			PRINT_INFO("Trying to establish a mapping for PID %lld (CR3: 0x%llx) of size %d (%d pages)\n", pid, cr3, size, pages);

			map->virt_address = __establishMapping_IA32e(vcpu, map);
			break;
		default:
			PRINT_ERROR("Paging not set in guest, aborting mapping!\n");
			return 0;
	}

	// Could we establish a mapping?
	if(map->virt_address != 0)
	{
		// We could establish a mapping.

		// Add to list
		XTIER_list_add(_mappings, &map->list);

		// Update Vars
		_free_pages_left -= map->pages;

		PRINT_INFO("Established a mapping: 0x%llx -> 0x%llx (%d pages)\n", map->virt_address, map->phys_address,
																			map->pages);

		// Status
		PRINT_DEBUG_FULL("Status: Free Pages in Page Pool %d\n", __freePagesInPageTablePool());
		PRINT_DEBUG_FULL("Status: Free Data Pages %d\n", _free_pages_left);

		// Return the virtual address of this mapping
		return map->virt_address;
	}
	else
	{
		// We could NOT establish a mapping
		PRINT_ERROR("Failed to establish a mapping!\n");
		kfree(map);

		return 0;
	}
}

void __removeMapping_32bit(struct kvm_vcpu *vcpu, struct mapping *map)
{
	int i;
	u64 tmp;
	struct mapping *mtmp;

	// define structs
	struct pageTableEntry pte;

	if(map == NULL)
	{
		PRINT_WARNING("The given mapping is NULL!\n");
		return;
	}

	// Get Indices
	tmp = map->virt_address;
	// get rid of the last 12 bits;
	tmp = (tmp >> 12);
	// Get PTE
	pte.index = tmp & ((1UL << 10) - 1);
	PRINT_DEBUG_FULL("PTE Index %d\n", pte.index);

	// Delete Entry
	// PTE
	pte.table = (map->top_level_page_table.value & PAGE_TABLE_MASK);
	PRINT_DEBUG_FULL("PT @ 0x%llx\n", pte.table);
	pte.value = 0;
	pte.value = 0;

	// Set PTEs
	for(i = 0; i < map->pages; i++)
	{
		kvm_write_guest(vcpu->kvm, pte.table + ((pte.index + i) * _page_table_entry_size),
						&(pte.value), _page_table_entry_size);
	}

	// Free physical frames
	__freeUnmappedPages(map->pages);

	// Can we reclaim page pool pages?
	// PDE
	mtmp = XTIER_list_get_element(_mappings, struct mapping, list,
									 cr3, map->cr3, NULL);

	if(mtmp == NULL)
	{
		// No more mappings for this CR3 - Release pdpte.
		PRINT_DEBUG_FULL("No more mappings for CR3 0x%llx!\n", map->cr3);

		// Remove PT
		__releasePageFromPageTablePool(pte.table);

		// Reset PDE
		map->top_level_page_table.value = 0;
		kvm_write_guest(vcpu->kvm, map->top_level_page_table.table + (map->top_level_page_table.index * _page_table_entry_size),
						&(map->top_level_page_table.value), _page_table_entry_size);
	}

	PRINT_DEBUG_FULL("Status: Free Pages in Page Pool %d\n", __freePagesInPageTablePool());
	PRINT_DEBUG_FULL("Status: Free Data Pages %d\n", _free_pages_left);

}

void __removeMapping_PAE(struct kvm_vcpu *vcpu, struct mapping *map)
{
	int i;
	u64 tmp;
	struct mapping *mtmp;

	// define structs
	struct pageTableEntry pte;

	if(map == NULL)
	{
		PRINT_WARNING("The given mapping is NULL!\n");
		return;
	}

	// Get Indices
	tmp = map->virt_address;
	// get rid of the last 12 bits;
	tmp = (tmp >> 12);
	// Get PTE
	pte.index = tmp & ((1UL << 9) - 1);
	PRINT_DEBUG_FULL("PTE Index %d\n", pte.index);

	// Delete Entry
	// PTE
	pte.table = (map->top_level_page_table.value & PAGE_TABLE_MASK);
	PRINT_DEBUG_FULL("PT @ 0x%llx\n", pte.table);
	pte.value = 0;
	pte.value = 0;

	// Set PTEs
	for(i = 0; i < map->pages; i++)
	{
		kvm_write_guest(vcpu->kvm, pte.table + ((pte.index + i) * _page_table_entry_size),
						&(pte.value), _page_table_entry_size);
	}

	// Free physical frames
	__freeUnmappedPages(map->pages);

	// Can we reclaim page pool pages?
	// PDE
	mtmp = XTIER_list_get_element(_mappings, struct mapping, list,
									 cr3, map->cr3, NULL);

	if(mtmp == NULL)
	{
		// No more mappings for this CR3 - Release pdpte.
		PRINT_DEBUG_FULL("No more mappings for CR3 0x%llx!\n", map->cr3);

		// Remove PT
		__releasePageFromPageTablePool(pte.table);

		// Reset PDE
		map->top_level_page_table.value = 0;
		kvm_write_guest(vcpu->kvm, map->top_level_page_table.table + (map->top_level_page_table.index * _page_table_entry_size),
						&(map->top_level_page_table.value), _page_table_entry_size);
	}

	PRINT_DEBUG_FULL("Status: Free Pages in Page Pool %d\n", __freePagesInPageTablePool());
	PRINT_DEBUG_FULL("Status: Free Data Pages %d\n", _free_pages_left);

}

void __removeMapping_IA32e(struct kvm_vcpu *vcpu, struct mapping *map)
{
	int i;
	u64 tmp;
	struct mapping *mtmp;

	// define structs
	struct pageTableEntry pdpte;
	struct pageTableEntry pdte;
	struct pageTableEntry pte;

	if(map == NULL)
	{
		PRINT_WARNING("The given mapping is NULL!\n");
		return;
	}

	// Get Indices
	tmp = map->virt_address;
	// get rid of the last 12 bits;
	tmp = (tmp >> 12);
	// Get PTE
	pte.index = tmp & ((1UL << 9) - 1);
	PRINT_DEBUG_FULL("PTE Index %d\n", pte.index);
	tmp = (tmp >> 9);
	// Get PDTE
	pdte.index = tmp & ((1UL << 9) - 1);
	PRINT_DEBUG_FULL("PDTE Index %d\n", pdte.index);
	tmp = (tmp >> 9);
	// Get PDPTE
	pdpte.index = tmp & ((1UL << 9) - 1);
	PRINT_DEBUG_FULL("PDPTE Index %d\n", pdpte.index);
	tmp = (tmp >> 9);

	// Delete Entry
	// PDPTE
	pdpte.table = (map->top_level_page_table.value & PAGE_TABLE_MASK);
	PRINT_DEBUG_FULL("PDPT @ 0x%llx\n", pdpte.table);
	// PDTE
	kvm_read_guest(vcpu->kvm, pdpte.table + (pdpte.index * _page_table_entry_size),
					&(pdpte.value), _page_table_entry_size);
	pdte.table = (pdpte.value & PAGE_TABLE_MASK);
	PRINT_DEBUG_FULL("PDT @ 0x%llx\n", pdte.table);
	pdpte.value = 0;
	// PTE
	kvm_read_guest(vcpu->kvm, pdte.table + (pdte.index * _page_table_entry_size),
					&(pdte.value), _page_table_entry_size);
	pte.table = (pdte.value & PAGE_TABLE_MASK);
	PRINT_DEBUG_FULL("PT @ 0x%llx\n", pte.table);
	pdte.value = 0;
	pte.value = 0;

	// Set PTEs
	for(i = 0; i < map->pages; i++)
	{
		kvm_write_guest(vcpu->kvm, pte.table + ((pte.index + i) * _page_table_entry_size),
						&(pte.value), _page_table_entry_size);
	}

	// Free physical frames
	__freeUnmappedPages(map->pages);

	// Can we reclaim page pool pages?
	// PTE
	if(__pageTableIsEmpty(vcpu, pte.table))
	{
		PRINT_DEBUG_FULL("PT (0x%llx) is empty!\n", pte.table);

		__releasePageFromPageTablePool(pte.table);
		// Set PDTE
		kvm_write_guest(vcpu->kvm, pdte.table + (pdte.index * _page_table_entry_size),
						&(pdte.value), _page_table_entry_size);
	}

	// PDTE
	if(__pageTableIsEmpty(vcpu, pdte.table))
	{
		PRINT_DEBUG_FULL("PDT (0x%llx) is empty!\n", pdte.table);

		__releasePageFromPageTablePool(pdte.table);
		// Set PDPTE
		kvm_write_guest(vcpu->kvm, pdpte.table + (pdpte.index * _page_table_entry_size),
						&(pdpte.value), _page_table_entry_size);
	}

	// PDPTE
	mtmp = XTIER_list_get_element(_mappings, struct mapping, list,
									 cr3, map->cr3, NULL);

	if(mtmp == NULL)
	{
		// No more mappings for this CR3 - Release pdpte.
		PRINT_DEBUG_FULL("No more mappings for CR3 0x%llx!\n", map->cr3);

		map->top_level_page_table.value = 0;
		kvm_write_guest(vcpu->kvm, map->top_level_page_table.table + (map->top_level_page_table.index * _page_table_entry_size),
						&(map->top_level_page_table.value), _page_table_entry_size);

		__releasePageFromPageTablePool(pdpte.table);

	}

	PRINT_DEBUG_FULL("Status: Free Pages in Page Pool %d\n", __freePagesInPageTablePool());
	PRINT_DEBUG_FULL("Status: Free Data Pages %d\n", _free_pages_left);

}

void XTIER_memory_remove_mappings_pid(struct kvm_vcpu *vcpu, u64 pid)
{
	struct mapping *map;
	struct XTIER_list_element *e;

	// Was the list initialized?
	if(_memory_intialized == 0)
	{
		PRINT_ERROR("Memory has not been initialized yet!. There have been no mappings so far.\n");
		return;
	}

	// Handle mappings
	XTIER_list_for_each(_mappings, e)
	{
		map = container_of(e, struct mapping, list);

		if(map != NULL && map->pid == pid)
		{
			PRINT_DEBUG("Deleting mapping 0x%llx -> 0x%llx\n", map->virt_address, map->phys_address);
			XTIER_list_remove(_mappings, &(map->list));

			switch(_paging_mode)
			{
				case PAGING_MODE_32bit:
					__removeMapping_32bit(vcpu, map);
					break;
				case PAGING_MODE_PAE:
					__removeMapping_PAE(vcpu, map);
					break;
				case PAGING_MODE_IA32e:
					__removeMapping_IA32e(vcpu, map);
					break;
				default:
					PRINT_ERROR("Paging not set in guest, aborting!\n");
					return;
			}

			// ToDo: Remove the mapping from the list
			//kfree(map);
		}
	}
}

/*
 * Mark the complete physical memory region as unaccessible in the EPT. Every guest access will then
 * lead to an EPT violation.
 */
void XTIER_memory_remove_access(struct kvm_vcpu *vcpu)
{
	u64 table;
	u64 entry_adr;
	u64 entry_value;
	u64 *entry_p;
	int nr_sptes;
	u64 sptes[4];
	int i;

	// Get the EPT mapping for our memory region
	nr_sptes = kvm_mmu_get_spte_hierarchy(vcpu, XTIER_MEMORY_AREA_ADDRESS, sptes);

	// There must at least be two entries one for the table and one for the entry.
	if(nr_sptes < 2)
	{
		PRINT_ERROR("The EPT mapping only contains a single entry for address 0x%llx\n", XTIER_MEMORY_AREA_ADDRESS);
		return;
	}

	// The Mapping is reverse. So the lowest number is the last entry
	// Select the last table.
	table = sptes[2] & ~(0xfff);

	// Lets find the address of the entry
	// An EPT table has 512 entries
	for(i = 0; i < 512; i++)
	{
		// Virtual address of the current entry
		entry_adr = (u64)__va(table + (i*8));
		// Pointer points to the current entry
		entry_p = (u64 *)entry_adr;
		// Read the data and store it.
		entry_value = (*entry_p);

		// Match for the last entry?
		if(entry_value == sptes[1])
		{
			PRINT_DEBUG("FOUND EPT entry 0x%llx @ 0x%llx\n", entry_value, entry_adr);
			// Remove rwx
			(*entry_p) = (entry_value & ~(0x7));

			break;
		}
	}

	// Flush cache
	kvm_mmu_invlpg(vcpu, XTIER_MEMORY_AREA_ADDRESS);
}

/*
 * Reallow the access to our physical memory region that was removed with the @see XTIER_memory_remove_access function.
 */
void XTIER_memory_reallow_access(struct kvm_vcpu *vcpu)
{
	u64 table;
	u64 entry_adr;
	u64 entry_value;
	u64 *entry_p;
	int nr_sptes;
	u64 sptes[4];
	int i;

	// Get the EPT mapping for our memory region
	nr_sptes = kvm_mmu_get_spte_hierarchy(vcpu, XTIER_MEMORY_AREA_ADDRESS, sptes);

	// There must at least be two entries one for the table and one for the entry.
	if(nr_sptes < 2)
	{
		PRINT_ERROR("The EPT mapping only contains a single entry for address 0x%llx\n", XTIER_MEMORY_AREA_ADDRESS);
		return;
	}

	// The Mapping is reverse. So the lowest number is the last entry
	// Select the last table.
	table = sptes[2] & ~(0xfff);

	// Lets find the address of the entry
	// An EPT table has 512 entries
	for(i = 0; i < 512; i++)
	{
		// Virtual address of the current entry
		entry_adr = (u64)__va(table + (i*8));
		// Pointer points to the current entry
		entry_p = (u64 *)entry_adr;
		// Read the data and store it.
		entry_value = (*entry_p);

		// Match for the last entry?
		if(entry_value == sptes[1])
		{
			PRINT_DEBUG("FOUND EPT entry 0x%llx @ 0x%llx\n", entry_value, entry_adr);
			// Remove rwx
			(*entry_p) = (entry_value | 0x7);
			break;
		}
	}

	// Flush cache
	kvm_mmu_invlpg(vcpu, XTIER_MEMORY_AREA_ADDRESS);
}

