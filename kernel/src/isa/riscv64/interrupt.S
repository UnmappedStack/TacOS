.global prepare_for_interrupt

.align 4
prepare_for_interrupt:
    // We need space on the stack for the following structure:
    // {
    //      u64 regs[32];
    //      u64 ret_addr; (sepc)
    //      u64 cause;
    //      u64 sstatus;
    //      u64 val;
    // }
    // This ends up being 24 bytes (4 elements * 8 bytes)
    // + 248 bytes (32 registers * 8 bytes), then rounded up to
    // be 16 byte aligned.
    addi sp, sp, -280
    // Then create the frame
    csrr t1, 0x141 // sepc
    sd t1, 0(sp)
    csrr t1, 0x142 // cause
    sd t1, 8(sp)
    csrr t1, 0x143 // value
    sd t1, 16(sp)
    csrr t1, 0x100 // sstatus
    sd t1, 24(sp)
    // save registers
    sd x0, 32(sp)
    sd x1, 40(sp)
    sd x2, 48(sp)
    sd x3, 56(sp)
    sd x4, 64(sp)
    sd x5, 72(sp)
    sd x6, 80(sp)
    sd x7, 88(sp)
    sd x8, 96(sp)
    sd x9, 104(sp)
    sd x10, 112(sp)
    sd x11, 120(sp)
    sd x12, 128(sp)
    sd x13, 136(sp)
    sd x14, 144(sp)
    sd x15, 152(sp)
    sd x16, 160(sp)
    sd x17, 168(sp)
    sd x18, 176(sp)
    sd x19, 184(sp)
    sd x20, 192(sp)
    sd x21, 200(sp)
    sd x22, 208(sp)
    sd x23, 216(sp)
    sd x24, 224(sp)
    sd x25, 232(sp)
    sd x26, 240(sp)
    sd x27, 248(sp)
    sd x28, 256(sp)
    sd x29, 264(sp)
    sd x30, 272(sp)
    sd x31, 280(sp)
    mv a0, sp // move the stack frame into call arguments
    call interrupt_handler
    // put the potentially modified sepc return addr back
    ld t1, (sp)      // t1 = stackframe.sepc
    csrw 0x141, t1   // set sepc again
    // restore registers
    ld x0, 32(sp)
    ld x1, 40(sp)
    ld x2, 48(sp)
    ld x3, 56(sp)
    ld x4, 64(sp)
    ld x5, 72(sp)
    ld x6, 80(sp)
    ld x7, 88(sp)
    ld x8, 96(sp)
    ld x9, 104(sp)
    ld x10, 112(sp)
    ld x11, 120(sp)
    ld x12, 128(sp)
    ld x13, 136(sp)
    ld x14, 144(sp)
    ld x15, 152(sp)
    ld x16, 160(sp)
    ld x17, 168(sp)
    ld x18, 176(sp)
    ld x19, 184(sp)
    ld x20, 192(sp)
    ld x21, 200(sp)
    ld x22, 208(sp)
    ld x23, 216(sp)
    ld x24, 224(sp)
    ld x25, 232(sp)
    ld x26, 240(sp)
    ld x27, 248(sp)
    ld x28, 256(sp)
    ld x29, 264(sp)
    ld x30, 272(sp)
    ld x31, 280(sp)
    addi sp, sp, 280 // registers frame + interrupt frame
    sret
