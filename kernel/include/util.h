#pragma once
#include <io.h>

#define WAIT_FOR_INTERRUPT() __asm__ volatile("hlt")
#define DISABLE_INTERRUPTS() __asm__ volatile("cli")
#define  ENABLE_INTERRUPTS() __asm__ volatile("sti")

// freezes the device by turning off interrupts then waiting (forever) for an interrupt to occur.
// we need to loop over WAIT_FOR_INTERRUPT() instead of doing it once because some interrupts can't be disabled
// (such as exceptions) so once isn't enough.
#define FREEZE_DEVICE() \
    do { \
        DISABLE_INTERRUPTS(); \
        for (;;) WAIT_FOR_INTERRUPT(); \
    } while (0)

#define CPUID(code, a, d) \
    __asm__ volatile("cpuid" : "=a"(*a), "=d"(*d) : "0"(code) : "ebx", "ecx")

#define IO_WAIT() outb(0x80, 0)
