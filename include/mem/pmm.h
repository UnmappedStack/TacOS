#pragma once
#include <stdint.h>
#include <list.h>
#include <stddef.h>

// TODO: Move this into a different file like paging.h
#define PAGE_SIZE 4096
#define PAGE_ALIGN_DOWN(x) (((x) / PAGE_SIZE) * PAGE_SIZE)

static const char *mem_types[] = {
    "Usable                ",
    "Reserved              ",
    "ACPI Reclaimable      ",
    "ACPI NVS              ",
    "Bad Memory            ",
    "Bootloader Reclaimable",
    "Executable and Modules",
    "Framebuffer           ",
};

typedef struct {
    struct list list;
    size_t length;
} Chunk;

void init_pmm();
uintptr_t kmalloc(size_t num_pages);
