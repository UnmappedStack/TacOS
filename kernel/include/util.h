#pragma once

#define WAIT_FOR_INTERRUPT() asm volatile("hlt")
#define DISABLE_INTERRUPTS() asm volatile("cli")
#define  ENABLE_INTERRUPTS() asm volatile("sti")

// freezes the device by turning off interrupts then waiting (forever) for an interrupt to occur.
// we need to loop over WAIT_FOR_INTERRUPT() instead of doing it once because some interrupts can't be disabled
// (such as exceptions) so once isn't enough.
#define FREEZE_DEVICE() \
    do { \
        DISABLE_INTERRUPTS(); \
        for (;;) WAIT_FOR_INTERRUPT(); \
    } while (0)
