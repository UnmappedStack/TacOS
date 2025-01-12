#include <tss.h>
#include <printf.h>
#include <mem/paging.h>
#include <kernel.h>
#include <mem/pmm.h>

void init_TSS() {
    kernel.tss = (TSS*) (kmalloc(1) + kernel.hhdm);
    kernel.tss->rsp0 = KERNEL_STACK_PTR;
    printf("Initiated TSS.\n");
}
