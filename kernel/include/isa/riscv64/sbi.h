#pragma once

typedef struct {
    int64_t err;
    uint64_t val;
} SBIRet;

void sbi_write_char(char c);
void sbi_ipi_all(void);
void sbi_ipi_cpux(size_t hartid);
