#pragma once
// probably worth nothing that stuff like enabling/disabling/waiting for interrupts
// are in include/isa/riscv64/riscv64.h rather than here

#include <stdint.h>

// a copy of this is defined in interrupt.S
typedef struct {
    uint64_t return_addr;
    uint64_t cause, val;
    uint64_t sstatus;
    uint64_t regs[32];
} InterruptStackFrame;

void interrupts_init(void);
