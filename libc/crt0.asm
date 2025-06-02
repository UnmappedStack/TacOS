[BITS 64]
global _start
global start_heap
extern main
extern init_environ
extern init_streams
extern exit

section .text
_start:
    mov rbp, rsp
    push rbp
    push rcx
    ;syscall ; test
    pop rcx
    ; save argc+argv+envp
    push rdi
    push rsi
    push rdx
    ; initiate everything the libc uses (streams, heap, etc)
    call init_libc
    ; restore argc+argv+envp, call entry, end exit
    pop rdx
    pop rsi
    pop rdi
    call main
    call exit
    jmp $ ; in case exit failed, loop forever

; takes envp in rdx
init_libc:
    mov rdi, rdx
    call init_environ
    call init_streams
    ret
