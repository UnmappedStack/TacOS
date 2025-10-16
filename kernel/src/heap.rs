use linked_list_allocator::LockedHeap;
use crate::{pmm, println, kernel::KERNEL};
use core::fmt::Write;

#[global_allocator]
static ALLOCATOR: LockedHeap = LockedHeap::empty();

pub fn init() {
    let mut kernel = KERNEL.lock();
    let heap_start = pmm::valloc(&mut *kernel, 2) as *mut u8;
    let heap_size = 4096 * 2;
    unsafe {
        ALLOCATOR.lock().init(heap_start, heap_size);
    }
    println!("Kernel heap initialised.");
}
