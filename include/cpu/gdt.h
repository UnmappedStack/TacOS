#pragma once
#include <stdint.h>

typedef struct {
    uint16_t size;
    uint64_t offset;
} __attribute__((packed)) GDTR;

void init_GDT();
