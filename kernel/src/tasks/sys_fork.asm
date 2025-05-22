[BITS 64]
extern printf
extern fork
global sys_fork

sys_fork:
    lea rdi, [rsp + 8 + 112] ; rax contains the stack before the call frame & pushed regs
    call fork
    ret

msg1: db "rsp = %p", 10, 0
msg2: db "rax = %p", 10, 0
