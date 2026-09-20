// x86-64 context switching. this is quite a hotpath so keep this zoomy zoomy fast fast

// neovim was being a dickhead so I had to add this lol:
// asmsyntax=asm

#include <exposed_structs.S>
#define THREAD_FLAG_FIRST_SWITCH 0b100

.global context_switch
.global preempt

.macro PUSH_SOME_REGS
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
.endm // PUSH_SOME_REGS

.macro POP_SOME_REGS
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
.endm // POP_SOME_REGS

.macro PUSH_MOST_REGS
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %r9 
    pushq %r10
    pushq %r11
    pushq %rsi
    pushq %rdi
.endm // PUSH_MOST_REGS

.macro POP_MOST_REGS
    popq %rdi
    popq %rsi
    popq %r11
    popq %r10
    popq %r9 
    popq %rdx
    popq %rcx
    popq %rax
.endm // POP_MOST_REGS

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

/* just an isr stub to go to C's yield().
 * TODO: this should do eoi instead of yield */
preempt:
    cli
    PUSH_MOST_REGS
    call yield
    POP_MOST_REGS
    ret

/* saves the context of the current thread and enters the context of the next
 * thread. can be called by either yield or the timer for preemption.
 *
 * arguments:
 *   -> rdi: current thread
 *   -> rsi: next thread
 */ 
context_switch:
    cli

    // put current & next threads on the stack to use after
    pushq %rdi // current thread
    pushq %rsi // next thread

    // save state
    PUSH_SOME_REGS

    // the saved registers are 48 bytes (6 regs * 8 byte alignment)
    // so we can get back the current & next threads from before that
    movq 48(%rsp), %rdx // rdx = next thread, +0 bytes for rsi
    movq 56(%rsp), %rsi // rsi = current thread, +8 bytes for rdi

    movq THREAD_KERNEL_STACK_OFFSET(%rdx), %rcx // rcx = rsp
    movq THREAD_ENTRY_POINT_OFFSET(%rdx), %r8   // r8  = entry point
    movq THREAD_FLAGS_OFFSET(%rdx), %r9         // r9  = flags

    // save rsp
    movq %rsp, THREAD_KERNEL_STACK_OFFSET(%rsi)

    // switch to next task's rsp
    movq %rcx, %rsp
    
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
    movq %r9, THREAD_FLAGS_OFFSET(%rdx)

    // rip = entry point (already in r8)
    pushq %r8

    // don't wanna give threads access to the registers of the last thread lol
    CLEAR_ALL_REGS

    // finally enter the new context
    ret 

.previously_switched:

    // restore state
    POP_SOME_REGS
    
    // we don't need the current_thread and next_thread that were saved
    addq $16, %rsp

    // return frame is already on the stack, iretq back to wherever we were
    ret
