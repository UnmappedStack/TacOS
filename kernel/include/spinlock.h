#pragma once
#include <cpu.h>
#include <stdatomic.h>

typedef struct {
    atomic_flag flag;
    int state;
    int64_t owner;
    bool initialised;
} Spinlock;

void __spinlock_acquire(volatile atomic_flag *flag);
void __spinlock_release(volatile atomic_flag *flag);
void spinlock_acquire(volatile Spinlock *lock);
void spinlock_release(volatile Spinlock *lock);
