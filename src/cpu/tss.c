#include <kernel.h>
#include <mem/paging.h>
#include <mem/pmm.h>
#include <printf.h>
#include <tss.h>

void init_TSS(void) {
    kernel.tss = (TSS *)(kmalloc(1) + kernel.hhdm);
    kernel.tss->rsp0 = KERNEL_STACK_PTR;
    printf("Initiated TSS.\n");
}
