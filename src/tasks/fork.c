#include <fork.h>
#include <printf.h>
#include <kernel.h>

pid_t fork() {
    Task *new_task   = task_add();
    new_task->entry  = kernel.scheduler.current_task->entry;
    new_task->parent = kernel.scheduler.current_task;
    new_task->flags  = kernel.scheduler.current_task->flags;
    new_task->pml4   = (uint64_t) init_paging_task();
    bool is_first    = true;
    for (struct list *iter = &kernel.scheduler.current_task->memregion_list->list;
         iter != &kernel.scheduler.current_task->memregion_list->list || is_first;
         iter = iter->next
    ) {
        is_first = false;
        memregion_clone((Memregion*) iter,
                        kernel.scheduler.current_task->pml4 + kernel.hhdm,
                        new_task->pml4 + kernel.hhdm
        );
    }
    return new_task->pid;
}
