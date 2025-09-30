#include <kernel.h>
#include <mem/paging.h>
#include <mem/pmm.h>
#include <tss.h>

TSS *init_TSS(uintptr_t kernel_rsp) {
    TSS *tss = (TSS *)(kmalloc(1) + kernel.hhdm);
    tss->rsp0 = kernel_rsp;
    return tss;
}
