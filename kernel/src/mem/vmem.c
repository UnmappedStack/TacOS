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
#include <kprintf.h>
#include <util.h>
#include <kernel.h>

void vmem_init(void) {
    kernel_info.vmem_orders_cache  = cache_create(sizeof(VMemOrder));
    kernel_info.vmem_regions_cache = cache_create(sizeof(VMemRegion));
}

/* TODO: also take stuff for importing, qcaches, and allow sleep vs nosleep (we
 * currently assume nosleep)
 * we don't acquire the arena's lock, we assume that it should just not be used
 * by anything else yet. */
void vmem_arena_init(VMemArena *arena, size_t quantum, size_t num_orders) {
    arena->quantum_size = quantum;

    llist_init(&arena->orders);
    size_t region_sz = 1;
    for (size_t i = 0; i < num_orders; i++) {
        VMemOrder *order = slab_alloc(kernel_info.vmem_orders_cache);
        list_insert(&arena->orders, &order->list);

        order->order = i;
        llist_init(&order->regions);

        order->region_sz = region_sz;
        region_sz *= 2;
    }
}

// adds a range to an arena so that stuff from that range can be allocated.
// both region_base and region_size refer to the new region being added and
// are in units of arena->quantum_size. returns true on success and false on error.
bool vmem_add(VMemArena *arena, uintptr_t region_base, size_t region_size) {
    spinlock_acquire(&arena->lock);

    for (LList *list = arena->orders.prev;
         list != &arena->orders; list = list->prev) {
        VMemOrder *this_order = CONTAINER_OF(list, VMemOrder, list);
        VMemOrder *order_up   = CONTAINER_OF(list->next, VMemOrder, list); // the order double the size of this one
        
        if ((list->next == &arena->orders && region_size >= this_order->region_sz) ||
            (region_size >= this_order->region_sz && region_size <= order_up->region_sz)) {
            // its the right size for this region, insert it
            VMemRegion *region = slab_alloc(kernel_info.vmem_regions_cache);
            region->base = region_base, region->size = region_size;
            llist_insert(&this_order->regions, &region->list);

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
