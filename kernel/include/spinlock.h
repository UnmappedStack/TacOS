#pragma once
#include <cpu.h>
#include <stdatomic.h>

typedef struct {
    atomic_flag flag;
    int state;
    int64_t owner;
    bool initialised;
} Spinlock;

void spinlock_acquire(volatile Spinlock *lock);
void spinlock_release(volatile Spinlock *lock);
