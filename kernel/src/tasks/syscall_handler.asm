;; The design of how I handle syscalls is inspired by Dcraftbg :D
[BITS 64]

extern printf
extern iretq_msg
extern lock_pit
extern unlock_pit

%include "include/asm.inc"

extern syscalls
extern num_syscalls
extern sys_invalid

global syscall_isr
global syscall_handler

;; `int 0x80` syscall interrupt handler
syscall_isr:
    cmp rax, [num_syscalls]
    jge invalid_syscall
    pushmost
    call [syscalls + rax * 8]
    popmost
    iretq

;; `syscall` instruction handler
syscall_handler:
    mov [user_rsp], rsp
    mov [user_rflags], r11
    mov rsp, 0xFFFFFFFFFFFFF000
    push rcx ; return address
    pushall
    mov rdi, in_syscall_msg
    mov rsi, [user_rsp]
    call printf
    popall
    pop rcx
    cli ; no interrupts should ever be called while on the user stack yet
        ; from within the kernel.
    mov rsp, [user_rsp]
    mov r11, [user_rflags]
    o64 sysret

invalid_syscall:
    push rdi
    mov rdi, rax
    call sys_invalid
    iretq

section .rodata
in_syscall_msg: db "In `syscall` handler :) rsp is %p", 10, 0
rdi_msg: db "RDI = %i", 10, 0
rsp_msg: db "RSP in syscall handler = %p", 10, 0

section .data
user_rsp:    dq 0
user_rflags: dq 0

