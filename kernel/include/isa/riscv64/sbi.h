#pragma once

typedef struct {
    long err;
    long val;
} SBIRet;

void sbi_write_char(char c);
