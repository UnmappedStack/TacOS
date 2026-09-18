// x86-64 context switching. this is quite a hotpath so keep this zoomy zoomy fast fast

#include <exposed_structs.S>
#define THREAD_FLAG_FIRST_SWITCH 0b100

.global context_switch

.macro PUSH_ALL_REGS
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %r9 
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    pushq %rbp
    pushq %rsi
    pushq %rdi
.endm // PUSH_ALL_REGS

.macro POP_ALL_REGS
    popq %rdi
    popq %rsi
    popq %rbp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9 
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
.endm // POP_ALL_REGS

.macro CLEAR_ALL_REGS
    xorq %rdi, %rdi
    xorq %rsi, %rsi 
    xorq %rbp, %rbp
    xorq %r15, %r15
    xorq %r14, %r14
    xorq %r13, %r13
    xorq %r12, %r12
    xorq %r11, %r11
    xorq %r10, %r10
    xorq %r9 , %r9
    xorq %rdx, %rdx
    xorq %rcx, %rcx
    xorq %rbx, %rbx
    xorq %rax, %rax
.endm // CLEAR_ALL_REGS

.section .text

/* saves the context of the current thread and enters the context of the next
 * thread. can be called by either yield or the timer for preemption.
 *
 * arguments:
 *   -> rdi: current thread
 *   -> rsi: next thread
 */ 
context_switch:
    // would be a race condition to have preemptions while switching
    cli

    // put current & next threads on the stack to use after
    pushq %rdi // current thread
    pushq %rsi // next thread

    // save state
    PUSH_ALL_REGS

    // the saved registers are 112 bytes (14 regs * 8 byte alignment)
    // so we can get back the current & next threads from before that
    movq 112(%rsp), %rdx // rdx = next thread, +8 bytes for rsi
    movq 120(%rsp), %rsi // rsp = current thread, +16 bytes for rdi
    movq THREAD_KERNEL_STACK_OFFSET(%rdx), %rcx // rcx = rsp
    movq THREAD_ENTRY_POINT_OFFSET(%rdx), %r8   // r8  = entry point
    movq THREAD_FLAGS_OFFSET(%rdx), %r9         // r9  = flags

    // if rsi (previous thread) is not null then save rsp
    testq %rsi, %rsi
    jz dont_save_rsp
    movq %rsp, THREAD_KERNEL_STACK_OFFSET(%rsi)
dont_save_rsp:

    // if its the first run then it needs to construct a return frame,
    // otherwise we can just use the existing one
    andq $THREAD_FLAG_FIRST_SWITCH, %r9
    jz .previously_switched

.first_switch:
    
    // first clear the first switch flag
    // r11 = ~THREAD_FLAG_FIRST_SWITCH
    movq $THREAD_FLAG_FIRST_SWITCH, %r11
    notq %r11
    // thread_flags &= r11
    // (r9 already contains the flags)
    andq %r11, %r9
    movq THREAD_FLAGS_OFFSET(%rdx), %r9

    // create iretq frame
    // ss = 16 (segment selector 16 for kernel data, ring 0)
    pushq $16
    // rsp = rcx
    pushq %rcx
    // rflags (0x200 = interrupt enable)
    pushq $0x200
    // cs = 8 (segment selector 8 for kernel code, ring 0
    pushq $8
    // rip = entry point (already in r8)
    pushq %r8

    // don't wanna give threads access to the registers of the last thread lol
    CLEAR_ALL_REGS

    // finally enter the new context
    iretq

.previously_switched:

    // restore state
    POP_ALL_REGS
    
    // we don't need the current_thread and next_thread that were saved
    addq $16, %rsp

    // return frame is already on the stack, iretq back to wherever we were
    iretq

.section .data
msg1: .asciz "prev thread: %x\nnext thread: %x\nRSP: %x\nEntry point: %x\nFlags: %x\n"
