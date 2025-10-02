#include <cpu.h>
#include <spinlock.h>
#include <smp.h>
#include <fork.h>
#include <kernel.h>
#include <mem/pmm.h>
#include <printf.h>
#include <scheduler.h>

void push_gprs_in_task(Task *task, uint64_t new_task_rsp, void *callframe) {
    printf("gprs from 0x%p to 0x%p\n", callframe, new_task_rsp);
    memcpy((void *)(new_task_rsp - 8 * 15),
           (void *)((uintptr_t)callframe - 8 * 14), 8 * 14);
    *((uint64_t *)(new_task_rsp - 8)) = 0; // clear rax
    task->rsp -= 8 * 15;
}

extern Spinlock scheduler_lock;
pid_t fork(CallFrame *callframe) {
    DISABLE_INTERRUPTS();
    bool found = false;
    Task *initial_task = CURRENT_TASK;
    Task *new_task = task_add();
    memcpy(new_task->resources, initial_task->resources,
           sizeof(new_task->resources));
    memcpy(new_task->cwd, initial_task->cwd, MAX_PATH_LEN);
    memset(new_task->children, 0, sizeof(new_task->children));
    new_task->entry = CURRENT_TASK->entry;
    new_task->parent = CURRENT_TASK;
    new_task->pml4 = (uint64_t)init_paging_task();
    new_task->rsp = KERNEL_STACK_PTR;
    bool is_first = true;
    uintptr_t mem = kmalloc(KERNEL_STACK_PAGES);
    map_pages((uint64_t *)(new_task->pml4 + kernel.hhdm), KERNEL_STACK_ADDR,
              mem, KERNEL_STACK_PAGES,
              KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_WRITE);
    if (!CURRENT_TASK->memregion_list)
        goto endcopy;
    for (struct list *iter =
             &CURRENT_TASK->memregion_list->list;
         iter != &CURRENT_TASK->memregion_list->list ||
         is_first;
         iter = iter->next) {
        is_first = false;
        memregion_clone((Memregion *)iter,
                        CURRENT_TASK->pml4 + kernel.hhdm,
                        new_task->pml4 + kernel.hhdm);
    }
endcopy:
    for (size_t i = 0; i < MAX_CHILDREN; i++) {
        if (!initial_task->children[i].pid) {
            found = true;
            initial_task->children[i] = (Child){
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
    __asm__ volatile("movq %%rsp, %0" : "=r"(rsp));
    uint64_t new_task_rsp =
        virt_to_phys((uint64_t *)(new_task->pml4 + kernel.hhdm),
                     KERNEL_STACK_ADDR) +
        kernel.hhdm + KERNEL_STACK_PAGES * PAGE_SIZE;
    if (new_task_rsp == 0xDEAD + kernel.hhdm + KERNEL_STACK_PAGES * PAGE_SIZE) {
        printf("\nFork failed, couldn't find of new address space\n");
        HALT_DEVICE();
    }
    if (callframe) {
        *((uint64_t *)(new_task_rsp - 8)) = 0x20 | 3;
        *((uint64_t *)(new_task_rsp - 16)) = callframe->rsp;
        *((uint64_t *)(new_task_rsp - 24)) = callframe->rflags;
        *((uint64_t *)(new_task_rsp - 32)) = 0x18 | 3;
        *((uint64_t *)(new_task_rsp - 40)) = (uint64_t)callframe->rip;
        new_task->rsp -= 5 * 8;
        printf(" -> new task rsp before = 0x%p\n", new_task->rsp);
        push_gprs_in_task(new_task, new_task_rsp - 5 * 8, callframe);
        printf(" -> new task rsp after =  0x%p\n", new_task->rsp);
    }
    map_apic_into_task(new_task->pml4);
     /* Flags are set last so that it's only
      * ever run after everything else is set up
      * (because of the TASK_PRESENT flag) */
    new_task->flags = CURRENT_TASK->flags
                        & ~TASK_RUNNING;
    return new_task->pid;
}
