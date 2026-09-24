// risc-v context switching. this is quite a hotpath so keep this zoomy zoomy fast fast

// neovim was being a dickhead so I had to add this lol:
// asmsyntax=asm

#include <exposed_structs.S>

.global context_switch

.macro PUSH_REGS
    addi sp, sp, -96
    sd s0, 0(sp)
    sd s1, 8(sp)
    sd s2, 16(sp)
    sd s3, 24(sp)
    sd s4, 32(sp)
    sd s5, 40(sp)
    sd s6, 48(sp)
    sd s7, 56(sp)
    sd s8, 64(sp)
    sd s9, 72(sp)
    sd s10, 80(sp)
    sd s11, 88(sp)
.endm // PUSH_REGS

.macro POP_REGS
    ld s0, 0(sp)
    ld s1, 8(sp)
    ld s2, 16(sp)
    ld s3, 24(sp)
    ld s4, 32(sp)
    ld s5, 40(sp)
    ld s6, 48(sp)
    ld s7, 56(sp)
    ld s8, 64(sp)
    ld s9, 72(sp)
    ld s10, 80(sp)
    ld s11, 88(sp)
    addi sp, sp, 96
.endm // POP_REGS

.macro PUSH reg
    addi sp, sp, -8
    sd \reg, 0(sp)
.endm

.macro POP reg
    ld \reg, 0(sp)
    addi sp, sp, 8
.endm

/* saves the context of the current thread and enters the context of the next
 * thread. can be called by either yield or the timer for preemption.
 *
 * arguments:
 *   -> a0: current thread
 *   -> a1: next thread
 */ 
context_switch:
    PUSH ra
    PUSH_REGS // save regs for previous task

    sd sp, THREAD_KERNEL_STACK_OFFSET(a0) // save sp
    ld sp, THREAD_KERNEL_STACK_OFFSET(a1) // switch sp

    POP_REGS // restore regs for next task
    POP ra
    ret // go back to where the next task left off (or entry point)
