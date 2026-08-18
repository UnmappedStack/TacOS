.global prepare_for_interrupt

.align 4
prepare_for_interrupt:
    // We need space on the stack for the following structure:
    // {
    //      u64 ret_addr; (sepc)
    //      u64 cause;
    //      u64 val;
    // }
    // This ends up being 32 bytes (3 elements * 8 bytes, rounded up to align to 16 bytes)
    // The stack then ALSO has to be taken down by 
    addi sp, sp, -48
    // Then create the frame
    csrr t1, 0x141 // sepc
    sd t1, 0(sp)
    csrr t1, 0x142 // cause
    sd t1, 8(sp)
    csrr t1, 0x143 // value
    sd t1, 16(sp)
    mv a0, sp// move the stack frame into call arguments
    // save registers
    // 464 bytes for register storage: 16 bytes * 29 registers (-2 registers because the
    // stack pointer and hardwired zero registers have no reason to be saved)
    addi sp, sp, -480
    sd x1,    8(sp)
    sd x3,   24(sp)
    sd x4,   32(sp)
    sd x5,   40(sp)
    sd x6,   48(sp)
    sd x7,   56(sp)
    sd x8,   64(sp)
    sd x9,   72(sp)
    sd x10,  80(sp)
    sd x11,  88(sp)
    sd x12,  96(sp)
    sd x13, 104(sp)
    sd x14, 112(sp)
    sd x15, 120(sp)
    sd x16, 128(sp)
    sd x17, 136(sp)
    sd x18, 144(sp)
    sd x19, 152(sp)
    sd x20, 160(sp)
    sd x21, 168(sp)
    sd x22, 176(sp)
    sd x23, 184(sp)
    sd x24, 192(sp)
    sd x25, 200(sp)
    sd x26, 208(sp)
    sd x27, 216(sp)
    sd x28, 224(sp)
    sd x29, 232(sp)
    sd x30, 240(sp)
    sd x31, 248(sp)
    // keep in mind that before pushing all the registers we already stored the stack frame pointer
    // in a0 (the first argument according to the rv64 abi), so that'll be passed to the c handler
    call interrupt_handler
    // put the potentially modified sepc return addr back
    addi t0, sp, 480 // t0 = start of stack frame
    ld t1, (t0)      // t1 = stackframe.sepc
    csrw 0x141, t1   // set sepc again
    // restore registers
    ld x1,    8(sp)
    ld x3,   24(sp)
    ld x4,   32(sp)
    ld x5,   40(sp)
    ld x6,   48(sp)
    ld x7,   56(sp)
    ld x8,   64(sp)
    ld x9,   72(sp)
    ld x10,  80(sp)
    ld x11,  88(sp)
    ld x12,  96(sp)
    ld x13, 104(sp)
    ld x14, 112(sp)
    ld x15, 120(sp)
    ld x16, 128(sp)
    ld x17, 136(sp)
    ld x18, 144(sp)
    ld x19, 152(sp)
    ld x20, 160(sp)
    ld x21, 168(sp)
    ld x22, 176(sp)
    ld x23, 184(sp)
    ld x24, 192(sp)
    ld x25, 200(sp)
    ld x26, 208(sp)
    ld x27, 216(sp)
    ld x28, 224(sp)
    ld x29, 232(sp)
    ld x30, 240(sp)
    ld x31, 248(sp)
    addi sp, sp, 528 // registers frame + interrupt frame
    sret
