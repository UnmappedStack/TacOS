#include <serial.h>
#include <framebuffer.h>
#include <util.h>
#include <gdt.h>
#include <idt.h>
#include <kprintf.h>

void _start(void) {
    serial_init();
    kprintf("this is a kprintf. char=%c, num=%u, str=%s, hex=%x\n", 'A', 69, "hi", 0x69);
    framebuffer_init();
    fill_framebuffer(0, 0xff0000);
    fill_framebuffer(1, 0x0000ff);
    gdt_init();
    idt_init();
    exceptions_init();
    char *ptr = (void*)0x0;
    *ptr = 1;
    FREEZE_DEVICE();
}
