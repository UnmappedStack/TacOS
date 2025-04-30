#pragma once

#define DISABLE_INTERRUPTS() \
    __asm__ volatile ( \
        "cli" \
    )

#define ENABLE_INTERRUPTS() \
    __asm__ volatile ( \
        "sti" \
    )

#define WAIT_FOR_INTERRUPT() \
    __asm__ volatile ( \
        "hlt" \
    )

#define HALT_DEVICE() \
    do { \
        DISABLE_INTERRUPTS(); \
        for (;;) { \
            WAIT_FOR_INTERRUPT(); \
        } \
    } while (0)

#define IO_WAIT() \
    __asm__ volatile ( \
        "outb %%al, $0x80" : : : "al" \
    )


#define CPUID(code, a, d) \
    __asm__ volatile( \
        "cpuid" : "=a"(*a), "=d"(*d) : "0"(code) : "ebx", "ecx" \
    )
