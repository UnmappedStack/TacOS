#include <slab.h>
#include <kprintf.h>
#include <kernel.h>
#include <mm.h>
#include <panic.h>
#include <pma.h>

Cache *cache_create(uint64_t object_size) {
    if (object_size > PAGE_BYTES)
        kpanic("Object size too large (must be under 1 page)");

    Cache *cache = (Cache*) (pma_valloc());
    cache->object_size = object_size;
    cache->objects_per_slab = PAGE_ALIGN_UP(object_size + sizeof(Slab)) / object_size - sizeof(Slab);

    list_init(&cache->free);
    list_init(&cache->partial);
    list_init(&cache->filled);

    return cache;
}

// create a new free slab for a cache & return it
Slab *cache_grow(Cache *cache) {
    Slab *new_slab = (Slab*) (pma_valloc());

    list_insert(&cache->free, &new_slab->list);
    list_init(&new_slab->objects);
    new_slab->cache = cache;

    // add a bunch of empty objects
    for (size_t i = 0; i < cache->objects_per_slab; i++) {
        struct list *object = (void*)((uintptr_t)new_slab + sizeof(Slab) + (cache->object_size * i));
        list_insert(&new_slab->objects, object);
    }
    return new_slab;
}

void *slab_alloc(Cache *cache) {
    /* first, find a slab to use (or create one if
     * there's none with free objects) */
    Slab *slab;
    if (!list_empty(&cache->partial)) slab = (Slab*) &cache->partial;
    else if (!list_empty(&cache->free))  slab = (Slab*) &cache->free;
    else slab = cache_grow(cache);

    // get the object to return the memory of & remove it from the freelist
    struct list *object = slab->objects.next;
    list_remove(object);

    // move it to another list
    list_remove(&slab->list);
    if (!list_empty(&slab->objects)) {
        // it can't be free anymore but it's definitely still got some avaliable memory
        list_insert(&cache->partial, &slab->list);
    } else {
        // it's full
        list_insert(&cache->filled, &slab->list);
    }
    return object;
}
