use crate::{println, pmm, kernel::KERNEL};
use core::{arch::asm, fmt::Write, mem};

unsafe extern "C" { fn reload_gdt(); }

type GDTEntry = u64;

#[repr(C, packed)]
struct GDTR {
    size: u16,
    offset: u64,
}

const fn create_entry(limit: u64, base: u64, access: u64, flags: u64) -> GDTEntry {
    let mut ret: GDTEntry = 0;
    let limit1 = limit & 0xFFFF;
    let limit2 = (limit >> 16) & 0b1111;
    let base1  = base & 0xFFFF;
    let base2  = (base >> 16) & 0xFF;
    let base3  = (base >> 24) & 0xFF;
    ret |= limit1;
    ret |= base1 << 16;
    ret |= base2 << 32;
    ret |= access << 40;
    ret |= (limit2 & 0xF) << 48;
    ret |= (flags & 0xF) << 52;
    ret |= base3 << 56;
    ret
}

fn load(gdtr: GDTR) {
    unsafe {
        // Load the new GDTR
        asm!("lgdt [rax]",
            in("rax") &raw const gdtr);
        // Reload the segment registers
        reload_gdt();
    }
}

pub fn init() {
    let mut kernel = KERNEL.lock();
    let gdt = pmm::valloc(&mut *kernel, 1) as *mut GDTEntry;
    unsafe {
        *gdt.add(0) = create_entry(0, 0, 0, 0);
        *gdt.add(1) = create_entry(0, 0, 0x9A, 0x2);
        *gdt.add(2) = create_entry(0, 0, 0x92, 0x0);
        *gdt.add(3) = create_entry(0, 0, 0xFA, 0x2);
        *gdt.add(4) = create_entry(0, 0, 0xF2, 0x0);
    }
    let gdtr = GDTR {
        size: mem::size_of::<GDTEntry>() as u16 * 5 - 1,
        offset: gdt as u64,
    };
    load(gdtr);
    println!("GDT initialised.");
}
