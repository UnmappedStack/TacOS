#pragma once
#include <stdint.h>

typedef struct {
    uint16_t offset1;
    uint16_t segment;
    uint8_t  ist;
    uint8_t  flags;
    uint16_t offset2;
    uint32_t offset3;
    uint32_t rsvd2;
}__attribute__((packed)) IDTGate;

typedef struct {
    uint16_t size;
    uint64_t offset;
}__attribute__((packed)) IDTR;

void idt_init(void);
