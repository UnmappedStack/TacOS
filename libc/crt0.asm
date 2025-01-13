[BITS 64]
global _start
extern main

section .text
_start:
    call init_libc
    call main
    mov rdi, rax
    mov rax, 4
    int 0x80
    jmp $ ; in case exit failed, loop forever

init_libc:
    ;; TODO: Implement any initialisation
    ret
