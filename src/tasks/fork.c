#include <fork.h>
#include <mem/pmm.h>
#include <cpu.h>
#include <scheduler.h>
#include <printf.h>
#include <kernel.h>

void push_gprs_in_task(Task *task, uint64_t new_task_rsp) {
    size_t rax, rbx, rcx, rdx;
    size_t rsi, rdi, rbp;
    size_t r9, r10, r11, r12, r13, r14, r15;
    __asm__ volatile("mov %%rax, %0" : "=r" (rax));
    __asm__ volatile("mov %%rbx, %0" : "=r" (rbx));
    __asm__ volatile("mov %%rcx, %0" : "=r" (rcx));
    __asm__ volatile("mov %%rdx, %0" : "=r" (rdx));
    __asm__ volatile("mov %%rsi, %0" : "=r" (rsi));
    __asm__ volatile("mov %%rdi, %0" : "=r" (rdi));
    __asm__ volatile("mov %%rbp, %0" : "=r" (rbp));
    __asm__ volatile("mov %%r9,  %0" : "=r" (r9));
    __asm__ volatile("mov %%r10, %0" : "=r" (r10));
    __asm__ volatile("mov %%r11, %0" : "=r" (r11));
    __asm__ volatile("mov %%r12, %0" : "=r" (r12));
    __asm__ volatile("mov %%r13, %0" : "=r" (r13));
    __asm__ volatile("mov %%r14, %0" : "=r" (r14));
    __asm__ volatile("mov %%r15, %0" : "=r" (r15));
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
        push_gprs_in_task(new_task, new_task_rsp - 5 * 8);
    }
    // copy the stack over
    new_task->flags = kernel.scheduler.current_task->flags; /* Flags are set last so that it's only 
                                                             * ever run after everything else is set up
                                                             * (because of the TASK_PRESENT flag) */
    return new_task->pid;
}
