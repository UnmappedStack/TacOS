#include <pit.h>
#include <cpu/idt.h>
#include <kernel.h>
#include <io.h>
#include <printf.h>

extern void context_switch();

void init_pit() {
    outb(0x43, 0b110100); // set mode to rate generator, channel 0, lobyte/hibyte, binary mode
    outb(0x40, (HERTZ_DIVIDER) & 0xFF);
    outb(0x40, (HERTZ_DIVIDER >> 8) & 0xFF);
    set_IDT_entry(32, &context_switch, 0x8F, kernel.IDT);
    lock_pit();
    printf("Initialised PIT.\n");
}

void unlock_pit() {
    unmask_irq(0);
}

void lock_pit() {
    mask_irq(0);
}
