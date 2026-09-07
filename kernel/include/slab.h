#pragma once
#include <list.h>
#include <stdint.h>
#include <lock.h>

typedef struct {
    /* these each are a circular doubly linked list of slabs.
     * each slab contains its own freelist of slabs for the cache's allocation
     * size and will be moved into the appropriate list as needed.*/
    LList free;
    LList partial;
    LList filled;

    uint64_t object_size;
    uint64_t objects_per_slab;

    Spinlock lock;
} Cache;

typedef struct {
    LList list; // other slabs on this slab group in the cache
    LList objects; // all *free* objects on the slab (freelist)
    uint64_t num_objects_free;
    Cache *cache; // cache which owns this slab

    /* objects follow in memory */
} Slab;

Cache *cache_create(uint64_t object_size);
void *slab_alloc(Cache *cache);
void slab_free(Cache *cache, void *object);
void rbtree_init(void);
