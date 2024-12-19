[BITS 64]

global reload_gdt

section .text

;; Reloads the GDT
reload_gdt:
    sub rsp, 8 ; Alignation
    push 0x08
    push .reload_CS
    retfq

.reload_CS:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    xor ax, ax
    mov fs, ax
    mov gs, ax
    pop rax
    ret
