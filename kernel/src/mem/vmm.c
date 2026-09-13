// mostly higher level interface virtual memory stuff (which is fully
// isa agnostic) and region tracking
#include <vmm.h>
#include <list.h>
#include <util.h>
#include <kprintf.h>
#include <pma.h>
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
    llist_init(&region->phys_regions);

    Node *rbtree_node = rbtree_insert(&vmspace->regions, 
                                      &region->rbtree_node,
                                      vaddr_start);
    if (!rbtree_node) return NULL;

    return region;
}

void vmregion_add_phys_region(VMRegion *region, uintptr_t phys_base, size_t num_pages) {
    VMPhysRegion *p_region = slab_alloc(kernel_info.vm_phys_region_cache);
    p_region->paddr     = phys_base;
    p_region->num_pages = num_pages;
    llist_insert(&region->phys_regions, &p_region->list);
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

// remaps a region from one address space into the other (same underlying physical memory).
// returns false on error
bool vmm_remap(VMSpace *dest, VMSpace *source, void *vaddr) {
    Node *region_node = rbtree_search(&source->regions, (uint64_t)vaddr);
    if (!region_node) return false;
    VMRegion *region = CONTAINER_OF(region_node, VMRegion, rbtree_node);

    VMRegion *new_region = vmregion_create(dest, region->vaddr_start, region->size_pages);

    llist_iter(&region->phys_regions, list) {
        VMPhysRegion *phys_region = CONTAINER_OF(list, VMPhysRegion, list);
        vmregion_add_phys_region(new_region, phys_region->paddr, phys_region->num_pages);
    }

    return true;
}

// TODO: we can easily vmem_free the allocated virtual memory, but we need to
// be able to also free the tracked physical regions its mapped to
void *vmm_valloc_backed(VMSpace *vmspace, size_t num_pages, uint64_t flags) {
    void *vaddr = vmem_alloc(vmspace->arena, num_pages, VMEM_INSTANTFIT);
    if (!vaddr) return NULL;

    vmregion_create(vmspace, (uintptr_t) vaddr, num_pages);

    for (size_t i = 0; i < num_pages; i++) {
        uintptr_t paddr = pma_palloc();
        map_page((uint64_t*)(vmspace->cr3 + kernel_info.hhdm), /* pml4 */
                 (uintptr_t) vaddr + i * PAGE_BYTES,           /* vaddr */
                 paddr, flags);
    }

    return vaddr;
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
    
    if (!kernel_info.vm_phys_region_cache) {
        kernel_info.vm_phys_region_cache = cache_create(sizeof(VMPhysRegion));
        if (!kernel_info.vm_phys_region_cache) kpanic("failed to create VMPhysRegion alloc cache");
    }

    VMSpace *vmspace = slab_alloc(kernel_info.vmspace_cache);

    vmspace->regions = (Tree) {0};
    vmspace->cr3 = create_address_space(); // page mapping specifics.

    // currently mappings that do *not* go into the rbtree include:
    //       - the kernel binary
    //       - direct physical memory mappings via hhdm
    // TODO: maybe consider if these should be tracked as well?

    size_t allocatable_area_size = kernel_info.hhdm/PAGE_BYTES-1 - ALLOCATABLE_BASE/PAGE_BYTES;
    vmspace->arena = vmem_arena_init(
            PAGE_BYTES, /* quantum size */
            kernel_info.vmspace_alloc_cache,
            NULL /* import alloc */, NULL /* import free */,
            NULL /* import arena */
    );
    if (!vmem_add(vmspace->arena,
             4,                    /* base (pages) */
             allocatable_area_size /* length (pages) */
    )) {
        kpanic("vmm: failed to add memory region to vmspace's vmem arena");
    }

    return vmspace;
}
