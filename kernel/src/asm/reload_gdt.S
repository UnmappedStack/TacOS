.globl reload_gdt
reload_gdt:
    sub $8, %rsp // Alignation
    push $0x08
    push .reload_CS
    retfq

.reload_CS:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    xor %ax, %ax
    mov %ax, %fs
    mov %ax, %gs
    pop %rax
    ret
