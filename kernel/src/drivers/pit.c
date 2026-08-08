// pretty much the pit's sole purpose is to calibrate the lapic timer

#include <isa/cpu.h>
#include <io.h>
#include <kernel.h>
#include <pit.h>
#include <kprintf.h>
#include <util.h>

__attribute__((interrupt)) void decrement_pit_counter(void *) {
    kernel_info.pit_counter--;
    end_of_interrupt();
}

void pit_wait(uint64_t ms) {
    kernel_info.pit_counter =
        ms; // this is easy since one PIT tick is set to one millisecond
    unlock_pit();
    ENABLE_INTERRUPTS();
    while (kernel_info.pit_counter > 0 && kernel_info.pit_counter <= ms) IO_WAIT();
    DISABLE_INTERRUPTS();
    lock_pit();
    return;
}

void pit_init(void) {
    outb(0x43, 0b110100); // set mode to rate generator, channel 0,
                          // lobyte/hibyte, binary mode
    outb(0x40, (HERTZ_DIVIDER) & 0xFF);
    outb(0x40, (HERTZ_DIVIDER >> 8) & 0xFF);
    kernel_info.idt[32] = idt_descriptor((uint64_t)&decrement_pit_counter, 8, 0x8E);
    map_ioapic(32, 2, 0, POLARITY_HIGH, TRIGGER_EDGE);
    lock_pit();
    kprintf("PIT init OK\n");
}

void unlock_pit(void) { unmask_ioapic(2, 0); }

void lock_pit(void) { mask_ioapic(2, 0); }
