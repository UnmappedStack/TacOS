#include <string.h>
#include <scheduler.h>
#include <printf.h>
#include <list.h>
#include <kernel.h>
#include <mem/slab.h>

void init_scheduler() {
    kernel.scheduler = (SchedulerQueue) {0};
    kernel.scheduler.cache = init_slab_cache(sizeof(Task), "Scheduler Queue");
    // init the kernel task
    Task *krnl_task   = task_add();
    krnl_task->pml4   = kernel.cr3;
    krnl_task->entry  = &_start;
    krnl_task->parent = krnl_task; // The kernel task is it's own parent
    krnl_task->flags  = 0;
    krnl_task->rsp    = USER_STACK_PTR;
    krnl_task->memregion_list = 0;
    kernel.scheduler.current_task = krnl_task;
    memregion_add_kernel(&krnl_task->memregion_list);
    printf("Initiated scheduler.\n");
}

Task *task_add() {
    Task *new_task   = slab_alloc(kernel.scheduler.cache);
    new_task->pid    = kernel.scheduler.pid_upto++;
    if (kernel.scheduler.list == 0) {
        list_init(&new_task->list);
        kernel.scheduler.list = &new_task->list;
    } else
        list_insert(kernel.scheduler.list, &new_task->list);
    return new_task;
}

Task *task_select() {
    kernel.scheduler.current_task = (Task*) kernel.scheduler.current_task->list.next;
    if (!(kernel.scheduler.current_task->flags & TASK_PRESENT))
        kernel.scheduler.current_task = (Task*) kernel.scheduler.current_task->list.next;
    printf("Task select return task with pid = %i\n", ((Task*)kernel.scheduler.current_task)->pid);
    printf("off = %i\n", offsetof(Task, entry));
    return (Task*) kernel.scheduler.current_task;
}

// for asm context switch
Task *get_current_task() {
    return kernel.scheduler.current_task;
}
