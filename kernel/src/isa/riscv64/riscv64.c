#include <isa/cpu.h>
#include <mm.h>

void switch_page_tree(uintptr_t cr3) {
    uint64_t satp = (cr3/PAGE_BYTES) | ((PAGE_LEVELS+5ULL)<<60);
    __asm__ volatile("sfence.vma");
    csr_write("satp", satp);
}
