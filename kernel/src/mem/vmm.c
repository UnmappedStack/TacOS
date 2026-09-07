// mostly higher level interface virtual memory stuff (which is fully
// isa agnostic) and region tracking
#include <vmm.h>
#include <paging.h>
#include <isa/cpu.h>
#include <kernel.h>

// return NULL on error, new VMRegion on success
VMRegion *vmregion_create(VMSpace *vmspace,
        uintptr_t vaddr_start, uint16_t size_pages) {

    VMRegion *region = slab_alloc(kernel_info.vmregion_cache);
    *region = (VMRegion) {
        .vaddr_start = vaddr_start,
        .size_pages  = size_pages,
    };

    Node *rbtree_node = rbtree_insert(&vmspace->regions, vaddr_start);
    if (!rbtree_node) return NULL;

    rbtree_node->data = region;

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

    VMSpace *vmspace = slab_alloc(kernel_info.vmspace_cache);

    vmspace->regions = (Tree) {0};
    vmspace->cr3 = create_address_space(); // page mapping specifics.

    // currently mappings that do *not* go into the rbtree include:
    //       - the kernel binary
    //       - direct physical memory mappings via hhdm
    // TODO: maybe consider if these should be tracked as well?

    return vmspace;
}
