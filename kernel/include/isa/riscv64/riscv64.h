#pragma once

#define SUPERVISOR_INTERRUPT_ENABLE_BIT (1 << 1)

#define WAIT_FOR_INTERRUPT() __asm__ volatile("wfi")
#define ENABLE_INTERRUPTS()    csr_set_bits(CSR_REG_SSTATUS, SUPERVISOR_INTERRUPT_ENABLE_BIT)
#define DISABLE_INTERRUPTS() csr_clear_bits(CSR_REG_SSTATUS, SUPERVISOR_INTERRUPT_ENABLE_BIT)

#define FREEZE_DEVICE() \
    do { \
        DISABLE_INTERRUPTS(); \
        for (;;) { \
            WAIT_FOR_INTERRUPT(); \
        } \
    } while (0)
