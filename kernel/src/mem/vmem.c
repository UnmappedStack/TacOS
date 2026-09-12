/* implementation of the vmem allocator (described here:
 * http://www.parrot.org/sites/www.parrot.org/files/vmem.pdf)
 *
 * basically its just a simple but decent number range allocator. this is used
 * for virtual memory allocation, but can also theoretically be used to allocate
 * other stuff like PIDs, FDs, etc. (although it wouldn't be in TacOS since those
 * don't exist with Everything is an Object...).
 *
 * you might notice there is one single big lock for each entire arena. this
 * is technically correct according to the vmem paper, though it might be good
 * to see if I can make it a bit more finegrained later idk.
 *
 * also i decided to change some stuff from how it was done in the original
 * paper, particularly in terms of api. for example you can't init the first
 * region from vmem_arena_init, rather you have to separately add regions. */

#include <vmem.h>
#include <string.h>
#include <slab.h>
#include <assert.h>
#include <kprintf.h>
#include <util.h>
#include <kernel.h>

void vmem_init(void) {
    kernel_info.vmem_regions_cache = cache_create(sizeof(VMemRegion));
}

/* TODO: also take stuff for importing, qcaches, and allow sleep vs nosleep (we
 * currently assume nosleep)
 * we don't acquire the arena's lock, we assume that it should just not be used
 * by anything else yet.
 * arena_cache expects a cache to allocate the VMemArena on, and will assume
 * the number of orders from the cache's object size. */
VMemArena *vmem_arena_init(size_t quantum, Cache *arena_cache) {
    assert(arena_cache->object_size >= sizeof(VMemArena) + sizeof(VMemOrder)*1);
    assert(!((arena_cache->object_size - sizeof(VMemArena)) % sizeof(VMemOrder)));

    VMemArena *arena    = slab_alloc(arena_cache);
    memset(arena, 0, sizeof(VMemArena));
    arena->quantum_size = quantum;
    arena->num_orders   = (arena_cache->object_size - sizeof(VMemArena)) / sizeof(VMemOrder);

    size_t region_sz = 1;
    for (size_t i = 0; i < arena->num_orders; i++) {
        VMemOrder *order = &arena->orders[i];

        order->order = i;
        llist_init(&order->regions);

        order->region_sz = region_sz;
        region_sz *= 2;
    }

    return arena;
}

// adds a range to an arena so that stuff from that range can be allocated.
// both region_base and region_size refer to the new region being added and
// are in units of arena->quantum_size. returns true on success and false on error.
bool vmem_add(VMemArena *arena, uintptr_t region_base, size_t region_size) {
    spinlock_acquire(&arena->lock);

    for (size_t i = arena->num_orders-1; i >= 0; i--) {
        VMemOrder *this_order = &arena->orders[i];
        VMemOrder *order_up   = &arena->orders[i+1]; // the order double the size of this one
        
        if ((i+1 == arena->num_orders && region_size >= this_order->region_sz) ||
            (region_size >= this_order->region_sz && region_size <= order_up->region_sz)) {
            // its the right size for this region, insert it
            VMemRegion *region = slab_alloc(kernel_info.vmem_regions_cache);
            region->base = region_base, region->size = region_size;
            llist_insert(&this_order->regions, &region->list);
            arena->orders_bitmap |= 1 << i;

            spinlock_release(&arena->lock)
            return true;
        }
    }

    // it doesn't fit in any section, return error (i think this should be
    // unreachable technically?)
    klogf(LOG_ERROR, "vmem_add can't fill any section\n");
    spinlock_release(&arena->lock);
    return false;
}

VMemOrder *find_order_by_size(VMemArena *arena, size_t size) {
    for (size_t i = 0; i < arena->num_orders; i++) {
        VMemOrder *this_order = &arena->orders[i];
        if ((size >= this_order->region_sz && size < this_order->region_sz * 2)
                || (i+1 == arena->num_orders) /* last one */) {
            return this_order;
        }
    }
    return NULL;
}

VMemRegion *find_bestfit_in_order(VMemOrder *order, size_t size) {
    VMemRegion *best_fit_region = NULL;
    llist_iter(&order->regions, list) {
        VMemRegion *region = CONTAINER_OF(list, VMemRegion, list);
        if (region->size < size) continue;
        if (!best_fit_region || region->size < best_fit_region->size) {
            best_fit_region = region;
        }
    }
    return best_fit_region;
}

/* takes a VMemRegion, splits off a chunk of size `size`, returns its base, then adds the other
 * part of the region back to the arena. */
void *split_and_ret(VMemArena *arena, VMemRegion *source_region, size_t size) {
    if (source_region->size != size) {
        VMemRegion *new_region = slab_alloc(kernel_info.vmem_regions_cache);
        assert(new_region);
        new_region->base = source_region->base + size;
        new_region->size = source_region->size - size;

        VMemOrder *insert_into = find_order_by_size(arena, new_region->size);
        llist_insert(&insert_into->regions, &new_region->list);
    }

    void *ret = (void*) (source_region->base * arena->quantum_size);
    rbtree_insert(&arena->cached_region_tags,
                  &source_region->rbtree_cache_node,
                  source_region->base);

    llist_remove(&source_region->list);
    return ret;
}

// returns a region in base units, that is, NOT in units of the quantum
void *vmem_alloc(VMemArena *arena, size_t size, VMemAllocType flag) {
    assert(size);
    
    spinlock_acquire(&arena->lock);
    VMemOrder *this_order = find_order_by_size(arena, size);
    assert(this_order);

    uint64_t orders_bitmap_after_this_order = arena->orders_bitmap >> this_order->order;
    if (!orders_bitmap_after_this_order) {
        klogf(LOG_ERROR, "vmem: no available memory\n");
        spinlock_release(&arena->lock);
        return NULL;
    }
    size_t get_from_id = count_leading_zeroes(orders_bitmap_after_this_order) + this_order->order;
    VMemOrder *get_from = &arena->orders[get_from_id];

    switch (flag) {
    case VMEM_BESTFIT:
        VMemRegion *region = find_bestfit_in_order(get_from, size);
        if (!region) {
            // TODO: start searching the next regions as a fallback
            klogf(LOG_ERROR, "vmem: no fitting region in freelist n for VMEM_BESTFIT\n");
            spinlock_release(&arena->lock);
            return NULL;
        }

        void *ret = split_and_ret(arena, region, size);
        spinlock_release(&arena->lock);
        return ret;
    case VMEM_INSTANTFIT:
        klogf(LOG_ERROR, "TODO: VMEM_INSTANTFIT");
        spinlock_release(&arena->lock);
        return NULL;
    case VMEM_NEXTFIT:
        klogf(LOG_ERROR, "TODO: VMEM_NEXTFIT");
        spinlock_release(&arena->lock);
        return NULL;
    }
    klogf(LOG_ERROR, "unknown vmem_alloc flag\n");
    spinlock_release(&arena->lock);
    return NULL;
}
