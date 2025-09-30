#pragma once
#include <cpu.h>
#include <stdatomic.h>
#define Spinlock atomic_flag

#define spinlock_acquire(lock) \
    while (atomic_flag_test_and_set(lock)) { \
        __builtin_ia32_pause(); \
    }

#define spinlock_release(lock) \
    atomic_flag_clear(lock);
