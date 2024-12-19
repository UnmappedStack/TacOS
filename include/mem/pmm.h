#pragma once
#include <stdint.h>
#include <list.h>
#include <stddef.h>

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
void kfree(uintptr_t addr, size_t num_pages);
