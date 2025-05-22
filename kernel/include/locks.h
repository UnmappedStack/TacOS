#pragma once
#include <stdatomic.h>
#define Spinlock atomic_flag

void spinlock_aquire(Spinlock *lock);
void spinlock_release(Spinlock *lock);
