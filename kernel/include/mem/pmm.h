#pragma once
#include <list.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    struct list list;
    size_t length;
} Chunk;

void init_pmm(void);
uintptr_t kmalloc(size_t num_pages);
void kfree(uintptr_t addr, size_t num_pages);
