#pragma once
#include <smp.h>

#define CPUID_FEATURE_MSR (1u << 5)
typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} CPUIDResult;

uint64_t rdmsr(uint32_t msr);
void wrmsr(uint32_t msr, uint64_t value);
bool cpu_has_msr(void);

CPUIDResult cpuid(uint32_t leaf, uint32_t subleaf);

CPU *get_current_cpu_info(void);
void set_current_cpu_info(CPU *cpu_info);
