.globl divide_exception
.globl debug_exception
.globl breakpoint_exception
.globl overflow_exception
.globl bound_range_exceeded_exception
.globl invalid_opcode_exception
.globl device_not_avaliable_exception
.globl double_fault_exception
.globl coprocessor_segment_overrun_exception
.globl invalid_tss_exception
.globl segment_not_present_exception
.globl stack_segment_fault_exception
.globl general_protection_fault_exception
.globl page_fault_exception
.globl floating_point_exception
.globl alignment_check_exception
.globl machine_check_exception
.globl simd_floating_point_exception
.globl virtualisation_exception

divide_exception:
    push $0
    push $0
    jmp base_handler

debug_exception:
    push $0
    push $1
    jmp base_handler

breakpoint_exception:
    push $0
    push $3
    jmp base_handler

overflow_exception:
    push $0
    push $4
    jmp base_handler

bound_range_exceeded_exception:
    push $0
    push $5
    jmp base_handler

invalid_opcode_exception:
    push $0
    push $6
    jmp base_handler

device_not_avaliable_exception:
    push $0
    push $7
    jmp base_handler

double_fault_exception:
    push $8
    jmp base_handler

coprocessor_segment_overrun_exception:
    push $0
    push $9
    jmp base_handler

invalid_tss_exception:
    push $10
    jmp base_handler

segment_not_present_exception:
    push $11
    jmp base_handler

stack_segment_fault_exception:
    push $12
    jmp base_handler

general_protection_fault_exception:
    push $13
    jmp base_handler

page_fault_exception:
    push $14
    jmp base_handler

floating_point_exception:
    push $0
    push $16
    jmp base_handler

alignment_check_exception:
    push $17
    jmp base_handler

machine_check_exception:
    push $0
    push $18
    jmp base_handler

simd_floating_point_exception:
    push $0
    push $19
    jmp base_handler

virtualisation_exception:
    push $0
    push $20
    jmp base_handler

base_handler:
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
    mov %rax, %rdi
    mov %cr2, %rax
    push %rax
    cld
    mov %rdi, %rsp
    call exception_handler 
    add %rsp, 8
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax
    add 0x10, %rsp
    iretq
