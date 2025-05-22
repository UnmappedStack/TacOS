#pragma once
#include <list.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    struct list list;
    uint64_t flags;
    uintptr_t addr;
    size_t num_pages;
    bool is_program_binary;
} Memregion;

void init_memregion(void);
// See the note in memregion.c
Memregion *add_memregion(Memregion **list, uintptr_t addr, size_t num_pages,
                         bool is_program_binary, uint64_t flags);
void delete_memregion_list(Memregion **list);
void delete_memregion(Memregion *element);
Memregion *memregion_clone(Memregion *original, uint64_t old_page_tree,
                           uint64_t new_page_tree);
Memregion *memregion_find(Memregion *list, uintptr_t addr);
void memregion_add_kernel(Memregion **list);
