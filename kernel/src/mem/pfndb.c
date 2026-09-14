/* tracking mechanism of all physical pages.
 * some of these structures kind of vary in terminology
 * between systems so how I refer to stuff here is:
 *    - PFNDB    -> the overall tracking mechanism and the collection
 *               of all `PhysPage`s
 *    - PhysPage -> the structure which tracks a single physical page
 * In a lot of places you'll see PhysPage referred to as page_t (or in Linux,
 * `struct page`, but I find that PhysPage just fits better with the codebase's
 * structure naming scheme.
 *
 * some of this is pretty inspired by evalynOS:
 * https://git.evalyngoemer.com/evalynOS/evalynOS/src/branch/main/kernel/src/mem/pfndb.c
 */

#include <pfndb.h>
#include <paging.h>
#include <string.h>
#include <pma.h>
#include <assert.h>
#include <mm.h>
#include <kernel.h>
#include <kprintf.h>

extern uint64_t kernel_start[];

void pfndb_init(void) {
    /* we give the pfndb the virtual space between the end of the hhdm region
     * and the start of the kernel binary */
    uintptr_t pfndb_start = kernel_info.hhdm + kernel_info.end_of_hhdm_paddr;
    uintptr_t pfndb_end   = (uintptr_t) kernel_start;

    // (both in bytes, not page frames)
    uintptr_t pfndb_space_available = (pfndb_end - pfndb_start);
    uintptr_t pfndb_size = kernel_info.end_of_hhdm_paddr / PAGE_BYTES * sizeof(PhysPage);

    klogf(LOG_DEBUG, "PFNDB in range [%x, %x):\n", pfndb_start, pfndb_end);
    klogf(LOG_DEBUG, "  -> got size %x\n",  pfndb_space_available);
    klogf(LOG_DEBUG, "  -> need size %x\n", pfndb_size);
    assert(pfndb_space_available >= pfndb_size && "not enough virtual memory for PFNDB");

    // it can now actually be set up
    size_t num_memmap_entries = kernel_info.memmap->entry_count;
    struct limine_memmap_entry **entries = kernel_info.memmap->entries;
    size_t last_mapped_vpage = 0;
    for (size_t i = 0; i < num_memmap_entries; i++) {
        uint64_t type   = entries[i]->type;
        if (type != LIMINE_MEMMAP_USABLE) continue;

        uintptr_t base   = entries[i]->base;
        uint64_t  length = entries[i]->length;
        uint64_t  end    = base + length;
        if (length == 0) continue;

        uint64_t pfn_start = base / PAGE_BYTES;
        uint64_t pfn_end   = (base + length + PAGE_BYTES - 1) / PAGE_BYTES;
        uint64_t vaddr_start = pfndb_start + PAGE_ALIGN_DOWN((uintptr_t)(pfn_start * sizeof(PhysPage)));
        uint64_t vaddr_end   = pfndb_start + PAGE_ALIGN_DOWN((uintptr_t)(pfn_end   * sizeof(PhysPage) - 1));

        size_t allocated_mem = 0;
        for (uintptr_t vaddr = vaddr_start; vaddr <= vaddr_end; vaddr += PAGE_BYTES) {
            if (vaddr < last_mapped_vpage) continue; // already set up this page
           
            // this should later be tracked so we can remap it into other address spaces (TODO)
            uintptr_t paddr = pma_palloc();
            allocated_mem += PAGE_BYTES;
            map_page((uint64_t*)(kernel_info.vmspace->cr3 + kernel_info.hhdm),
                    vaddr, paddr, PAGE_WRITE | PAGE_PRESENT);
            memset((void*) vaddr, 0, PAGE_BYTES);

            last_mapped_vpage = vaddr + PAGE_BYTES;
        }

        assert(allocated_mem < length);
        klogf(LOG_STATUS, "PFNDB done up to %x\n", end);
    }

    kernel_info.pfndb = (PhysPage*) pfndb_start;
    klogf(LOG_STATUS, "PFNDB initialised\n");
}
