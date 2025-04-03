;; The design of how I handle syscalls is inspired by Dcraftbg :D
[BITS 64]

extern printf
extern iretq_msg
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
extern sys_remove
extern sys_mkdir
extern sys_lseek

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
    dq sys_remove ; 13
    dq sys_mkdir  ; 14
    dq sys_lseek  ; 15
syscall_lookup_end:

global syscall_isr

syscall_isr:
    cmp rax, (syscall_lookup_end-syscall_lookup) / 8
    jge invalid_syscall
    push rbp
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
    pop rbp
    iretq

invalid_syscall:
    push rdi
    mov rdi, rax
    call sys_invalid
    iretq

print_iretq_outputs:
    ; Pop iretq frame for printf
    pop rsi
    pop rdx
    pop rcx
    pop r8
    pop r9
    ; Push it back so it's still able to iretq
    push r9
    push r8
    push rcx
    push rdx
    push rsi
    ; print and return
    mov rdi, iretq_msg
    call printf
    mov rdi, rbp_msg
    mov rsi, rbp
    call printf
    iretq

section .rodata
in_syscall_msg: db "In syscall %i", 10, 0
rbp_msg: db "RBP = %p", 10, 0
