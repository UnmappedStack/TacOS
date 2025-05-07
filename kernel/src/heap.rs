use linked_list_allocator::LockedHeap;
use crate::{pmm, kernel, println};
use core::fmt::Write;

#[global_allocator]
static ALLOCATOR: LockedHeap = LockedHeap::empty();

pub fn init(kernel: &mut kernel::Kernel) {
    let heap_start = pmm::valloc(kernel, 2) as *mut u8;
    let heap_size = 4096 * 2;
    unsafe {
        ALLOCATOR.lock().init(heap_start, heap_size);
    }
    println!("Kernel heap initialised.");
}
