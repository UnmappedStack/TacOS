#pragma once
#include <stdatomic.h>

// TODO: this is unfair
#if defined(__x86_64__)
#define Spinlock atomic_flag
#define spinlock_acquire(lock) \
    while (atomic_flag_test_and_set(lock)) { \
        __builtin_ia32_pause(); \
    }
#define spinlock_release(lock) \
    atomic_flag_clear(lock);
#elif defined(__riscv)
#define Spinlock int
// TODO: stub
#define spinlock_acquire(lock) {(void)lock;}
#define spinlock_release(lock) {(void)lock;}
#endif
