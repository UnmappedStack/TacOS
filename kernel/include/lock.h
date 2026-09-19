#pragma once
#include <stdatomic.h>
#include <assert.h>
#include <stddef.h>

/* we keep two main types of non-scheduler-blocking locks here:
 *      - dumb spinlocks: just dumb unfair spinlocks, which are small in memory but
 *        unfair and bad for cache locality, so use them only for finegrained
 *        stuff
 *      - mcs spinlocks: fair and cache friendly spinlocks, but chonky in
 *        memory, so use them for less finegrained stuff
 */

/* mcs spinlocks*/
typedef struct MCSSpinlock MCSSpinlock;
struct MCSSpinlock {
	MCSSpinlock *next;
	bool locked;
};

void mcs_spinlock_acquire(MCSSpinlock *lock, MCSSpinlock *local_lock);
void mcs_spinlock_init(MCSSpinlock *lock);
void mcs_spinlock_release(MCSSpinlock *global_lock, MCSSpinlock *local_lock);

#define ENABLE_INTERRUPTS() \
    do { \
        CPU *cpu = get_current_cpu_info(); \
        if (!cpu) { \
            FORCE_ENABLE_INTERRUPTS(); \
        } else { \
            size_t *level = &cpu->interrupt_disable_level; \
            if (*level) { \
                (*level)--; \
            } \
            if (!(*level)) FORCE_ENABLE_INTERRUPTS(); \
        } \
    } while (0)

#define DISABLE_INTERRUPTS() \
    do { \
        CPU *cpu = get_current_cpu_info(); \
        if (!cpu) { \
            FORCE_DISABLE_INTERRUPTS(); \
        } else { \
            size_t *level = &cpu->interrupt_disable_level; \
            (*level)++; \
            FORCE_DISABLE_INTERRUPTS(); \
        } \
    } while (0)

/* dumb spinlocks */
#define DumbLock atomic_flag
#define dumblock_acquire(lock) \
    do { \
        DISABLE_INTERRUPTS(); \
        while (atomic_flag_test_and_set(lock)) { \
            PAUSE(); \
        } \
    } while (0)

#define dumblock_release(lock) \
    do { \
        atomic_flag_clear(lock); \
        ENABLE_INTERRUPTS(); \
    } while (0)
