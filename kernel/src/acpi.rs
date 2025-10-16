use crate::{kernel::KERNEL/*, mem::paging*/};
/*use alloc::sync::Arc;*/

struct AcpiKernelApi;

//impl uacpi::kernel_api::KernelApi for AcpiKernelApi {
//    fn raw_memory_read(&self, phys: uacpi::PhysAddr, byte_width: u8) -> Result<u64, Status> {
//        paging::page_map_vmem(KERNEL);
//    }
//}

pub fn init() {
    let kernel = KERNEL.lock();
//    uacpi::kernel_api::set_kernel_api(Arc::new(AcpiKernelApi));
    let _s = uacpi::init(
        uacpi::PhysAddr::new(kernel.rsdp),
        uacpi::LogLevel::WARN,
        false
    );
}
