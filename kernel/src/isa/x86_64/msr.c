#include <stdbool.h>
#include <stdint.h>
#include <isa/cpu.h>

CPUIDResult cpuid(uint32_t leaf, uint32_t subleaf) {
    CPUIDResult r;
    __asm__ volatile (
        "cpuid"
        : "=a"(r.eax), "=b"(r.ebx), "=c"(r.ecx), "=d"(r.edx)
        : "a"(leaf), "c"(subleaf)
    );
    return r;
}

bool cpu_has_msr(void) {
    CPUIDResult r = cpuid(1, 0);
    return (r.edx & CPUID_FEATURE_MSR) != 0;
}

uint64_t rdmsr(uint32_t msr) {
    uint32_t lo;
    uint32_t hi;

    __asm__ volatile (
        "rdmsr"
        : "=a"(lo), "=d"(hi)
        : "c"(msr)
    );

    return ((uint64_t)hi << 32) | lo;
}

void wrmsr(uint32_t msr, uint64_t value) {
    uint32_t lo = (uint32_t)value;
    uint32_t hi = (uint32_t)(value >> 32);

    __asm__ volatile (
        "wrmsr"
        :
        : "c"(msr), "a"(lo), "d"(hi)
        : "memory"
    );
}

/* accesses through gsbase for x86_64 */
CPU *get_current_cpu_info(void) {
    CPU *ret = (CPU*) rdmsr(GSBASE);
    if (ret == NULL) {
        kpanic("GSBase not set yet");
    }
    return ret;
}

void set_current_cpu_info(CPU *cpu_info) {
    wrmsr(GSBASE, (uint64_t)cpu_info);
}
