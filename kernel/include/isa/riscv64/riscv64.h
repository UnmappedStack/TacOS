#pragma once
#include <stdint.h>
#include <smp.h>

#define SUPERVISOR_INTERRUPT_ENABLE_BIT (1 << 1)

#define WAIT_FOR_INTERRUPT() __asm__ volatile("wfi")
#define ENABLE_INTERRUPTS()  csr_set_bits(CSR_REG_SSTATUS,   SUPERVISOR_INTERRUPT_ENABLE_BIT)
#define DISABLE_INTERRUPTS() csr_clear_bits(CSR_REG_SSTATUS, SUPERVISOR_INTERRUPT_ENABLE_BIT)

#define FREEZE_DEVICE() \
    do { \
        DISABLE_INTERRUPTS(); \
        for (;;) { \
            WAIT_FOR_INTERRUPT(); \
        } \
    } while (0)

void switch_page_tree(uintptr_t cr3);
CPU *get_current_cpu_info(void);
void set_current_cpu_info(CPU *cpu_info);
