#pragma once

#define WAIT_FOR_INTERRUPT() __asm__ volatile("wfi")

// TODO: disable interrupts first
#define FREEZE_DEVICE() \
    do { \
        for (;;) { \
            WAIT_FOR_INTERRUPT(); \
        } \
    } while (0)

#define ENABLE_INTERRUPTS() FREEZE_DEVICE() // TODO: stub
#define DISABLE_INTERRUPTS() FREEZE_DEVICE() // TODO: stub
