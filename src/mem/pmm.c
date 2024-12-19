#include <mem/pmm.h>
#include <kernel.h>
#include <printf.h>
#include <stddef.h>
#include <list.h>

void init_chunk(void *addr, size_t len) {
    Chunk *chunk  = (Chunk*) addr;
    chunk->length = PAGE_ALIGN_DOWN(len) / 4096;
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
    if (!list_len) {
        printf("Failed to initialise physical memory allocator, cannot continue.\n");
        asm volatile("cli"); // TODO: move to a seperate kpanicf function
        for (;;) asm volatile("hlt");
    }
    printf("Successfully initiated %i physical allocator chunks.\n", list_len);
}

// Returns a physical address
uintptr_t kmalloc(size_t num_pages) {
    size_t i = 0;
    Chunk *chunk;
    for (struct list *at = kernel.pmm_chunklist; at != kernel.pmm_chunklist || i == 0; at = at->next) {
        chunk = (Chunk*) at;
        if (chunk->length <= num_pages + 1) {
            i++;
            continue;
        }
        if (!i) kernel.pmm_chunklist = kernel.pmm_chunklist->next;
        list_remove(&chunk->list);
        Chunk *new_chunk = (Chunk*)((uint64_t) chunk + num_pages * PAGE_SIZE + 1);
        list_insert(kernel.pmm_chunklist, &new_chunk->list);
        init_chunk(new_chunk, (chunk->length - (num_pages + 1)) * PAGE_SIZE);
        return (uintptr_t) chunk;
    }
    return 0;
}




