#pragma once
#include <list.h>
#include <lock.h>

// base size of allocations
#define VMEM_QUANTUM PAGE_SIZE

// TODO: this needs to be in a second llist in order of address
typedef struct {
    LList list; // for chain of VmemRegions this is in, owned by a VMemOrder
    
    /* both in units of quantum, eg if it is for virtual memory allocation
     * then base would be the vpn not the address, and size would be the number
     * of page frames */
    uintptr_t base;
    size_t size;
} VMemRegion;

typedef struct {
    LList list; // for the chain of VmemOrders this is in

    uint16_t order; // this could maybe be just one byte idk
    uint32_t region_sz; // order^2 in units of quantum_size
    // linked list of regions of size region_sz
    LList(VMemRegionEntry) regions;
} VMemOrder;

typedef struct {
    Spinlock lock;
    uint16_t quantum_size;
    LList(VMemOrder) orders;
} VMemArena;

void vmem_arena_init(VMemArena *arena, size_t quantum, size_t num_orders);
bool vmem_add(VMemArena *arena, uintptr_t region_base, size_t region_size);
void vmem_init(void);
