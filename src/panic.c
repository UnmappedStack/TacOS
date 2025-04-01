#include <panic.h>
#include <cpu.h>
#include <cpu/idt.h>
#include <kernel.h>
#include <printf.h>

bool in_panic = false;

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
    DISABLE_INTERRUPTS();
    if (in_panic) HALT_DEVICE();
    in_panic = true;
    printf(BYEL "                         ### KERNEL PANIC! ###\n"
           BRED "  Something went seriously wrong and the system cannot continue.\n\n"
           BWHT  " === Debug information: ===\n" WHT);

    if (registers.type == 14)
        printf("Exception type: Page fault\n");
    else if (registers.type == 13)
        printf("Exception type: General protection fault\n");
    else
        printf("Exception type: %i\n", registers.type);
    
    printf("Error code: 0b%b\n\n", registers.code);
    printf("CR2: 0x%p\n\n", registers.cr2);
    printf("RSP: 0x%p | RAX: 0x%p\n", registers.rsp, registers.rax);
    printf("RBP: 0x%p | RBX: 0x%p\n", registers.rbp, registers.rbx);
    printf("CR2: 0x%p | RCX: 0x%p\n", registers.cr2, registers.rcx);
    printf("RDI: 0x%p | RDX: 0x%p\n", registers.rdi, registers.rdx);
    printf("RSI: 0x%p            \n", registers.rsi               );
    if ((registers.ss & 0b11) != (registers.cs & 0b11)) {
        printf("SS and CS ring levels do not match up.\n");
    } else {
        printf("Exception occurred in ring %i\n", registers.ss & 0b11);
    }
    stack_trace(registers.rbp, registers.rip);
    printf(BWHT "\nFreezing the computer now. Please reboot your machine with the physical power button.\n" WHT);
    HALT_DEVICE();
}

void init_exceptions() {
    set_IDT_entry(0, &divideException, 0xEF, kernel.IDT);
    set_IDT_entry(1, &debugException, 0xEF, kernel.IDT);
    set_IDT_entry(3, &breakpointException, 0xEF, kernel.IDT);
    set_IDT_entry(4, &overflowException, 0xEF, kernel.IDT);
    set_IDT_entry(5, &boundRangeExceededException, 0xEF, kernel.IDT);
    set_IDT_entry(6, &invalidOpcodeException, 0xEF, kernel.IDT);
    set_IDT_entry(7, &deviceNotAvaliableException, 0xEF, kernel.IDT);
    set_IDT_entry(8, &doubleFaultException, 0xEF, kernel.IDT);
    set_IDT_entry(9, &coprocessorSegmentOverrunException, 0xEF, kernel.IDT);
    set_IDT_entry(10, &invalidTSSException, 0xEF, kernel.IDT);
    set_IDT_entry(11, &segmentNotPresentException, 0xEF, kernel.IDT);
    set_IDT_entry(12, &stackSegmentFaultException, 0xEF, kernel.IDT);
    set_IDT_entry(13, &generalProtectionFaultException, 0xEF, kernel.IDT);
    set_IDT_entry(14, &pageFaultException, 0xEF, kernel.IDT);
    set_IDT_entry(16, &floatingPointException, 0xEF, kernel.IDT);
    set_IDT_entry(17, &alignmentCheckException, 0xEF, kernel.IDT);
    set_IDT_entry(18, &machineCheckException, 0xEF, kernel.IDT);
    set_IDT_entry(19, &simdFloatingPointException, 0xEF, kernel.IDT);
    set_IDT_entry(20, &virtualisationException, 0xEF, kernel.IDT);
}
