#include <fork.h>
#include <printf.h>
#include <kernel.h>

pid_t fork() {
    const Task *initial_task = kernel.scheduler.current_task;
    Task *new_task     = task_add();
    new_task->entry    = kernel.scheduler.current_task->entry;
    new_task->parent   = kernel.scheduler.current_task;
    new_task->pml4     = (uint64_t) init_paging_task();
    bool is_first      = true;
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
    new_task->flags = kernel.scheduler.current_task->flags; /* Flags are set last so that it's only 
                                                             * ever run after everything else is set up
                                                             * (because of the TASK_PRESENT flag) */
    // TODO: Remove debug messages once everything is *definitely* working
    if (kernel.scheduler.current_task->pid == initial_task->pid) {
        printf("Parent\n");
        return new_task->pid;
    } else if (kernel.scheduler.current_task->pid == new_task->pid) {
        printf("Child\n");
        return 0;
    } else {
        printf("Weird as fuck error in fork(), wtf??? (supposed to be unreachable but theoretically reachable)\n");
        return 0;
    }
}
