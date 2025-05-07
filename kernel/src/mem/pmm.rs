use crate::println;
use crate::kernel;
use core::fmt::Write;
use limine::memory_map::EntryType;

struct PMMNode {
    next: *mut PMMNode,
    prev: *mut PMMNode,
    size: u64,
}

const fn entrytype_as_str(e: EntryType) -> &'static str {
    match e {
        EntryType::USABLE => "Usable",
        EntryType::RESERVED => "Reserved",
        EntryType::ACPI_RECLAIMABLE => "ACPI Reclaimable",
        EntryType::ACPI_NVS => "ACPI NVS",
        EntryType::BAD_MEMORY=> "Bad Memory",
        EntryType::BOOTLOADER_RECLAIMABLE => "Bootloader Reclaimable",
        EntryType::EXECUTABLE_AND_MODULES => "Executable and Modules",
        EntryType::FRAMEBUFFER => "Framebuffer",
        _ => "Unknown"
    }
}

pub fn init_pmm(kernel: kernel::Kernel) {
    let entries = kernel.memmap.entries();
    let mut first_node: Option<*mut PMMNode> = None;
    for entry in entries {
        println!(" -> Base: 0x{:0X}, Length: 0x{:0X}, Type: {}",
            entry.base, entry.length, entrytype_as_str(entry.entry_type));
        if entry.entry_type != EntryType::USABLE { continue }
        let node = (entry.base + kernel.hhdm) as *mut PMMNode;
        match first_node {
            Some(v) => {
                // insert
                unsafe {
                    (*node).prev = (*v).prev;
                    (*node).next = v;
                    (*(*node).prev).next = node;
                    (*(*node).next).prev = node;
                }
            },
            None => {
                // init the list
                unsafe {
                    (*node).prev = node;
                    (*node).next = node;
                }
                first_node = Some(node);
            }
        }
        unsafe {
            (*node).size = entry.length;
        }
    }
    println!("PMM Allocator initialised.");
}
