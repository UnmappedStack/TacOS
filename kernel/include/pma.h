#pragma once
#include <stddef.h>
#include <stdint.h>
#include <list.h>

// this should be moved to a unified vmem+pmem file once there is one
#define PAGE_BYTES 4096

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
