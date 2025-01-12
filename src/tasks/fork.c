#include <fork.h>
#include <scheduler.h>
#include <printf.h>
#include <kernel.h>

pid_t fork() {
    bool found = false;
    Task *initial_task = kernel.scheduler.current_task;
    Task *new_task     = task_add();
    memcpy(new_task->resources, initial_task->resources, sizeof(new_task->resources));
    new_task->entry    = kernel.scheduler.current_task->entry;
    new_task->parent   = kernel.scheduler.current_task;
    new_task->pml4     = (uint64_t) init_paging_task();
    printf("new pml4 = %p\n", new_task->pml4);
    new_task->rsp      = kernel.scheduler.current_task->rsp;
    bool is_first      = true;
    if (!kernel.scheduler.current_task->memregion_list) {
        printf("Skipped\n");
        goto endcopy;
    }
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
    endcopy:
    for (size_t i = 0; i < MAX_CHILDREN; i++) {
        if (!initial_task->children[i].pid) {
            found = true;
            initial_task->children[i] = (Child) {
                .pid = new_task->pid,
                .status = 0,
            };
            break;
        }
    }
    if (!found) {
        printf("Couldn't fork, task already has too many children.\n");
        return 0;
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
