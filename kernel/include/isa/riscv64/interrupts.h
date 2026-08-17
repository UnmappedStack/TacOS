#pragma once
// probably worth nothing that stuff like enabling/disabling/waiting for interrupts
// are in include/isa/riscv64/riscv64.h rather than here

#define CALL_INTERRUPT

void interrupts_init(void);
