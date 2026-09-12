// mostly higher level interface virtual memory stuff (which is fully
// isa agnostic) and region tracking
#include <vmm.h>
#include <kprintf.h>
#include <paging.h>
#include <mm.h>
#include <isa/cpu.h>
#include <kernel.h>

#define ALLOCATABLE_BASE 0x4000 // vaddr of base address vmem can allocate from

// return NULL on error, new VMRegion on success
VMRegion *vmregion_create(VMSpace *vmspace,
        uintptr_t vaddr_start, uint16_t size_pages) {

    VMRegion *region = slab_alloc(kernel_info.vmregion_cache);
    *region = (VMRegion) {
        .vaddr_start = vaddr_start,
        .size_pages  = size_pages,
    };

    Node *rbtree_node = rbtree_insert(&vmspace->regions, 
                                      &region->rbtree_node,
                                      vaddr_start);
    if (!rbtree_node) return NULL;

    return region;
}

/* maps a single page in physical memory as its own region in the vmm.
 *
 * I'm not sure if I should have another version which can take like a list of
 * separate physical pages to map? afterwards I'll also need a way to allocate
 * virtual pages, tie it to physical pages, and create a region from that etc
 * but this should do for now */
void vmm_map_page(VMSpace *vmspace, uintptr_t vaddr, uintptr_t paddr, uint64_t flags) {
    // map into (theoretically) architecture specific page tree structure...
    map_page((uint64_t*)(vmspace->cr3 + kernel_info.hhdm), // vaddr of page tree
            vaddr, paddr, flags);

    // ...then just add it to the VMRegion tracker
    vmregion_create(vmspace, vaddr, /* size_pages */ 1);

}

VMSpace *create_virtual_memory_space(void) {
    if (!kernel_info.vmspace_cache) {
        kernel_info.vmspace_cache = cache_create(sizeof(VMSpace));
        if (!kernel_info.vmspace_cache) kpanic("failed to create VMSpace cache");
    }
    
    if (!kernel_info.vmregion_cache) {
        kernel_info.vmregion_cache = cache_create(sizeof(VMRegion));
        if (!kernel_info.vmregion_cache) kpanic("failed to create VMRegion cache");
    }
    
    if (!kernel_info.vmspace_alloc_cache) {
        kernel_info.vmspace_alloc_cache = cache_create(sizeof(VMemArena) + sizeof(VMemOrder) * VM_ALLOC_NUM_ORDERS);
        if (!kernel_info.vmspace_alloc_cache) kpanic("failed to create VMSpace alloc cache");
    }

    VMSpace *vmspace = slab_alloc(kernel_info.vmspace_cache);

    vmspace->regions = (Tree) {0};
    vmspace->cr3 = create_address_space(); // page mapping specifics.

    // currently mappings that do *not* go into the rbtree include:
    //       - the kernel binary
    //       - direct physical memory mappings via hhdm
    // TODO: maybe consider if these should be tracked as well?

    size_t allocatable_area_size = kernel_info.hhdm/PAGE_BYTES-1 - ALLOCATABLE_BASE/PAGE_BYTES;
    (void) allocatable_area_size;
    vmspace->arena = vmem_arena_init(
            PAGE_BYTES, /* quantum size */
            kernel_info.vmspace_alloc_cache
    );
    if (!vmem_add(vmspace->arena,
             PAGE_BYTES, /* base (pages) */
             1/* length (pages) */
    )) {
        kpanic("vmm: failed to add memory region to vmspace's vmem arena");
    }

    for (int i = 0; i < 10; i++) {
        void *ptr = vmem_alloc(vmspace->arena, 1 /* size in pages */, VMEM_INSTANTFIT);
        if (!ptr) break;
        klogf(LOG_DEBUG, "got %x from vmem_alloc\n", ptr);
    }
    FREEZE_DEVICE();

    return vmspace;
}
