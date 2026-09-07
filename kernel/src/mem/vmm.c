// mostly higher level interface virtual memory stuff (which is fully
// isa agnostic) and region tracking
#include <vmm.h>
#include <paging.h>
#include <isa/cpu.h>
#include <kernel.h>

VMSpace *create_virtual_memory_space(void) {
    if (!kernel_info.vmspace_cache) {
        kernel_info.vmspace_cache = cache_create(sizeof(VMSpace));
        if (!kernel_info.vmspace_cache) kpanic("failed to create VMSpace cache");
    }

    VMSpace *vmspace = slab_alloc(kernel_info.vmspace_cache);

    vmspace->cr3 = create_address_space(); // page mapping specifics

    return vmspace;
}
