#include <cpu.h>
#include <spinlock.h>
#include <kernel.h>
#include <list.h>
#include <mem/slab.h>
#include <printf.h>
#include <scheduler.h>
#include <string.h>

void init_scheduler(void) {
    kernel.scheduler = (SchedulerQueue){0};
    kernel.scheduler.cache = init_slab_cache(sizeof(Task), "Scheduler Queue");
    // init the kernel task
    Task *krnl_task = task_add();
    krnl_task->pml4 = kernel.cr3;
    krnl_task->entry = (void *)&_start;
    krnl_task->parent = krnl_task; // The kernel task is it's own parent
    krnl_task->flags = 0;
    krnl_task->rsp = USER_STACK_PTR;
    krnl_task->memregion_list = 0;
    memset(krnl_task->resources, 0, sizeof(krnl_task->resources));
    memset(krnl_task->children, 0, sizeof(krnl_task->children));
    CURRENT_TASK = krnl_task;
    kernel.scheduler.initiated = true;
    // memregion_add_kernel(&krnl_task->memregion_list);
    printf("Initiated scheduler.\n");
}

Task *task_add(void) {
    Task *new_task = slab_alloc(kernel.scheduler.cache);
    new_task->pid = kernel.scheduler.pid_upto++;
    new_task->flags = 0;
    if (kernel.scheduler.list == 0) {
        printf(" -> task_add() initiates queue\n");
        list_init(&new_task->list);
        kernel.scheduler.list = &new_task->list;
    } else {
        printf(" -> task_add() inserts task\n");
        list_insert(kernel.scheduler.list, &new_task->list);
    }
    printf(" -> New task = %p, pid = %i\n", new_task, new_task->pid);
    return new_task;
}

Spinlock scheduler_lock = {0};
void lock_scheduler(void) {
    spinlock_acquire(&scheduler_lock);
}

void unlock_scheduler(void) {
    spinlock_release(&scheduler_lock);
}

extern bool in_panic;
Task *task_select(void) {
    DISABLE_INTERRUPTS();
    if (in_panic)
        HALT_DEVICE();
    Task *first_task = CURRENT_TASK;
    first_task->flags &= ~TASK_RUNNING;
    CURRENT_TASK = (Task *)CURRENT_TASK->list.next;
    int tasks_just_running = 0;
    while (
        !(CURRENT_TASK->flags & TASK_PRESENT) || CURRENT_TASK->waiting_for ||
        CURRENT_TASK->flags & TASK_RUNNING) {
        if (CURRENT_TASK->flags & TASK_RUNNING)
            tasks_just_running++;
        if (first_task == (Task *)CURRENT_TASK && !tasks_just_running) {
            printf("No avaliable task! Was init killed?\n");
            HALT_DEVICE();
        } else if (tasks_just_running) {
            // give other cores a chance because it may take some time
            unlock_scheduler();
            for (size_t i = 0; i < 5000; i++)
                __builtin_ia32_pause();
            lock_scheduler();
        }
        CURRENT_TASK = (Task *)CURRENT_TASK->list.next;
    }
    CURRENT_TASK->flags |= TASK_RUNNING;
    return (Task *)CURRENT_TASK;
}

// for asm context switch
Task *get_current_task(void) { return CURRENT_TASK; }
