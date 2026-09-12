#pragma once
#include <stdint.h>
#include <vmem.h>
#include <rbtree.h>

#define VM_ALLOC_NUM_ORDERS 8

// I tried to keep this smol
typedef struct {
    Node rbtree_node;
    uintptr_t vaddr_start;
    uint16_t size_pages; // we'd probably never have over 0x10000 pages right?
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
