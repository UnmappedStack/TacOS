#include <serial.h>
#include <slab.h>
#include <paging.h>
#include <panic.h>
#include <framebuffer.h>
#include <util.h>
#include <gdt.h>
#include <idt.h>
#include <pma.h>
#include <kprintf.h>
#include <kernel.h>

KernelInfo kernel_info = {0};

void _start(void) {
    serial_init();
    framebuffer_init();
    gdt_init();
    idt_init();
    exceptions_init();
    pma_init();
    pma_palloc();

    SWITCH_PAGE_TREE(create_address_space());
    kprintf("Page tree switched successfully\n");

    Cache *cache = cache_create(32);
    void *obj = slab_alloc(cache);
    kprintf("object: %x\n", obj);
    slab_free(cache, obj);
    obj = slab_alloc(cache);
    kprintf("object: %x\n", obj);

    FREEZE_DEVICE();
}
