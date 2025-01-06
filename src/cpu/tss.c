#include <tss.h>
#include <printf.h>
#include <mem/paging.h>
#include <kernel.h>

void init_TSS() {
    kernel.tss.rsp0 = KERNEL_STACK_PTR;
    printf("Initiated TSS.\n");
}
