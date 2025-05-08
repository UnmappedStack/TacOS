use crate::{kernel, println, pmm};
use core::{fmt::Write};

const   PAGE_WRITE: u64 = 0b001;
const PAGE_PRESENT: u64 = 0b010;
const   _PAGE_USER: u64 = 0b100;
const   PADDR_MASK: u64 = !(0xfff | (1 << 63));

const fn addr_to_idx(addr: u64, level: u8) -> usize {
    ((addr >> (9 * (level as u64) + 12)) & 0x1ff) as usize
}

/* Maps a virtual address to a physical address with paging with a 
 * specific page tree pml4 provided.
 * This isn't the fastest, I'm aware, it's optimised for readability 
 * rather than speed. */
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe fn page_map_vmem(kernel: &mut kernel::Kernel, pml4: *mut u64,
                                    paddr: u64, vaddr: u64, flags: u64) {
    let pml1idx = addr_to_idx(vaddr, 1);
    let pml2idx = addr_to_idx(vaddr, 2);
    let pml3idx = addr_to_idx(vaddr, 3);
    let pml4idx = addr_to_idx(vaddr, 4);
    if *pml4.add(pml4idx) == 0 {
        let paddr = pmm::palloc(kernel, 1) as u64;
        *pml4.add(pml4idx) = PAGE_WRITE | PAGE_PRESENT | paddr;
        ((paddr + kernel.hhdm) as *mut u64).write_bytes(0, 512);
    }
    let pml3 = ((*pml4.add(pml4idx) & PADDR_MASK) + kernel.hhdm) as *mut u64;
    if *pml3.add(pml3idx) == 0 {
        let paddr = pmm::palloc(kernel, 1) as u64;
        *pml3.add(pml3idx) = PAGE_WRITE | PAGE_PRESENT | paddr;
        ((paddr + kernel.hhdm) as *mut u64).write_bytes(0, 512);
    }
    let pml2 = ((*pml3.add(pml3idx) & PADDR_MASK) + kernel.hhdm) as *mut u64;
    if *pml2.add(pml2idx) == 0 {
        let paddr = pmm::palloc(kernel, 1) as u64;
        *pml2.add(pml2idx) = PAGE_WRITE | PAGE_PRESENT | paddr;
        ((paddr + kernel.hhdm) as *mut u64).write_bytes(0, 512);
    }
    let pml1 = ((*pml2.add(pml2idx) & PADDR_MASK) + kernel.hhdm) as *mut u64;
    *pml1.add(pml1idx) = paddr | flags;
}

pub fn init(kernel: &mut kernel::Kernel) {
    let pml4 = pmm::valloc(kernel, 1) as *mut u64;
    /* map a test page (this isn't yet actually loaded into cr3) */
    unsafe {
        pml4.write_bytes(0, 512);
        page_map_vmem(kernel, pml4, 0, kernel.hhdm,
                            PAGE_WRITE | PAGE_PRESENT);
    }
    println!("Page tree initialised.");
}
