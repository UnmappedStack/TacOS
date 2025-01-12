;; The design of how I handle syscalls is inspired by Dcraftbg :D
[BITS 64]

extern sys_read
extern sys_write
extern sys_open
extern sys_close
extern sys_invalid
extern sys_exit
extern sys_getpid
extern sys_fork
extern sys_execve
extern sys_kill
extern sys_isatty
extern sys_wait
extern sys_sbrk
extern sys_unlink
PTR_SIZE equ 8

syscall_lookup:
    dq sys_read
    dq sys_write
    dq sys_open
    dq sys_close
    dq sys_exit
    dq sys_getpid
    dq sys_fork
    dq sys_execve
    dq sys_kill
    dq sys_isatty
    dq sys_wait
    dq sys_sbrk
    dq sys_unlink
syscall_lookup_end:

global syscall_isr

syscall_isr:
    cli
    cmp rax, (syscall_lookup_end-syscall_lookup) / 8
    jae invalid_syscall
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    call [syscall_lookup + rax * 8]
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    iretq

invalid_syscall:
    push rdi
    mov rdi, rax
    call sys_invalid
    pop rdi
    ret
