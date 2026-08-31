#pragma once
#include <stdatomic.h>

// TODO: this is unfair
#define Spinlock atomic_flag
#define spinlock_acquire(lock) \
    while (atomic_flag_test_and_set(lock)) { \
        PAUSE(); \
    }
#define spinlock_release(lock) \
    atomic_flag_clear(lock);
