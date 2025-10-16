use core::{fmt::Write, mem, arch::asm, ptr::null_mut};
use crate::{println, kernel::KERNEL, pmm};

#[repr(C, packed)]
pub struct IDTEntry {
    offset0: u16,
    segment: u16,
    rsvd0: u8,
    flags: u8,
    offset1: u16,
    offset2: u32,
    rsvd1: u32,
}

#[repr(C, packed)]
struct IDTR {
    size: u16,
    offset: u64,
}

unsafe fn create_entry(offset: u64, flags: u8) -> IDTEntry {
    let offset0 = offset as u16;
    let offset1 = (offset >> 16) as u16;
    let offset2 = (offset >> 32) as u32;
    IDTEntry {
        offset0,
        segment: 0x08,
        rsvd0: 0,
        flags,
        offset1, offset2,
        rsvd1: 0,
    }
}

#[allow(dead_code)]
pub fn map_isr(vector: usize, offset: u64, flags: u8) {
    let kernel = KERNEL.lock();
    assert!(vector < 256, "ISR vector must be <256");
    assert!(kernel.idt != null_mut(), "IDT may have not yet been initialised");
    unsafe {
        *kernel.idt.add(vector) = create_entry(offset, flags);
    }
}

pub fn init() {
    let mut kernel = KERNEL.lock();
    kernel.idt = pmm::valloc(&mut *kernel, 8) as *mut IDTEntry;
    let idtr = IDTR {
        size: mem::size_of::<IDTEntry>() as u16 * 256 - 1,
        offset: kernel.idt as u64,
    };
    unsafe {
        kernel.idt.write_bytes(0, 256);
        asm!("lidt [rax]", in("rax") &raw const idtr);
    }
    println!("IDT initialised.");
}
