#include <cpu.h>
#include <cpu/idt.h>
#include <kernel.h>
#include <panic.h>
#include <printf.h>

bool in_panic = false;

struct stackFrame {
    struct stackFrame *rbp;
    uint64_t rip;
};

void stack_trace(uint64_t rbp, uint64_t rip) {
    printf("\nStack Trace (Most recent call last): \n");
    printf(" 0x%x\n", rip);
    struct stackFrame *stack = (struct stackFrame *)rbp;
    while (stack) {
        printf(" 0x%x\n", stack->rip);
        if (stack->rbp->rip == stack->rip) {
            printf(" ...recursive call\n");
            return;
        }
        stack = stack->rbp;
    }
}
extern void sys_exit(int);
void exception_handler(IDTEFrame registers) {
    DISABLE_INTERRUPTS();
    if (in_panic)
        HALT_DEVICE();
    in_panic = true;
    printf(BYEL "                         ### KERNEL PANIC! ###\n" BRED
                "  Something went seriously wrong and the system cannot "
                "continue.\n\n" BWHT " === Debug information: ===\n" WHT);

    if (registers.type == 14)
        printf("Exception type: Page fault");
    else if (registers.type == 13)
        printf("Exception type: General protection fault");
    else
        printf("Exception type: %i", registers.type);
    printf(" in task of PID=%i\n", kernel.scheduler.current_task->pid);
    size_t cr3;
    __asm__ volatile("movq %%cr3, %0" : "=r"(cr3));
    printf("Error code: 0b%b\n\n", registers.code);
    printf("CR2: 0x%p\n\n", registers.cr2);
    printf("RSP: 0x%p | RAX: 0x%p\n", registers.rsp, registers.rax);
    printf("RBP: 0x%p | RBX: 0x%p\n", registers.rbp, registers.rbx);
    printf("CR2: 0x%p | RCX: 0x%p\n", registers.cr2, registers.rcx);
    printf("RDI: 0x%p | RDX: 0x%p\n", registers.rdi, registers.rdx);
    printf("RSI: 0x%p | CR3: 0x%p\n", registers.rsi, cr3);
    printf(" R8: 0x%p |  R9: 0x%p\n", registers.r8, registers.r9);
    if ((registers.ss & 0b11) != (registers.cs & 0b11)) {
        printf("SS and CS ring levels do not match up.\n");
    } else {
        printf("Exception occurred in ring %i\n", registers.ss & 0b11);
    }
    printf("SS w/o ring: %x\n", registers.ss & ~3);
    printf("CS w/o ring: %x\n", registers.cs & ~3);
    stack_trace(registers.rbp, registers.rip);
    if (kernel.scheduler.current_task->pid > 0) {
        write_framebuffer_text("\nSegmentation fault\n");
        in_panic = false;
        sys_exit(139);
    }
    printf(BWHT "\nFreezing the computer now. Please reboot your machine with "
                "the physical power button.\n" WHT);
    HALT_DEVICE();
}

void init_exceptions(void) {
    set_IDT_entry(0, (void *)&divideException, 0x8E, kernel.IDT);
    set_IDT_entry(1, (void *)&debugException, 0x8E, kernel.IDT);
    set_IDT_entry(3, (void *)&breakpointException, 0x8E, kernel.IDT);
    set_IDT_entry(4, (void *)&overflowException, 0x8E, kernel.IDT);
    set_IDT_entry(5, (void *)&boundRangeExceededException, 0x8E, kernel.IDT);
    set_IDT_entry(6, (void *)&invalidOpcodeException, 0x8E, kernel.IDT);
    set_IDT_entry(7, (void *)&deviceNotAvaliableException, 0x8E, kernel.IDT);
    set_IDT_entry(8, (void *)&doubleFaultException, 0x8E, kernel.IDT);
    set_IDT_entry(9, (void *)&coprocessorSegmentOverrunException, 0x8E,
                  kernel.IDT);
    set_IDT_entry(10, (void *)&invalidTSSException, 0x8E, kernel.IDT);
    set_IDT_entry(11, (void *)&segmentNotPresentException, 0x8E, kernel.IDT);
    set_IDT_entry(12, (void *)&stackSegmentFaultException, 0x8E, kernel.IDT);
    set_IDT_entry(13, (void *)&generalProtectionFaultException, 0x8E,
                  kernel.IDT);
    set_IDT_entry(14, (void *)&pageFaultException, 0x8E, kernel.IDT);
    set_IDT_entry(16, (void *)&floatingPointException, 0x8E, kernel.IDT);
    set_IDT_entry(17, (void *)&alignmentCheckException, 0x8E, kernel.IDT);
    set_IDT_entry(18, (void *)&machineCheckException, 0x8E, kernel.IDT);
    set_IDT_entry(19, (void *)&simdFloatingPointException, 0x8E, kernel.IDT);
    set_IDT_entry(20, (void *)&virtualisationException, 0x8E, kernel.IDT);
}
