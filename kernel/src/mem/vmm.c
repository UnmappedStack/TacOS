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
