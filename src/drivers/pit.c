#include <cpu.h>
#include <cpu/idt.h>
#include <io.h>
#include <kernel.h>
#include <pit.h>
#include <printf.h>

__attribute__((interrupt)) void decrement_pit_counter(void *) {
    kernel.pit_counter--;
    end_of_interrupt();
}

void pit_wait(uint64_t ms) {
    kernel.pit_counter =
        ms; // this is easy since one PIT tick is set to one millisecond
    unlock_pit();
    ENABLE_INTERRUPTS();
    while (kernel.pit_counter > 0 && kernel.pit_counter <= ms) IO_WAIT();
    DISABLE_INTERRUPTS();
    lock_pit();
    return;
}

void init_pit(void) {
    outb(0x43, 0b110100); // set mode to rate generator, channel 0,
                          // lobyte/hibyte, binary mode
    outb(0x40, (HERTZ_DIVIDER) & 0xFF);
    outb(0x40, (HERTZ_DIVIDER >> 8) & 0xFF);
    set_IDT_entry(32, (void *)&decrement_pit_counter, 0x8F, kernel.IDT);
    map_ioapic(32, 2, 0, POLARITY_HIGH, TRIGGER_EDGE);
    lock_pit();
    printf("Initialised PIT.\n");
}

void unlock_pit(void) { unmask_ioapic(2, 0); }

void lock_pit(void) { mask_ioapic(2, 0); }
