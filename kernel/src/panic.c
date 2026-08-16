/* This probably has more ISA-checking macros littered throughout than I'd like.
 * I'm not going to mark it as to-do however simply because it has not caused any
 * real issues besides slightly non-satisfying code. You can probably find a bunch of
 * old messages of me wondering on Discord how to do this better then just deciding to
 * do it this way lol. */

#include <kprintf.h>
#include <isa/cpu.h>
#include <kernel.h>
#include <smp.h>
#include <lock.h>
#include <util.h>
#include <stdint.h>
#include <assets.h>
#include <stddef.h>

#if defined(__x86_64__)
typedef struct {
    uint64_t cr2;
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t type;
    uint64_t code;
    uint64_t rip;
    uint64_t cs;
    uint64_t flags;
    uint64_t rsp;
    uint64_t ss;
} IDTEFrame;
#elif defined(__riscv)
typedef struct {
    uint64_t type;
    // TODO
} IDTEFrame;
#else
#error "define error frame for new ISA"
#endif

struct stack_frame {
    struct stack_frame *rbp;
    uint64_t rip;
};

static char *exceptions[] = {
    [0]  = "Division Error",
    [1]  = "Debug",
    [2]  = "Non-maskable Interrupt",
    [3]  = "Breakpoint",
    [4]  = "Overflow",
    [5]  = "Bound Range Exceeded",
    [6]  = "Invalid Opcode",
    [7]  = "Device Not Available",
    [8]  = "Double Fault",
    [9]  = "Coprocessor Segment Overrun",
    [10] = "Invalid TSS",
    [11] = "Segment Not Present",
    [12] = "Stack-Segment Fault",
    [13] = "General Protection Fault",
    [14] = "Page Fault",
    [16] = "x87 Floating-Point Exception",
    [17] = "Alignment Check",
    [18] = "Machine Check",
    [19] = "SIMD Floating-Point Exception",
    [20] = "Virtualization Exception",
    [21] = "Control Protection Exception",
    [28] = "Hypervisor Injection Exception",
    [29] = "VMM Communication Exception",
    [30] = "Security Exception",
};

/* if msg is null then it'll use whatever it finds from frame->type (mostly for exceptions),
   but if its set then it'll use it as the error message (for manual calls) */
Spinlock panic_lock = {0};
void panic_handler(const char *msg, IDTEFrame frame) {
    DISABLE_INTERRUPTS();
    kprintf("\n === KERNEL PANIC ENTERED === \n\n");
    spinlock_acquire(&panic_lock); // never released

    if (kernel_info.smp_enabled) halt_all_processors();

    uint64_t cr3;
    __asm__ volatile("movq %%cr3, %0" : "=r"(cr3));
    int i = 0;
#define ASCII_ART_LINE() kprintf(ascii_art[i++]);
#define ASCII_ART_NEWLINE() \
    do { \
        ASCII_ART_LINE(); \
        kprintf("\n"); \
    } while (0)
    const char *error_type;
    if (msg != NULL)
        error_type = msg;
    else if (frame.type <= 30 && !(frame.type < 28 && frame.type > 21) && frame.type != 15)
        error_type = exceptions[frame.type];
    else error_type = "Triple fault or unknown exception";
    for (int n = 0; n < 7; n++) ASCII_ART_NEWLINE();
    ASCII_ART_LINE(); kprintf(" WOAH! You messed this all up!\n");
    ASCII_ART_LINE(); kprintf(" This is ALL your fault. I take ZERO responsibility!\n");
    ASCII_ART_NEWLINE();
    int current_cpu = (kernel_info.smp_enabled) ? current_processor()->id : 0;
#if defined(__x86_64__)
    ASCII_ART_LINE(); kprintf(" Exception type: %s in ring %u\n", error_type, frame.ss & 0b11);
    ASCII_ART_LINE(); kprintf(" SS: %u, CS: %u, CPU%u\n", frame.ss, frame.cs, current_cpu);
    ASCII_ART_LINE(); kprintf(" Error code: %x\n", frame.code);
    ASCII_ART_NEWLINE();
    ASCII_ART_LINE(); kprintf(" Register dump:\n");
    ASCII_ART_LINE(); kprintf("  RAX: %x, RBX: %x\n", frame.rax, frame.rbx);
    ASCII_ART_LINE(); kprintf("  RCX: %x, RDX: %x\n", frame.rcx, frame.rdx);
    ASCII_ART_LINE(); kprintf("  RSI: %x, RDI: %x\n", frame.rsi, frame.rdi);
    ASCII_ART_LINE(); kprintf("   R8: %x,  R9: %x\n",  frame.r8, frame.r9);
    ASCII_ART_LINE(); kprintf("  RBP: %x, RSP: %x\n", frame.rbp, frame.rsp);
    ASCII_ART_LINE(); kprintf("  CR2: %x, CR3: %x\n", frame.cr2, cr3);
    ASCII_ART_NEWLINE();
    ASCII_ART_LINE(); kprintf(" Stack Trace (Most recent call last): \n");
    ASCII_ART_LINE(); kprintf("  -> %x\n", frame.rip);
    struct stack_frame *stack = (struct stack_frame*)frame.rbp;
    while (stack && stack->rip) {
        ASCII_ART_LINE(); kprintf("  -> %x\n", stack->rip);
        if (stack->rbp && stack->rbp->rip == stack->rip) {
            ASCII_ART_LINE(); kprintf(" ...recursive call\n");
            break;
        }
        stack = stack->rbp;
    }
#elif defined(__riscv)
    // TODO
    (void) current_cpu;
    (void) error_type;
#endif
    ASCII_ART_NEWLINE();
    if (msg != NULL) {
        ASCII_ART_LINE(); kprintf(" (Manually induced panic so registers may be null)\n");
    }
    while(i < (int)(sizeof(ascii_art)/sizeof(ascii_art[0]))) ASCII_ART_NEWLINE();
    FREEZE_DEVICE();
}

// this is pretty much a wrapper around panic_handler except it supports on-demand panics that
// don't stem from an exception.
void kpanic(const char *msg) {
    // we don't care about any of the frame data since this is a panic, not an exception...
    // *except* rbp+rip (for a stack trace) and cs+ss
    IDTEFrame frame = {0};
#if defined(__x86_64__)
    __asm__ volatile("movq %%rbp, %0" : "=r"(frame.rbp));
    __asm__ volatile("movq %%rbp, %0" : "=r"(frame.rip));
    __asm__ volatile("movq %%cs, %0" : "=r"(frame.cs));
    __asm__ volatile("movq %%ss, %0" : "=r"(frame.ss));
#endif
    panic_handler(msg, frame);
}
