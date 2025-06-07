#include <printf.h>
#include <cpu.h>
#include <cpu/msr.h>
#include <cpu/cpuid.h>
#define CPUID_FLAG_MSR 1 << 5

void init_msr(void) {
   static uint32_t a, d; // eax, edx
   cpuid(1, &a, &d);
   if (d & CPUID_FLAG_MSR) {
       printf("MSRs are avaliable on this CPU.\n");
       return;
   }
   printf("MSRs are not avaliable on this device.\n");
   HALT_DEVICE();
}

void read_msr(uint32_t msr, uint64_t *val) {
    uint32_t lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    *val = lo | ((uint64_t) hi << 32);
}

void write_msr(uint32_t msr, uint64_t val) {
    __asm__ volatile("wrmsr" : : "a"(val & 0xFFFFFFFF), "d"(val >> 32), "c"(msr));
}
