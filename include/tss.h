#pragma once
#include <stdint.h>

typedef struct {
    uint32_t rsvd0;
    uint64_t rsp0;
    uint32_t rsvd1[23];
} __attribute__((packed)) TSS;

void init_TSS(void);
