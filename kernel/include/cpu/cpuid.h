static inline void cpuid(int code, uint32_t *a, uint32_t *d) {
    __asm__ volatile("cpuid":"=a"(*a),"=d"(*d):"a"(code):"ecx","ebx");
}
