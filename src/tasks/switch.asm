[BITS 64]

global iretq_msg
global context_switch
extern get_current_task
extern task_select
extern printf
extern increment_global_clock

%macro pushall 0
    push rax
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
%endmacro
%macro popall 0
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
    pop rax
%endmacro
%macro clearall 0
    xor rax, rax
    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx
    xor rdi, rdi
    xor rsi, rsi
    xor rbp, rbp
    xor r8,  r8
    xor r9,  r9
    xor r10, r10
    xor r11, r11
    xor r12, r12
    xor r13, r13
    xor r14, r14
    xor r15, r15
%endmacro
%macro eoi 0
    mov al, 0x20
    out 0x20, al
    xor al, al
%endmacro

%define TASK_PID_OFF   16
%define TASK_PML4_OFF  24
%define TASK_RSP_OFF   32
%define TASK_ENTRY_OFF 40
%define TASK_FLAGS_OFF 64

context_switch:
    pushall
    call increment_global_clock
    call task_select ; next Task* is in rax
    mov r15, rax
    ;; Save current rsp of *this* task
    call get_current_task
    mov [rax + TASK_RSP_OFF], rsp
    ;; Switch page tree
    mov r11, [r15 + TASK_PML4_OFF]
    mov cr3, r11
    ;; Switch rsp
    mov rsp, [r15 + TASK_RSP_OFF]
    ;; Check if it's the first exec
    mov r11, [r15 + TASK_FLAGS_OFF]
    and r11, 0b100
    jz .previously_executed
.first_exec:
    ;; Disable first exec flag
    mov r11, 0b100
    not r11
    mov r10, r15
    and r10, r11
    mov [r15 + TASK_FLAGS_OFF], r10
    ;; Push the interrupt stack
    ; ss = 0x20 | 3
    mov rbx, 0x20
    or rbx, 3
    push rbx
    ; rsp = 0x70000000000
    mov rbx, 0x700000000000
    push rbx
    ; rflags = 0x200
    mov rbx, 0x200
    push rbx
    ; cs = 0x18 | 3
    mov rbx, 0x18
    or rbx, 3
    push rbx
    ; rip = entry point in elf
    mov rbx, [r15 + TASK_ENTRY_OFF]
    push rbx
    ;; clear all general purpose registers, send EOI to interrupt controller, and iretq
    eoi
    clearall
    iretq
.previously_executed:
    eoi
    popall
    iretq
    
section .rodata

msg:  db "In context switch :D", 10, 0
msg2: db "PID to switch to: %i", 10, 0
iretq_msg: db "Iretq frame: rip: 0x%p, cs: %i, rflags: 0x%x, rsp = 0x%p, ss = %i", 10, 0
msg4: db "In previously_executed", 10, 0
msg5: db "In first exec", 10, 0
