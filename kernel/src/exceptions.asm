[BITS 64]

extern panic_handler

section .text

global divide_exception
global debug_exception
global breakpoint_exception
global overflow_exception
global bound_range_exceeded_exception
global invalid_opcode_exception
global device_not_avaliable_exception
global double_fault_exception
global coprocessor_segment_overrun_exception
global invalid_TSS_exception
global segment_not_present_exception
global stack_segment_fault_exception
global general_protection_fault_exception
global page_fault_exception
global floating_point_exception
global alignment_check_exception
global machine_check_exception
global simd_floating_point_exception
global virtualisation_exception

align 0x08, db 0x00
divide_exception:
    push 0
    push 0
    jmp base_handler

align 0x08, db 0x00
debug_exception:
    push 0
    push 1
    jmp base_handler

align 0x08, db 0x00
breakpoint_exception:
    push 0
    push 3
    jmp base_handler

align 0x08, db 0x00
overflow_exception:
    push 0
    push 4
    jmp base_handler

align 0x08, db 0x00
bound_range_exceeded_exception:
    push 0
    push 5
    jmp base_handler

align 0x08, db 0x00
invalid_opcode_exception:
    push 0
    push 6
    jmp base_handler

align 0x08, db 0x00
device_not_avaliable_exception:
    push 0
    push 7
    jmp base_handler

align 0x08, db 0x00
double_fault_exception:
    push 8
    jmp base_handler

align 0x08, db 0x00
coprocessor_segment_overrun_exception:
    push 0
    push 9
    jmp base_handler

align 0x08, db 0x00
invalid_TSS_exception:
    push 10
    jmp base_handler

align 0x08, db 0x00
segment_not_present_exception:
    push 11
    jmp base_handler

align 0x08, db 0x00
stack_segment_fault_exception:
    push 12
    jmp base_handler

align 0x08, db 0x00
general_protection_fault_exception:
    push 13
    jmp base_handler

align 0x08, db 0x00
page_fault_exception:
    push 14
    jmp base_handler

align 0x08, db 0x00
floating_point_exception:
    push 0
    push 16
    jmp base_handler

align 0x08, db 0x00
alignment_check_exception:
    push 17
    jmp base_handler

align 0x08, db 0x00
machine_check_exception:
    push 0
    push 18
    jmp base_handler

align 0x08, db 0x00
simd_floating_point_exception:
    push 0
    push 19
    jmp base_handler

align 0x08, db 0x00
virtualisation_exception:
    push 0
    push 20
    jmp base_handler

align 0x08, db 0x00
base_handler:
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
    mov rax, cr2
    push rax
    cld
    mov rsi, rsp
    xor rdi, rdi
    call panic_handler 
    add rsp, 8
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
    add rsp, 0x10
    iretq
