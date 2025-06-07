#pragma once
#include <stdint.h>

void init_msr(void);
void read_msr(uint32_t msr, uint64_t *val);
void write_msr(uint32_t msr, uint64_t val);
