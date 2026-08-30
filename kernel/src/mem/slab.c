#include <slab.h>
#include <util.h>
#include <string.h>
#include <kprintf.h>
#include <kernel.h>
#include <mm.h>
#include <isa/cpu.h>
#include <pma.h>

Cache *cache_create(uint64_t object_size) {
    if (object_size > PAGE_BYTES)
        kpanic("Object size too large (must be under 1 page)");
    else if (object_size < sizeof(LList))
        kpanic("Object size too small (must be over sizeof(LList))");

    Cache *cache = (Cache*) (pma_valloc());
    memset(cache, 0, PAGE_BYTES);

    cache->object_size = object_size;
    cache->objects_per_slab = (PAGE_BYTES - sizeof(Slab)) / object_size;

    list_init(&cache->free);
    list_init(&cache->partial);
    list_init(&cache->filled);

    return cache;
}

// create a new free slab for a cache & return it
Slab *cache_grow(Cache *cache) {
    Slab *new_slab = (Slab*) (pma_valloc());
    memset(new_slab, 0, PAGE_BYTES);

    list_insert(&cache->free, &new_slab->list);
    list_init(&new_slab->objects);
    new_slab->cache = cache;
    new_slab->num_objects_free = cache->objects_per_slab;

    // add a bunch of empty objects
    for (size_t i = 0; i < cache->objects_per_slab; i++) {
        LList *object = (void*)((uintptr_t)new_slab + sizeof(Slab) + (cache->object_size * i));
        list_insert(&new_slab->objects, object);
    }
    return new_slab;
}

// allocates an object
void *slab_alloc(Cache *cache) {
    spinlock_acquire(&cache->lock);

    /* first, find a slab to use (or create one if
     * there's none with free objects) */
    Slab *slab;
    if (!list_empty(&cache->partial)) slab = CONTAINER_OF(cache->partial.next, Slab, list); 
    else if (!list_empty(&cache->free)) slab = CONTAINER_OF(cache->free.next, Slab, list);
    else slab = cache_grow(cache);

    // get the object to return the memory of & remove it from the freelist
    if (list_empty(&slab->objects)) {
        kpanic("Got empty list in slab_alloc");
    }
    LList *object = slab->objects.next;
    list_remove(object);
    slab->num_objects_free--;

    // move it to another list
    list_remove(&slab->list);
    if (!list_empty(&slab->objects)) {
        // it can't be free anymore but it's definitely still got some avaliable memory
        if (slab->num_objects_free == 0) kpanic("unreachable (slab_alloc)");
        list_insert(&cache->partial, &slab->list);
    } else if (slab->num_objects_free == 0) {
        // it's full
        list_insert(&cache->filled, &slab->list);
    } else kpanic("unreachable");

    spinlock_release(&cache->lock);
    return object;
}

bool is_object_on_slab(Slab *slab, void *object) {
    uintptr_t _slab = (uintptr_t) slab;
    uintptr_t _obj  = (uintptr_t) object;
    return (_obj >= _slab + sizeof(Slab)) &&
           (_obj < _slab + sizeof(Slab) * slab->cache->objects_per_slab);
}

// checks if an object is on any slab within a linked list of slabs. if so then it returns
// the slab, otherwise it returns NULL if not found.
// this is just a linear search, probably could be faster but I'm not sure how in a way that's
// not gonna be unnecessarily memory greedy. feel free to open an issue with an idea.
Slab *is_object_on_slab_list(LList *list, void *object) {
    for (LList *at = list->next; at != list; at = at->next) {
        Slab *this_slab = CONTAINER_OF(at, Slab, list);
        if (is_object_on_slab(this_slab, object)) return this_slab;
    }
    return NULL;
}

// frees an object on a slab
void slab_free(Cache *cache, void *object) {
    spinlock_acquire(&cache->lock);

    // find the slab that the object is on, checking partial and full
    Slab *slab;
    bool from_full = false;
    /* yes I know the second one has an empty statement body but I feel like it's better for readability
     * this way so I don't really care */
    if      ((slab=is_object_on_slab_list(&cache->filled,  object)) != NULL) from_full = true;
    else if ((slab=is_object_on_slab_list(&cache->partial, object)) != NULL) {}
    else kpanic("invalid slab object free");

    list_insert(&slab->objects, object);
    slab->num_objects_free++;

    if (from_full) {
        // if it was taken from a full slab then it will no longer be full slab, so it'll need to become partial
        LList *insert_into = (cache->object_size > 1) ? &cache->partial : &cache->free;
        list_remove(&slab->list);
        list_insert(insert_into, &slab->list);
    } else if (slab->num_objects_free == cache->objects_per_slab) {
        // if it was taken from partial and is now free then it also must be moved
        list_remove(&slab->list);
        list_insert(&cache->free, &slab->list);
    }
    
    spinlock_release(&cache->lock);
}

void cache_free(Cache *cache) {
    // there really shouldn't be much we need to destroy a cache for anyways
    (void) cache;
    kpanic("TODO (cache_free)");
}
