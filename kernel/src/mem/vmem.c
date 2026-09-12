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

    VMemArena *arena = slab_alloc(arena_cache);
    memset(arena, 0, sizeof(VMemArena));
    arena->quantum_size = quantum;

    arena->num_orders = (arena_cache->object_size - sizeof(VMemArena)) / sizeof(VMemOrder);
    if (arena->num_orders > 64) arena->num_orders = 64;

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

// unlike vmem_add(), it MUST be of a size that already fits in a specific region
// (that is, it can at max be 2^n-1 where n is the number of freelists)
bool vmem_add_sized(VMemArena *arena, uintptr_t region_base, size_t region_size) {
    spinlock_acquire(&arena->lock);

    for (size_t i = arena->num_orders-1; i >= 0; i--) {
        VMemOrder *this_order = &arena->orders[i];
        VMemOrder *order_up   = &arena->orders[i+1]; // the order double the size of this one
        
        if ((i+1 == arena->num_orders && region_size >= this_order->region_sz) ||
            (region_size >= this_order->region_sz && region_size <= order_up->region_sz)) {

            klogf(LOG_DEBUG, "add to order %u (region size = %x, base=%x)\n", i, region_size, region_base);
            // its the right size for this region, insert it
            VMemRegion *region = slab_alloc(kernel_info.vmem_regions_cache);
            memset(region, 0, sizeof(VMemRegion));
            region->base = region_base, region->size = region_size;
            llist_insert(&this_order->regions, &region->list);
            arena->orders_bitmap |= 1ULL << i;

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

// should this be somewhere else outside vmem? yeah maybe but i dont care until i
// actually need it somewhere else (TODO)
static inline size_t pow(size_t base, size_t exp) {
    size_t ret = 1;
    for (size_t i = 0; i < exp; i++) {
        ret *= base;
    }
    return ret;
}

// adds a range to an arena so that stuff from that range can be allocated.
// both region_base and region_size refer to the new region being added and
// are in units of arena->quantum_size. returns true on success and false on error.
bool vmem_add(VMemArena *arena, uintptr_t region_base, size_t region_size) {
    uintptr_t base = region_base;
    size_t size_left = region_size;
    size_t max_size = pow(2, arena->num_orders-1);
    assert(max_size);
    while (size_left) {
        size_t size_to_add = (size_left > max_size) ? max_size : size_left;
        if (!vmem_add_sized(arena, base, size_to_add)) return false;

        base += size_to_add;
        size_left -= size_to_add;
    }
    return true;
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
void *split_and_ret(VMemArena *arena, VMemRegion *source_region, VMemOrder *source_order, size_t size) {
    if (source_region->size != size) {
        VMemRegion *new_region = slab_alloc(kernel_info.vmem_regions_cache);
        assert(new_region);
        new_region->base = source_region->base + size;
        new_region->size = source_region->size - size;

        VMemOrder *insert_into = find_order_by_size(arena, new_region->size);
        llist_insert(&insert_into->regions, &new_region->list);
        arena->orders_bitmap |= 1ULL << insert_into->order;
    }

    void *ret = (void*) (source_region->base * arena->quantum_size);
    rbtree_insert(&arena->cached_region_tags,
                  &source_region->rbtree_cache_node,
                  source_region->base);

    llist_remove(&source_region->list);
    if (list_empty(&source_order->regions)) {
        arena->orders_bitmap &= ~(1ULL << source_order->order);
    }
    return ret;
}

// holy long name wtf. at least its descriptive i guess lol
int get_first_nonempty_list_after_list_n(VMemArena *arena, uint64_t n) {
    uint64_t orders_bitmap_after_this_order = arena->orders_bitmap >> n;
    if (!orders_bitmap_after_this_order) {
        klogf(LOG_ERROR, "vmem: no available memory\n");
        return -1;
    }
    size_t get_from_id = count_leading_zeroes(orders_bitmap_after_this_order) + n;
    assert(!list_empty(&arena->orders[get_from_id].regions));
    return get_from_id;
}

// returns a region in base units, that is, NOT in units of the quantum
void *vmem_alloc(VMemArena *arena, size_t size, VMemAllocType flag) {
    int get_from_id;
    void *ret;
    assert(size);
    
    spinlock_acquire(&arena->lock);
    VMemOrder *this_order = find_order_by_size(arena, size);
    assert(this_order);

    switch (flag) {
    case VMEM_BESTFIT:
        get_from_id = get_first_nonempty_list_after_list_n(arena, this_order->order);
        assert(get_from_id >= 0);
        VMemOrder *get_from = &arena->orders[(size_t)get_from_id];

        VMemRegion *region = find_bestfit_in_order(get_from, size);
        if (!region) {
            klogf(LOG_ERROR, "vmem: no fitting region in freelists for VMEM_BESTFIT (oom)\n");
            spinlock_release(&arena->lock);
            return NULL;
        }

        ret = split_and_ret(arena, region, get_from, size);
        spinlock_release(&arena->lock);
        return ret;
    case VMEM_INSTANTFIT:
        if (this_order->order + 1 >= arena->num_orders) {
            // instant fit won't work in this case as there *is* no next list.
            // fall back to best fit.
            klogf(LOG_WARN, "vmem: fell back to best fit rather than instant fit\n");
            spinlock_release(&arena->lock);
            return vmem_alloc(arena, size, VMEM_BESTFIT);
        }
        get_from_id = get_first_nonempty_list_after_list_n(arena, this_order->order + 1);
        if (get_from_id < 0) {
            spinlock_release(&arena->lock);
            return NULL;
        }

        VMemOrder *order_next = &arena->orders[(size_t)get_from_id];

        VMemRegion *first_region = CONTAINER_OF(order_next->regions.next, VMemRegion, list);
        llist_remove(&first_region->list);

        ret = split_and_ret(arena, first_region, order_next, size);
        spinlock_release(&arena->lock);
        return ret;
    case VMEM_NEXTFIT:
        klogf(LOG_ERROR, "TODO: VMEM_NEXTFIT\n");
        spinlock_release(&arena->lock);
        return NULL;
    }
    klogf(LOG_ERROR, "unknown vmem_alloc flag\n");
    spinlock_release(&arena->lock);
    return NULL;
}
