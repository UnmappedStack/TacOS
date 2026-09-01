#include <isa/cpu.h>
#include <stdint.h>
#include <kprintf.h>

static inline SBIRet sbi_send1(uint32_t extid, uint32_t funcid, uint32_t arg1) {
    register long a7 __asm__("a7") = extid;
    register long a6 __asm__("a6") = funcid;
    register long a0 __asm__("a0") = arg1;
    register long a1 __asm__("a1");
    __asm__ volatile("ecall"
                    : "+r"(a0), "=r"(a1)
                    : "r"(a7), "r"(a6)
                    : "memory");
    return (SBIRet) {.err = a0, .val = a1};
}

static inline SBIRet sbi_send2(int32_t extid, int32_t funcid,
                                     uint64_t arg1, uint64_t arg2) {
    register uint64_t a7 __asm__("a7") = extid;
    register uint64_t a6 __asm__("a6") = funcid;
    register uint64_t a0 __asm__("a0") = arg1;
    register uint64_t a1 __asm__("a1") = arg2;
    __asm__ volatile("ecall"
                    : "+r"(a0), "+r"(a1)
                    : "r"(a7), "r"(a6)
                    : "memory");
    return (SBIRet) {.err = a0, .val = a1};
}

static inline long sbi_send_legacy1(long funcid, long arg0) {
    register long a7 __asm__("a7") = funcid;
    register long a0 __asm__("a0") = arg0;
    __asm__ volatile("ecall"
        : "+r"(a0)
        : "r"(a7)
        : "memory");
    return a0;
}

void sbi_write_char(char c) {
    sbi_send_legacy1(1, c);
}

void sbi_ipi_all(void) {
    sbi_send2(0x735049, 0, 0, -1);

    // give a bit of time for the ipi to be handled before continuing. I guess
    // technically this is a race condition but it doesn't really matter that much here.
    // anyways TODO fix this
    for (int i = 0; i < 10000; i++) PAUSE();
}
