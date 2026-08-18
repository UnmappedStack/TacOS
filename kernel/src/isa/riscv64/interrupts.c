#include <isa/cpu.h>
#include <stdint.h>
#include <kprintf.h>

// normal interrupts
#define INTERRUPT_EBREAK 3

// exceptions
#define EXCEPTION_ADDRESS_MISALIGNED       0
#define EXCEPTION_INSTRUCTION_ACCESS_FAULT 1
#define EXCEPTION_ILLEGAL_INSTRUCTION      2
#define EXCEPTION_LOAD_ADDRESS_MISALIGNED  4
#define EXCEPTION_STORE_AMO_ACCESS_FAULT   7
#define EXCEPTION_INSTRUCTION_PAGE_FAULT   12
#define EXCEPTION_LOAD_PAGE_FAULT          13
#define EXCEPTION_STORE_AMO_PAGE_FAULT     15
#define EXCEPTION_SOFTWARE_CHECK           18
#define EXCEPTION_HARDWARE_ERROR           19

// a copy of this is defined in interrupt.S
typedef struct {
    uint64_t return_addr;
    uint64_t cause, val;
    uint64_t regs[31];
} InterruptStackFrame;

void interrupt_handler(InterruptStackFrame *frame) {
    switch (frame->cause) {
    case INTERRUPT_EBREAK:
        kprintf("\n   > EBREAK -> scause=%x\n", frame->cause);
        /* so basically we have to check if the instruction at the return
         * address is aligned. if it is then just increment it by 16 bytes,
         * otherwise by a full 32 bytes. then we can just return to the
         * assembly caller. It is compressed if the low 2 bits are not set. */
        bool is_compressed = (*((uint16_t*)frame->return_addr) & 0b11) != 0b11;
        uint64_t instruction_size = (is_compressed) ? 2 : 4;
        frame->return_addr += instruction_size;
        kprintf("   > %s, instruction increment %u\n\n",
                (is_compressed) ? "compressed" : "non-compressed", instruction_size);
        break;
    case EXCEPTION_ADDRESS_MISALIGNED:
    case EXCEPTION_INSTRUCTION_ACCESS_FAULT:
    case EXCEPTION_ILLEGAL_INSTRUCTION:
    case EXCEPTION_LOAD_ADDRESS_MISALIGNED:
    case EXCEPTION_STORE_AMO_ACCESS_FAULT:
    case EXCEPTION_INSTRUCTION_PAGE_FAULT:
    case EXCEPTION_LOAD_PAGE_FAULT:
    case EXCEPTION_STORE_AMO_PAGE_FAULT:
    case EXCEPTION_SOFTWARE_CHECK:
    case EXCEPTION_HARDWARE_ERROR:
        kprintf("kernel panic uh oh %u\n", frame->cause);
        FREEZE_DEVICE();
    default:
        kprintf("   > unhandled interrupt (probably an exception), freeze this cpu (TODO handle this properly)\n");
        FREEZE_DEVICE();
    }
}

extern void prepare_for_interrupt(void); // asm handler, will call interrupt_handler() as defined above
void interrupts_init(void) {
    csr_write(CSR_REG_STVEC, (uintptr_t)&prepare_for_interrupt);
    __asm__ volatile("ebreak");
    char *ptr = (void*)2;
    *ptr = 'h';
    kprintf(ptr);
}
