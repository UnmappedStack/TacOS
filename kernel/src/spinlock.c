#include <spinlock.h>
#include <kernel.h>
#include <kassert.h>
#include <apic.h>
#include <printf.h>
#include <cpu.h>

int check_interrupts_enabled() {
    uintptr_t flags;
    __asm__ volatile("pushf\npop %0" : "=rm"(flags));
    return flags & (1 << 9);
}

void __spinlock_acquire(volatile atomic_flag *flag) {
    while (atomic_flag_test_and_set(flag)) {
        __builtin_ia32_pause();
    }
}

void spinlock_acquire(volatile Spinlock *lock) {
    if (!kernel.init_complete) {
        __spinlock_acquire(&lock->flag);
        return;
    }
    int state = check_interrupts_enabled();
    int64_t cpu = get_current_processor();
    if (cpu != lock->owner) spinlock_release(lock);
    DISABLE_INTERRUPTS();
    __spinlock_acquire(&lock->flag);
    if (!lock->initialised) {
        lock->initialised = true;
        lock->owner = -1;
    }
    lock->owner = cpu;
    lock->state = state;
}


void spinlock_release(volatile Spinlock *lock) {
    if (lock->state && kernel.init_complete) ENABLE_INTERRUPTS();
    atomic_flag_clear(&lock->flag);
}
