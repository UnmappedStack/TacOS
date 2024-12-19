#pragma once
#include <list.h>
#include <stddef.h>

typedef struct Cache Cache;
typedef struct Slab Slab;

struct Cache {
    struct list list;
    char   name[20];
    struct list free_nodes;
    struct list partial_nodes;
    struct list full_nodes;
    size_t obj_size;
    size_t obj_per_slab;
    size_t slab_size_pages;
};

struct Slab {
    struct list list;
    Cache  *cache;
    size_t free_stack_head;
    size_t num_free;
    void   *data;
    void   *data_end;
    // Free stack comes immediately after, then the data
};

Cache *init_slab_cache(size_t obj_size, const char *name);
void  *slab_alloc(Cache *cache);
int    slab_free(Cache *cache, void *addr);
void   cache_destroy(Cache *cache);
