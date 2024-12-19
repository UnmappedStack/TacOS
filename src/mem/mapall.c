#include <printf.h>
#include <mem/paging.h>
#include <kernel.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

extern uint64_t p_kernel_start[];
extern uint64_t p_writeallowed_start[];
extern uint64_t p_kernel_end[];

uint64_t kernel_start       = (uint64_t) p_kernel_start;
uint64_t writeallowed_start = (uint64_t) p_writeallowed_start;
uint64_t kernel_end         = (uint64_t) p_kernel_end;

void map_sections(uint64_t pml4[]) {
    for (size_t entry = 0; entry < kernel.memmap_entry_count; entry++) {
        uint64_t entry_type = kernel.memmap[entry].type;
        if (entry_type == LIMINE_MEMMAP_USABLE ||
            entry_type == LIMINE_MEMMAP_FRAMEBUFFER ||
            entry_type == LIMINE_MEMMAP_KERNEL_AND_MODULES) {
            printf("Trying to map entry #%i (%i pages, vaddr = %p, paddr = %p)...", entry, kernel.memmap[entry].length / 4096, kernel.memmap[entry].base + kernel.hhdm, kernel.memmap[entry].base);
            map_pages(pml4, kernel.memmap[entry].base + kernel.hhdm, kernel.memmap[entry].base, kernel.memmap[entry].length / 4096, KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_WRITE);
            printf(" Success\n");
        }
    }
}

void map_kernel(uint64_t pml4[]) {
    uint64_t length_buffer = 0;
    uint64_t phys_buffer = 0;
    /* map from kernel_start to writeallowed_start with only the present flag */
    length_buffer = PAGE_ALIGN_UP(writeallowed_start - kernel_start);
    phys_buffer = kernel.kernel_phys_addr + (kernel_start - kernel.kernel_virt_addr);
    map_pages(pml4, PAGE_ALIGN_DOWN(kernel_start), phys_buffer, length_buffer / 4096, KERNEL_PFLAG_PRESENT);
    /* map from writeallowed_start to kernel_end with `present` and `write` flags */
    length_buffer = PAGE_ALIGN_UP(kernel_end - writeallowed_start);
    phys_buffer = kernel.kernel_phys_addr + (writeallowed_start - kernel.kernel_virt_addr);
    map_pages(pml4, PAGE_ALIGN_DOWN(writeallowed_start), phys_buffer, length_buffer / 4096, KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_WRITE); 
    // map the kernel's stack
    alloc_pages(pml4, KERNEL_STACK_ADDR, KERNEL_STACK_PAGES, KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_WRITE);
}

void map_all(uint64_t pml4[]) {
    map_kernel(pml4);
    map_sections(pml4);
}
