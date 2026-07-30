#pragma once
#include <stddef.h>
#include <mm.h>
#include <stdint.h>
#include <list.h>

typedef struct {
    struct list list; // points to next+prev PMMNode
    size_t size_pages;
    // We don't need a FREE flag because if it's in the list then we
    // already know that it's free.
    // We also don't need the base because it will always have its pointer at the base.
} PMMNode;

void pma_init(void);

uintptr_t pma_palloc(void);
void pma_pfree(uintptr_t ptr);

uintptr_t pma_valloc(void);
void pma_vfree(uintptr_t ptr);
