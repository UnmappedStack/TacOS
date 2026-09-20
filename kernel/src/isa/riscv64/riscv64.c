#include <isa/cpu.h>
#include <isa/riscv64/csr.h>
#include <mm.h>
#include <smp.h>

void switch_page_tree(uintptr_t cr3) {
    uint64_t satp = (cr3/PAGE_BYTES) | ((PAGE_LEVELS+5ULL)<<60);
    __asm__ volatile("sfence.vma");
    csr_write("satp", satp);
}

#define PPN_MASK 0x3fffff
void read_page_tree(uintptr_t *dest) {
    __asm__ volatile("sfence.vma");
    uint64_t satp = csr_read("satp");
    *dest = (satp & PPN_MASK) * PAGE_BYTES;
}

// may return 0 if not yet set
CPU *get_current_cpu_info(void) {
   return (CPU*) csr_read(CSR_REG_SSCRATCH);
}

void set_current_cpu_info(CPU *cpu_info) {
    csr_write(CSR_REG_SSCRATCH, cpu_info);
}
