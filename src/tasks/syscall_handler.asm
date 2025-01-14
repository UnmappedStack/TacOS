;; The design of how I handle syscalls is inspired by Dcraftbg :D
[BITS 64]

extern lock_pit
extern unlock_pit

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

syscall_lookup:
    dq sys_read   ; 0
    dq sys_write  ; 1
    dq sys_open   ; 2
    dq sys_close  ; 3
    dq sys_exit   ; 4
    dq sys_getpid ; 5
    dq sys_fork   ; 6
    dq sys_execve ; 7
    dq sys_kill   ; 8
    dq sys_isatty ; 9
    dq sys_wait   ; 10
    dq sys_sbrk   ; 11
    dq sys_unlink ; 12
syscall_lookup_end:

global syscall_isr

syscall_isr:
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    cmp rax, (syscall_lookup_end-syscall_lookup) / 8
    jge invalid_syscall
    call [syscall_lookup + rax * 8]
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
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
    iretq
