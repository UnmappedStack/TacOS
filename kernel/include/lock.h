#pragma once
#include <stdatomic.h>

/* we keep two main types of non-scheduler-blocking locks here:
 *      - spinlocks: just dumb unfair spinlocks, which are small in memory but
 *        unfair and bad for cache locality, so use them only for finegrained
 *        stuff
 *      - mcs spinlocks: fair and cache friendly spinlocks, but chonky in
 *        memory, so use them for less finegrained stuff
 */

/* dumb spinlocks */
#define Spinlock atomic_flag
#define spinlock_acquire(lock) \
    while (atomic_flag_test_and_set(lock)) { \
        PAUSE(); \
    }

#define spinlock_release(lock) \
    atomic_flag_clear(lock);

/* mcs spinlocks*/
typedef struct MCSSpinlock MCSSpinlock;
struct MCSSpinlock {
	MCSSpinlock *next;
	atomic_flag locked;
};

bool mcs_lock_acquire(MCSSpinlock *lock, MCSSpinlock *local_lock);
void mcs_lock_init(MCSSpinlock *lock);
void mcs_lock_release(MCSSpinlock *global_lock, MCSSpinlock *local_lock);
