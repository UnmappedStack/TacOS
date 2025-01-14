[BITS 64]
global _start
extern main

%define HEAP_VERIFY_OFF        0
%define HEAP_NEXT_OFF          1
%define HEAP_POOL_SIZE_OFF     9
%define HEAP_REQUIRED_SIZE_OFF 17

section .text
_start:
    call init_libc
    call main
    mov rdi, rax
    mov rax, 4
    int 0x80
    jmp $ ; in case exit failed, loop forever

init_libc:
    ;; Initiate the heap
    ; Get the location of the start of the heap
    mov rax, 11 ; sbrk(
    mov rdi, 0  ;   0
    int 0x80    ; );
    mov [heap_start], rax
    ; Move the program break forward by a page
    mov rax, 11   ; sbrk(
    mov rdi, 4096 ;   4096
    int 0x80      ; );
    ; Fill the heap so far with a single pool
    mov byte  [heap_start + HEAP_VERIFY_OFF       ], 69
    mov qword [heap_start + HEAP_NEXT_OFF         ], 0
    mov qword [heap_start + HEAP_POOL_SIZE_OFF    ], 4095
    mov qword [heap_start + HEAP_REQUIRED_SIZE_OFF], 4095
    ret

section .data
heap_start: dq 0
