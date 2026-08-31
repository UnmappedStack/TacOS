#include <isa/cpu.h>
#include <isa/riscv64/csr.h>
#include <mm.h>
#include <smp.h>

void switch_page_tree(uintptr_t cr3) {
    uint64_t satp = (cr3/PAGE_BYTES) | ((PAGE_LEVELS+5ULL)<<60);
    __asm__ volatile("sfence.vma");
    csr_write("satp", satp);
}

CPU *get_current_cpu_info(void) {
   CPU *ret = (CPU*) csr_read(CSR_REG_SSCRATCH);
   if (!ret) kpanic("sscratch not set yet");
   return ret;
}

void set_current_cpu_info(CPU *cpu_info) {
    csr_write(CSR_REG_SSCRATCH, cpu_info);
}
