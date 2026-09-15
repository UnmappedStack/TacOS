#pragma once
#include <stdatomic.h>

/* we keep two main types of non-scheduler-blocking locks here:
 *      - dumb spinlocks: just dumb unfair spinlocks, which are small in memory but
 *        unfair and bad for cache locality, so use them only for finegrained
 *        stuff
 *      - mcs spinlocks: fair and cache friendly spinlocks, but chonky in
 *        memory, so use them for less finegrained stuff
 */

/* dumb spinlocks */
#define DumbLock atomic_flag
#define dumblock_acquire(lock) \
    while (atomic_flag_test_and_set(lock)) { \
        PAUSE(); \
    }

#define dumblock_release(lock) \
    atomic_flag_clear(lock);

/* mcs spinlocks*/
typedef struct MCSSpinlock MCSSpinlock;
struct MCSSpinlock {
	MCSSpinlock *next;
	bool locked;
};

void mcs_spinlock_acquire(MCSSpinlock *lock, MCSSpinlock *local_lock);
void mcs_spinlock_init(MCSSpinlock *lock);
void mcs_spinlock_release(MCSSpinlock *global_lock, MCSSpinlock *local_lock);
