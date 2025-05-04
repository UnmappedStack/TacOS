.globl __setjmp_internal
__setjmp_internal:
    pushq %rbp
    mov %rsp, %rbp
    // save return address
    movq 8(%rbp), %rax
    movq %rax, 0(%rdi)
    // save stack pointer
    leaq 16(%rbp), %rax
    movq %rax, 8(%rdi)
    // save gprs
    movq %rax,  16(%rdi)
    movq %rbx,  24(%rdi)
    movq %rcx,  32(%rdi)
    movq %rdx,  40(%rdi)
    movq %rdi,  48(%rdi)
    movq %rsi,  56(%rdi)
    movq  %r8,  64(%rdi)
    movq  %r9,  72(%rdi)
    movq %r10,  80(%rdi)
    movq %r11,  88(%rdi)
    movq %r12,  96(%rdi)
    movq %r13, 104(%rdi)
    movq %r14, 112(%rdi)
    movq %r15, 120(%rdi)
    movq (%rbp), %rax
    mov %rax, 128(%rdi)
    // return 0
    popq %rbp
    xorq %rax, %rax
    ret

.globl __longjmp_internal
__longjmp_internal:
    // set return value to val
    movq %rsi, %rax
    // restore gprs
    movq  24(%rdi), %rbx
    movq  32(%rdi), %rcx
    movq  40(%rdi), %rdx
    movq  64(%rdi), %r8
    movq  72(%rdi), %r9
    movq  80(%rdi), %r10
    movq  88(%rdi), %r11
    movq  96(%rdi), %r12
    movq 104(%rdi), %r13
    movq 112(%rdi), %r14
    movq 120(%rdi), %r15
    movq 128(%rdi), %rbp
    // restore rsp and jump to return address
    mov 0(%rdi), %rsi
    mov 8(%rdi), %rsp
    jmp %rsi


