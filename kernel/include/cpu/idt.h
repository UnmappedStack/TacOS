#pragma once
#include <stdint.h>

#define IDT_INTERRUPT_TYPE 0x8E
#define IDT_TRAP_TYPE 0x8F
#define IDT_SOFTWARE_TYPE 0xEF

typedef struct {
    uint16_t offset1;
    uint16_t segment_selector;
    uint8_t rsvd;
    uint8_t flags;
    uint16_t offset2;
    uint32_t offset3;
    uint32_t reserved;
} __attribute__((packed)) IDTEntry;

typedef struct {
    uint16_t size;
    uint64_t offset;
} __attribute__((packed)) IDTR;

void init_IDT(void);
void set_IDT_entry(uint32_t vector, void *isr, uint8_t flags, IDTEntry *IDT);
void load_IDT(void);
