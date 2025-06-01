#pragma once
#include <stdint.h>

void init_msr();
void read_msr(uint32_t msr, uint32_t *lo, uint32_t *hi);
void write_msr(uint32_t msr, uint64_t val);
