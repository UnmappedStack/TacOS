#include <mem/pmm.h>
#include <kernel.h>
#include <printf.h>
#include <stddef.h>
#include <list.h>

void init_chunk(void *addr, size_t len) {
    Chunk *chunk  = (Chunk*) addr;
    chunk->length = len;
    // NOTE: The list is already initialised.
}

void init_pmm() {
    printf("HHDM: 0x%p\n", kernel.hhdm);
    printf("There are %i memory map entries.\n", kernel.memmap_entry_count);
    printf("|==================|==================|======================|\n"
           "|      Address     |        Size      |          Type        |\n"
           "|==================|==================|======================|\n");
    size_t list_len = 0;
    for (size_t i = 0; i < kernel.memmap_entry_count; i++) {
        printf("|0x%p|0x%p|%s|\n", kernel.memmap[i].base, kernel.memmap[i].length, mem_types[kernel.memmap[i].type]);
        if (kernel.memmap[i].type != 0) continue; // must be marked usable
        list_len++;
        if (!kernel.pmm_chunklist) {
            kernel.pmm_chunklist = (struct list*) kernel.memmap[i].base;
            list_init(kernel.pmm_chunklist);
        } else {
            list_insert(kernel.pmm_chunklist, (struct list*) kernel.memmap[i].base);
        }
        init_chunk((void*) kernel.memmap[i].base, kernel.memmap[i].length);
    }
    printf("|==================|==================|======================|\n");
    printf("Successfully initiated %i physical allocator chunks.\n", list_len);
}
