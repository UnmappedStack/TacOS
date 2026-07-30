#pragma once

// just for basic headers shared between vmm and pmm that
// don't necessarily belong to one more than another

#define PAGE_BYTES 4096

/* macro utility ops */
#define PAGE_ALIGN_DOWN(addr) ((addr / PAGE_BYTES) * PAGE_BYTES)
#define PAGE_ALIGN_UP(x) ((((x) + (PAGE_BYTES-1)) / PAGE_BYTES) * PAGE_BYTES)
