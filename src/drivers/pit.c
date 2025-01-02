#include <pit.h>
#include <cpu/idt.h>
#include <kernel.h>
#include <io.h>
#include <printf.h>

// TODO: Link this to the context switch - it's currently just a test interrupt
__attribute__((interrupt))
void pit_isr(void* unused) {
    (void) unused;
    printf("Got PIT interrupt.\n");
    end_of_interrupt();
}

void init_pit() {
    outb(0x43, 0b110100); // set mode to rate generator, channel 0, lobyte/hibyte, binary mode
    outb(0x40, (HERTZ_DIVIDER) & 0xFF);
    outb(0x40, (HERTZ_DIVIDER >> 8) & 0xFF);
    set_IDT_entry(32, &pit_isr, 0x8F, kernel.IDT);
    lock_pit();
    printf("Initialised PIT.\n");
}

void unlock_pit() {
    unmask_irq(0);
}

void lock_pit() {
    mask_irq(0);
}
