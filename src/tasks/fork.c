#include <fork.h>
#include <mem/pmm.h>
#include <cpu.h>
#include <scheduler.h>
#include <printf.h>
#include <kernel.h>

void push_gprs_in_task(Task *task, uint64_t new_task_rsp, void *callframe) {
    memcpy((void*) (new_task_rsp - 112), (void*) ((uintptr_t) callframe - 120), 128);
    task->rsp -= 15 * 8;
}

pid_t fork(CallFrame *callframe) {
    bool found = false;
    Task *initial_task = kernel.scheduler.current_task;
    Task *new_task     = task_add();
    memcpy(new_task->resources, initial_task->resources, sizeof(new_task->resources));
    memset(new_task->children, 0, sizeof(new_task->children));
    new_task->entry    = kernel.scheduler.current_task->entry;
    new_task->parent   = kernel.scheduler.current_task;
    new_task->pml4     = (uint64_t) init_paging_task();
    new_task->rsp      = kernel.scheduler.current_task->rsp;
    bool is_first      = true;
    uintptr_t mem = kmalloc(KERNEL_STACK_PAGES);
    map_pages((uint64_t*) (new_task->pml4 + kernel.hhdm), KERNEL_STACK_ADDR, mem, KERNEL_STACK_PAGES, KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_WRITE);
    if (!kernel.scheduler.current_task->memregion_list) goto endcopy;
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
        printf("\nCouldn't fork, task already has too many children.\n");
        HALT_DEVICE();
    }
    size_t rsp;
    __asm__ volatile("movq %%rsp, %0" : "=r" (rsp));
    uint64_t new_task_rsp = virt_to_phys(
            (uint64_t*) (new_task->pml4 + kernel.hhdm), KERNEL_STACK_ADDR
        ) + kernel.hhdm + KERNEL_STACK_PAGES * PAGE_SIZE;
    if (new_task_rsp == 0xDEAD + kernel.hhdm + KERNEL_STACK_PAGES * PAGE_SIZE) {
        printf("\nFork failed, couldn't find of new address space\n");
        HALT_DEVICE();
    }
    if (callframe) {
        *((uint64_t*) (new_task_rsp -  8)) = 0x20 | 3;
        *((uint64_t*) (new_task_rsp - 16)) = callframe->rsp;
        *((uint64_t*) (new_task_rsp - 24)) = callframe->rflags;
        *((uint64_t*) (new_task_rsp - 32)) = 0x18 | 3;
        *((uint64_t*) (new_task_rsp - 40)) = (uint64_t) callframe->rip;
        new_task->rsp -= 5 * 8;
        push_gprs_in_task(new_task, new_task_rsp - 5 * 8, callframe);
    }
    // copy the stack over
    new_task->flags = kernel.scheduler.current_task->flags; /* Flags are set last so that it's only 
                                                             * ever run after everything else is set up
                                                             * (because of the TASK_PRESENT flag) */
    return new_task->pid;
}
