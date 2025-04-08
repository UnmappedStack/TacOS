#include <fork.h>
#include <scheduler.h>
#include <printf.h>
#include <kernel.h>

void push_gprs_in_task(Task *task, uint64_t new_task_rsp) {
    size_t rax, rbx, rcx, rdx;
    size_t rsi, rdi, rbp;
    size_t r9, r10, r11, r12, r13, r14, r15;
    asm volatile("mov %%rax, %0" : "=r" (rax));
    asm volatile("mov %%rbx, %0" : "=r" (rbx));
    asm volatile("mov %%rcx, %0" : "=r" (rcx));
    asm volatile("mov %%rdx, %0" : "=r" (rdx));
    asm volatile("mov %%rsi, %0" : "=r" (rsi));
    asm volatile("mov %%rdi, %0" : "=r" (rdi));
    asm volatile("mov %%rbp, %0" : "=r" (rbp));
    asm volatile("mov %%r9,  %0" : "=r" (r9));
    asm volatile("mov %%r10, %0" : "=r" (r10));
    asm volatile("mov %%r11, %0" : "=r" (r11));
    asm volatile("mov %%r12, %0" : "=r" (r12));
    asm volatile("mov %%r13, %0" : "=r" (r13));
    asm volatile("mov %%r14, %0" : "=r" (r14));
    asm volatile("mov %%r15, %0" : "=r" (r15));
    *((uint64_t*) (new_task_rsp - 8))   = rax;
    *((uint64_t*) (new_task_rsp - 16))  = rbx;
    *((uint64_t*) (new_task_rsp - 24))  = rcx;
    *((uint64_t*) (new_task_rsp - 32))  = rdx;
    *((uint64_t*) (new_task_rsp - 40))  = rsi;
    *((uint64_t*) (new_task_rsp - 48))  = rdi;
    *((uint64_t*) (new_task_rsp - 56))  = rbp;
    *((uint64_t*) (new_task_rsp - 64))  = r9;
    *((uint64_t*) (new_task_rsp - 72))  = r10;
    *((uint64_t*) (new_task_rsp - 80))  = r11;
    *((uint64_t*) (new_task_rsp - 88))  = r12;
    *((uint64_t*) (new_task_rsp - 96))  = r13;
    *((uint64_t*) (new_task_rsp - 104)) = r14;
    *((uint64_t*) (new_task_rsp - 112)) = r15;
    task->rsp -= 15 * 8;
}

pid_t fork(void) {
    printf("\n === IN FORK ===\n\n");
    bool found = false;
    Task *initial_task = kernel.scheduler.current_task;
    Task *new_task     = task_add();
    printf(" -> fork() inherits resources\n");
    memcpy(new_task->resources, initial_task->resources, sizeof(new_task->resources));
    printf(" -> fork() empties child list\n");
    memset(new_task->children, 0, sizeof(new_task->children));
    printf(" -> fork() sets other fields\n");
    new_task->entry    = kernel.scheduler.current_task->entry;
    new_task->parent   = kernel.scheduler.current_task;
    new_task->pml4     = (uint64_t) init_paging_task();
    printf("new pml4 = %p\n", new_task->pml4);
    new_task->rsp      = kernel.scheduler.current_task->rsp;
    bool is_first      = true;
    alloc_pages((uint64_t*) (new_task->pml4 + kernel.hhdm), KERNEL_STACK_ADDR, KERNEL_STACK_PAGES, KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_WRITE);
    printf("Mapped stack\n");
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
    uint64_t new_task_rsp = virt_to_phys(
            (uint64_t*) (new_task->pml4 + kernel.hhdm), KERNEL_STACK_PTR
        ) + kernel.hhdm;
    size_t rsp;
    asm volatile("movq %%rsp, %0" : "=r" (rsp));
    printf("pml4 = %p, rsp = %p\n", new_task->pml4, new_task->rsp);
    *((uint64_t*) (new_task_rsp -  8)) = 0x20 | 3;
    *((uint64_t*) (new_task_rsp - 16)) = rsp;
    *((uint64_t*) (new_task_rsp - 24)) = 0x200;
    *((uint64_t*) (new_task_rsp - 32)) = 0x18 | 3;
    *((uint64_t*) (new_task_rsp - 40)) = (uint64_t) &&new_task_starts_here;
    new_task->rsp -= 5 * 8;
    push_gprs_in_task(new_task, new_task_rsp);
    printf("\n === SETTING FLAGS ===\n\n");
    new_task->flags = kernel.scheduler.current_task->flags; /* Flags are set last so that it's only 
                                                             * ever run after everything else is set up
                                                             * (because of the TASK_PRESENT flag) */
new_task_starts_here:
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
