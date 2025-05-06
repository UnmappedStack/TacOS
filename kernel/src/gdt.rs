use crate::println;
use core::arch::asm;
use core::fmt::Write;
use core::mem;

unsafe extern "C" { fn reload_gdt(); }

type GDTEntry = u64;

#[repr(C, packed)]
struct GDTR {
    size: u16,
    offset: u64,
}

fn create_gdt_entry(limit: u64, base: u64, access: u64, flags: u64) -> GDTEntry {
    let mut ret: GDTEntry = 0;
    ret |= limit & 0xFFFF; // lower limit
    ret |= (limit >> 16) << 48; // upper limit
    ret |= base & 0xFFFFFF; // lower base
    ret |= (base >> 24) << 56; // upper base
    ret |= access << 40;
    ret |= flags << 52;
    ret
}
#[allow(named_asm_labels)]
fn load_gdt(gdtr: GDTR) {
    unsafe {
        // Load the new GDTR
        asm!("lgdt [rax]",
            in("rax") &raw const gdtr);
        // Reload the segment registers
        reload_gdt();
    }
}

pub fn init_gdt() {
    let mut gdt: [GDTEntry; 5] = [0; 5];
    gdt[0] = create_gdt_entry(0, 0, 0, 0);
    gdt[1] = create_gdt_entry(0xFFFFF, 0, 0x9A, 0xA);
    gdt[2] = create_gdt_entry(0xFFFFF, 0, 0x92, 0xC);
    gdt[3] = create_gdt_entry(0xFFFFF, 0, 0xFA, 0xA);
    gdt[4] = create_gdt_entry(0xFFFFF, 0, 0xF2, 0xC);
    let gdtr = GDTR {
        size: mem::size_of::<GDTEntry>() as u16 * 5 - 1,
        offset: &raw const gdt as u64,
    };
    load_gdt(gdtr);
    println!("Initiated GDT.");
}
