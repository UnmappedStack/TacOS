#pragma once
#include <stdint.h>

typedef struct {
    uint16_t  limit1;
    uint16_t  base1;
    uint8_t   base2;
    uint8_t   access;
    uint8_t   limit_and_flags;
    uint8_t   base3;
}__attribute__((packed)) GDTDescriptor;

typedef struct {
    uint16_t size;
    uint64_t offset;
}__attribute__((packed)) GDTR;

void gdt_init(void);
