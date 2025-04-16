[BITS 64]
global _start
global start_heap
extern main
extern init_streams

%define HEAP_VERIFY_OFF        0
%define HEAP_NEXT_OFF          1
%define HEAP_POOL_SIZE_OFF     9
%define HEAP_REQUIRED_SIZE_OFF 17
%define HEAP_FREE_OFF          25

section .text
_start:
    sub rsp, 0
    mov rbp, rsp
    push rbp
    ; save argc+argv
    push rdi
    push rsi
    ; initiate everything the libc uses (streams, heap, etc)
    call init_libc
    ; restore argc+argv & call entry
    pop rsi
    pop rdi
    call main
    ; exit
    mov rdi, rax
    mov rax, 4
    int 0x80
    jmp $ ; in case exit failed, loop forever

init_libc:
    ;; Initiate the heap
    ; Move the program break forward by a page and get the initial program break
    mov rax, 11   ; sbrk(
    mov rdi, 4096 ;   4096
    int 0x80      ; );
    mov [start_heap], rax
    ; Fill the heap so far with a single pool
    mov byte  [rax + HEAP_VERIFY_OFF       ], 69
    mov qword [rax + HEAP_NEXT_OFF         ], 0
    mov qword [rax + HEAP_POOL_SIZE_OFF    ], 4095
    mov qword [rax + HEAP_REQUIRED_SIZE_OFF], 4095
    mov byte  [rax + HEAP_FREE_OFF         ], 1
    ;; Initiate other stuff
    call init_streams
    ret

section .data
start_heap:
    dq 0
