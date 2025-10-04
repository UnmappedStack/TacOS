#pragma once
#include <serial.h>

extern bool in_panic;
#define kassert(c) \
    do { \
        if (!(c)) {\
            write_serial("Kernel panic, assert failed: " #c "\n"); \
            in_panic = true; \
            HALT_DEVICE(); \
        } \
    } while (0)
