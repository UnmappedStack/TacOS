#include <isa/cpu.h>
#include <stdint.h>
#include <kprintf.h>

__attribute__((optimize("align-functions=4")))
void interrupt_handler(void) {
    kprintf("hi from an interrupt\n");
    FREEZE_DEVICE();
}

void interrupts_init(void) {
    csr_write(CSR_REG_STVEC, (uintptr_t)&interrupt_handler);
    __asm__ volatile("ebreak");
}
