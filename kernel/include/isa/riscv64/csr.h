#pragma once
#include <stdint.h>

#define CSR_REG_SSTATUS  "0x100"
#define CSR_REG_SIE      "0x104"
#define CSR_REG_STVEC    "0x105"
#define CSR_REG_SSCRATCH "0x140"
#define CSR_REG_STIMECMP "0x14D"

#define STIE 0x20
#define SSIP 0x02

#define csr_set_bits(csr, val) \
    __asm__ volatile("csrs " csr ", %0" :: "r"(val))

#define csr_clear_bits(csr, val) \
    __asm__ volatile("csrc " csr ", %0" :: "r"(val))

#define csr_write(csr, val) \
    __asm__ volatile("csrw " csr ", %0" :: "r"(val))

#define csr_read(csr) \
    ({size_t val; __asm__ volatile("csrr %0, " csr : "=r"(val)); val; })

