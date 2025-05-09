use crate::{kernel, println, pmm};
use core::{fmt::Write};
use limine::memory_map::EntryType;

const   PAGE_WRITE: u64 = 0b001;
const PAGE_PRESENT: u64 = 0b010;
const   _PAGE_USER: u64 = 0b100;
const   PADDR_MASK: u64 = !(0xfff | (1 << 63));

fn page_align_down(addr: u64) -> usize {
    ((addr / 4096) * 4096) as usize
}

fn page_align_up(addr: u64) -> usize {
    (((addr + 4095) / 4096) * 4096) as usize
}

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

/* Again I'm aware this isn't very fast, it's for readability not speed */
pub unsafe fn map_consecutive_pages(kernel: &mut kernel::Kernel,
                                    pml4: *mut u64,
                                    paddr_start: u64, vaddr_start: u64,
                                    num_pages: usize, flags: u64) {
    for i in 0..num_pages {
        unsafe {
            page_map_vmem(kernel, pml4,
                            paddr_start + ((i * 4096) as u64),
                            vaddr_start + ((i * 4096) as u64),
                            flags);
        }
    }
}

unsafe extern "C" {
    static ld_kernel_ro_start: [u64; 1];
    static ld_kernel_writable_start: [u64; 1];
    static ld_kernel_end: [u64; 1];
}
fn map_kernel(kernel: &mut kernel::Kernel, pml4: *mut u64) {
    let kernel_ro_start       = (&raw const ld_kernel_ro_start) as u64;
    let kernel_writable_start = (&raw const ld_kernel_writable_start) as u64;
    let kernel_end            = (&raw const ld_kernel_end) as u64;
    println!("Kernel ELF layout:");
    println!("\t- RO start       = {:p}\n\
              \t- Writable start = {:p}\n\
              \t- Kernel end     = {:p}",
              kernel_ro_start       as *const i8,
              kernel_writable_start as *const i8,
              kernel_end            as *const i8);
    unsafe {
        let len = page_align_up(kernel_writable_start - kernel_ro_start) / 4096;
        let phys = kernel.kernel_phys_base +
                                    (kernel_ro_start - kernel.kernel_virt_base);
        map_consecutive_pages(kernel, pml4, phys, kernel_ro_start,
                                                        len, PAGE_PRESENT);
    }
    unsafe {
        let len = page_align_up(kernel_end - kernel_writable_start) / 4096;
        let phys = kernel.kernel_phys_base +
                              (kernel_writable_start - kernel.kernel_virt_base);
        map_consecutive_pages(kernel, pml4, phys, kernel_ro_start,
                                                len, PAGE_PRESENT | PAGE_WRITE);
    }
}

pub fn init(kernel: &mut kernel::Kernel) {
    let pml4 = pmm::valloc(kernel, 1) as *mut u64;
    unsafe {
        pml4.write_bytes(0, 512);
    }
    let entries = kernel.memmap.entries();
    for entry in entries {
        if entry.entry_type == EntryType::RESERVED   ||
           entry.entry_type == EntryType::BAD_MEMORY {
            continue;
        }
        unsafe {
            map_consecutive_pages(kernel, pml4, entry.base,
                                    entry.base + kernel.hhdm,
                                    (entry.length / 4096) as usize,
                                    PAGE_WRITE | PAGE_PRESENT);
        }
    }
    map_kernel(kernel, pml4);
    println!("Page tree initialised.");
}
