#pragma once

#define DISABLE_INTERRUPTS() \
    asm volatile ( \
        "cli" \
    )

#define ENABLE_INTERRUPTS() \
    asm volatile ( \
        "sti" \
    )

#define WAIT_FOR_INTERRUPT() \
    asm volatile ( \
        "hlt" \
    )

#define HALT_DEVICE() \
    do { \
        DISABLE_INTERRUPTS(); \
        for (;;) { \
            WAIT_FOR_INTERRUPT(); \
        } \
    } while (0)
