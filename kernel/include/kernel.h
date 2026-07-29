#pragma once

#include <list.h>
#include <framebuffer.h>

// shared kernel struct for everything global.
// this will probably also contain locking information later when there's smp etc
typedef struct {
    Framebuffer framebuffers[MAX_FRAMEBUFFERS];
    struct list pmm_nodes;
    uintptr_t hhdm;
    struct limine_memmap_response *memmap;
} KernelInfo;

extern KernelInfo kernel_info;
