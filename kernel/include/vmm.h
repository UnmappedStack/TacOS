#pragma once
#include <stdint.h>
#include <vmem.h>
#include <rbtree.h>

#define VM_ALLOC_NUM_ORDERS 64

typedef struct {
    LList list;
    uintptr_t paddr;
    size_t num_pages;
} VMPhysRegion;

// I tried to keep this smol
typedef struct {
    Node rbtree_node;
    uintptr_t vaddr_start;
    uint16_t size_pages; // we'd probably never have over 0x10000 pages right?

    // we assume that the regions are mapped *in order* to the virtual memory.
    // it is NOT guaranteed to be mapped up to the end of the virt mem region.
    LList(VMPhysicalRegion) phys_regions;
} VMRegion;

// a full virtual memory space, one for each process
typedef struct {
    uintptr_t cr3; /* *technically* cr3 only refers to x86 but I use it to just
                    * refer to the physical address of the highest page table level
                    * (usually pml4 or potentially pml5) */

    Tree regions;  // allocated virtual memory regions in an rbtree

    VMemArena *arena; // for virtual memory allocation
} VMSpace;

VMSpace *create_virtual_memory_space(void);
VMRegion *vmregion_create(VMSpace *vmspace, uintptr_t vaddr_start, uint16_t size_pages);
void *vmm_valloc_backed(VMSpace *vmspace, size_t num_pages, uint64_t flags);
void vmm_map_page(VMSpace *vmspace, uintptr_t vaddr, uintptr_t paddr, uint64_t flags);
