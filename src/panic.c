#include <panic.h>
#include <cpu.h>
#include <cpu/idt.h>
#include <kernel.h>
#include <printf.h>

struct stackFrame {
    struct stackFrame* rbp;
    uint64_t rip;
};

void stack_trace(uint64_t rbp, uint64_t rip) {
    printf("\nStack Trace (Most recent call last): \n");
    printf(" 0x%x\n", rip);
    struct stackFrame *stack = (struct stackFrame*) rbp;
    while (stack) {
        printf(" 0x%x\n", stack->rip);
        stack = stack->rbp;
    }
}

void exception_handler(IDTEFrame registers) {
    (void) registers;
    DISABLE_INTERRUPTS();
    printf(BYEL "                         ### KERNEL PANIC! ###\n"
           BRED "  Something went seriously wrong and the system cannot continue.\n\n"
           BWHT  " === Debug information: ===\n" WHT);

    if (registers.type == 14)
        printf("Exception type: Page fault\n");
    else if (registers.type == 13)
        printf("Exception type: General protection fault\n");
    else
        printf("Exception type: %i\n", registers.type);
    
    printf("Error code: %i\n", registers.code);
    stack_trace(registers.rbp, registers.rip);
    printf(BWHT "\nFreezing the computer now. Please reboot your machine with the physical power button.\n" WHT);
    HALT_DEVICE();
}

void init_exceptions() {
    set_IDT_entry(0, &divideException, 0x8E, kernel.IDT);
    set_IDT_entry(1, &debugException, 0x8E, kernel.IDT);
    set_IDT_entry(3, &breakpointException, 0x8E, kernel.IDT);
    set_IDT_entry(4, &overflowException, 0x8E, kernel.IDT);
    set_IDT_entry(5, &boundRangeExceededException, 0x8E, kernel.IDT);
    set_IDT_entry(6, &invalidOpcodeException, 0x8E, kernel.IDT);
    set_IDT_entry(7, &deviceNotAvaliableException, 0x8E, kernel.IDT);
    set_IDT_entry(8, &doubleFaultException, 0x8E, kernel.IDT);
    set_IDT_entry(9, &coprocessorSegmentOverrunException, 0x8E, kernel.IDT);
    set_IDT_entry(10, &invalidTSSException, 0x8E, kernel.IDT);
    set_IDT_entry(11, &segmentNotPresentException, 0x8E, kernel.IDT);
    set_IDT_entry(12, &stackSegmentFaultException, 0x8E, kernel.IDT);
    set_IDT_entry(13, &generalProtectionFaultException, 0x8E, kernel.IDT);
    set_IDT_entry(14, &pageFaultException, 0x8E, kernel.IDT);
    set_IDT_entry(16, &floatingPointException, 0x8E, kernel.IDT);
    set_IDT_entry(17, &alignmentCheckException, 0x8E, kernel.IDT);
    set_IDT_entry(18, &machineCheckException, 0x8E, kernel.IDT);
    set_IDT_entry(19, &simdFloatingPointException, 0x8E, kernel.IDT);
    set_IDT_entry(20, &virtualisationException, 0x8E, kernel.IDT);
}
