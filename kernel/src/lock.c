#include <lock.h>
#include <isa/cpu.h>
#include <stdbool.h>

/* takes a pointer to a processor global MCSSpinlock and initialises it */
void mcs_lock_init(MCSSpinlock *lock) {
	atomic_flag_clear(&lock->locked);
	lock->next = NULL;
}

/* takes a global lock `lock` and a processor/thread local `local_lock` then enters
 * a queue to take the lock.
 *
 * returns false on error and true once the lock is acquired.
 * if there is no error then it will not return until the lock is acquired. 
 *
 * if it returns false on error then the state of whether the lock is acquired or
 * not is undefined. */
bool mcs_lock_acquire(MCSSpinlock *lock, MCSSpinlock *local_lock) {
	atomic_flag useless;

	if (!local_lock || !lock) return false;
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	
	mcs_lock_init(local_lock);
	__atomic_exchange(&local_lock->locked, &lock->locked, &useless, __ATOMIC_SEQ_CST);

	// load the new local lock into the global lock's next and get the original
	// local lock we are now waiting on to finish first
	MCSSpinlock *waiting_on;
	__atomic_exchange(&lock->next, &local_lock, &waiting_on, __ATOMIC_SEQ_CST);

	if (waiting_on) {
		// give the lock we're waiting on a pointer to ourselves
		__atomic_store_n(&waiting_on->next, local_lock, __ATOMIC_SEQ_CST);
	}

	/* now wait on *ourself* rather than an external lock for until its available.
	 * the thread we are waiting for should update *this* thread when its done with
	 * the lock. */
	while (atomic_flag_test_and_set(&local_lock->locked)) PAUSE();
	atomic_flag_clear(&local_lock->locked); // we dont need to mark ourself as locked

	// congrats you have the lock :)
	// lets make sure everyone knows we have it! they better not take my lock
	atomic_flag_test_and_set(&lock->locked);

	return true;
}

/* takes a local MCSSpinlock which is holding a global lock, releases it
 * for the next thread to use, and frees it. */
void mcs_lock_release(MCSSpinlock *global_lock, MCSSpinlock *local_lock) {
	/* if the global lock points to the current lock, it was never contended so it
	 * is just set to null and will return. if it points to another local lock
	 * then its contended and we need to let it know that it can have the lock now. */
	bool was_contended = !__sync_bool_compare_and_swap(
				&global_lock->next, local_lock, NULL);
	
	if (!was_contended) {
		atomic_flag_clear(&global_lock->locked);
		goto cleanup;
	}

	// looks like someone else wants it, let's let them know they can have it now
	while (!local_lock->next);
	atomic_flag_clear(&local_lock->next->locked);

cleanup:
}
