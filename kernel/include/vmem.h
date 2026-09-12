#pragma once
#include <slab.h>
#include <stddef.h>
#include <rbtree.h>
#include <list.h>
#include <lock.h>

typedef enum {
    VMEM_BESTFIT,    /* search for the smallest segment on list n that satisfies it */
    VMEM_INSTANTFIT, /* do the first segment on list n+1 */
    VMEM_NEXTFIT     /* we don't bother with this yet but its just the region
                      * immediately after the last */
} VMemAllocType;

// TODO: this needs to be in a second llist in order of address
typedef struct {
    LList list; // for chain of VmemRegions this is in, owned by a VMemOrder
    Node rbtree_cache_node; 

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

typedef struct VMemArena VMemArena;
struct VMemArena {
    Spinlock lock;
    uint16_t quantum_size;
    Tree(VMemRegion) cached_region_tags;

    void *(*import_alloc_fn)(VMemArena*, size_t sz, VMemAllocType);
    void  (*import_free_fn )(VMemArena*, void *resource);
    VMemArena *import_source;

    uint8_t num_orders; // max 63 so 1 byte is fine
    uint64_t orders_bitmap;
    VMemOrder orders[0];
};

VMemArena *vmem_arena_init(size_t quantum, Cache *arena_cache,
                           void *(*import_alloc_fn)(VMemArena*, size_t sz, VMemAllocType),
                           void  (*import_free_fn )(VMemArena*, void *resource),
                           VMemArena *import_source);
bool vmem_add(VMemArena *arena, uintptr_t region_base, size_t region_size);
void *vmem_alloc(VMemArena *arena, size_t size, VMemAllocType flag);
void vmem_free(VMemArena *arena, void *resource);
void vmem_init(void);
