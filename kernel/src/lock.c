#include <lock.h>
#include <kprintf.h>
#include <isa/cpu.h>
#include <stdbool.h>
#include <assert.h>

/* takes a pointer to a processor global MCSSpinlock and initialises it */
void mcs_lock_init(MCSSpinlock *lock) {
	lock->next = NULL;
    lock->locked = false;
}

/* takes a global lock `lock` and a processor/thread local `local_lock` then enters
 * a queue to take the lock.
 *
 * returns false on error and true once the lock is acquired.
 * if there is no error then it will not return until the lock is acquired. 
 *
 * if it returns false on error then the state of whether the lock is acquired or
 * not is undefined. */
void mcs_spinlock_acquire(MCSSpinlock *lock, MCSSpinlock *local_lock) {
	assert(local_lock && lock);

    DISABLE_INTERRUPTS();
	mcs_lock_init(local_lock);

	// load the new local lock into the global lock's next and get the original
	// local lock we are now waiting on to finish first
	MCSSpinlock *waiting_on = NULL;
	__atomic_exchange(&lock->next, &local_lock, &waiting_on, __ATOMIC_SEQ_CST);

	if (waiting_on) {
		// give the lock we're waiting on a pointer to ourselves
		__atomic_store_n(&waiting_on->next, local_lock, __ATOMIC_SEQ_CST);

		// mark ourselves as locked
		__atomic_store_n(&local_lock->locked, true, __ATOMIC_SEQ_CST);

		/* now wait on *ourself* rather than an external lock for until its available.
		 * the thread we are waiting for should update *this* thread when its done with
		 * the lock. */
		while (__atomic_load_n(&local_lock->locked, __ATOMIC_SEQ_CST)) PAUSE();
	}
}

/* takes a local MCSSpinlock which is holding a global lock, releases it
 * for the next thread to use, and frees it. */
void mcs_spinlock_release(MCSSpinlock *global_lock, MCSSpinlock *local_lock) {
	/* if the global lock points to the current lock, it was never contended so it
	 * is just set to null and will return. if it points to another local lock
	 * then its contended and we need to let it know that it can have the lock now. */
	bool was_contended = !__sync_bool_compare_and_swap(
				&global_lock->next, local_lock, NULL);
	
	if (!was_contended) {
		__atomic_store_n(&global_lock->locked, false, __ATOMIC_SEQ_CST);
        ENABLE_INTERRUPTS();
		return;
	}

	// looks like someone else wants it, let's let them know they can have it now
	while (!local_lock->next);
	__atomic_store_n(&local_lock->next->locked, false, __ATOMIC_SEQ_CST);
    ENABLE_INTERRUPTS();
}
