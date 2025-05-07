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

fn init_pmm_list(list_node: *mut PMMNode, store_at: &mut Option<*mut PMMNode>) {
    unsafe {
        (*list_node).prev = list_node;
        (*list_node).next = list_node;
    }
    *store_at = Some(list_node);
}

fn pmm_list_insert(new_node: *mut PMMNode, list: *mut PMMNode) {
    unsafe {
        (*new_node).prev = (*list).prev;
        (*new_node).next = list;
        (*(*new_node).prev).next = new_node;
        (*(*new_node).next).prev = new_node;
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
            Some(v) => pmm_list_insert(node, v),
            None => init_pmm_list(node, &mut first_node),
        }
        unsafe { (*node).size = entry.length; }
    }
    println!("PMM Allocator initialised.");
}
