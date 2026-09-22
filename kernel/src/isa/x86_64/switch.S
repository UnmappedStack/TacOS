// x86-64 context switching. this is quite a hotpath so keep this zoomy zoomy fast fast

// neovim was being a dickhead so I had to add this lol:
// asmsyntax=asm

#include <exposed_structs.S>

.global context_switch
.global end_of_interrupt
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

preempt:
    cli
    PUSH_MOST_REGS
    call end_of_interrupt
    call yield
    // by the time we get here we're already on the next timeslice
    // after context switching *back* here... usually. occasionally yield
    // will not find a thread to switch to so we'll still be in the same
    // timeslice.
    POP_MOST_REGS
    iretq

/* saves the context of the current thread and enters the context of the next
 * thread. can be called by either yield or the timer for preemption.
 *
 * arguments:
 *   -> rdi: current thread
 *   -> rsi: next thread
 */ 
context_switch:
    // save state
    PUSH_SOME_REGS

    movq %rsp, THREAD_KERNEL_STACK_OFFSET(%rdi) // save rsp
    movq THREAD_KERNEL_STACK_OFFSET(%rsi), %rsp // switch to next task's rsp

    POP_SOME_REGS // restore state
    
    // return frame is already on the stack, just return
    ret
