#![allow(dead_code)]

use crate::println;
use crate::kernel;
use core::fmt::Write;
use limine::memory_map::EntryType;

pub struct PMMNode {
    next: *mut PMMNode,
    prev: *mut PMMNode,
    size: usize,
}

fn entrytype_as_str(e: EntryType) -> &'static str {
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

fn pmm_list_remove(kernel: &mut kernel::Kernel, node: *mut PMMNode) {
    unsafe {
        (*(*node).prev).next = (*node).next;
        (*(*node).next).prev = (*node).prev;
        if kernel.pmmlist == Some(node) {
            kernel.pmmlist = Some((*node).next);
        }
    }
}

pub fn init(kernel: &mut kernel::Kernel) {
    assert!(kernel.pmmlist == None, "Cannot initialise PMM twice!");
    let entries = kernel.memmap.entries();
    let mut first_node: Option<*mut PMMNode> = None;
    for entry in entries {
        println!(" -> Base: {:p}, Length: 0x{:0x}, Type: {}",
            entry.base as *const u8,
            entry.length, entrytype_as_str(entry.entry_type));
        if entry.entry_type != EntryType::USABLE { continue }
        let node = (entry.base + kernel.hhdm) as *mut PMMNode;
        match first_node {
            Some(v) => pmm_list_insert(node, v),
            None => init_pmm_list(node, &mut first_node),
        }
        unsafe { (*node).size = entry.length as usize / 4096; }
    }
    kernel.pmmlist = first_node;
    println!("PMM Allocator initialised.");
}

pub fn valloc(kernel: &mut kernel::Kernel, num_pages: usize) -> *mut usize {
    let mut entry = kernel.pmmlist.unwrap(); 
    let first_entry = entry;
    loop {
        unsafe {
            if (*entry).size <= num_pages {
                entry = (*entry).next;
                if entry == first_entry { break }
                continue;
            }
            pmm_list_remove(kernel, entry);
            let new_entry = (entry as usize + num_pages * 4096) as *mut PMMNode;
            pmm_list_insert(new_entry, (*entry).next);
            (*new_entry).size = (*entry).size - num_pages;
            return entry as *mut usize;
        }
    }
    panic!("Out of Memory");
}

pub fn palloc(kernel: &mut kernel::Kernel, num_pages: usize) -> usize {
    valloc(kernel, num_pages) as usize - kernel.hhdm as usize
}
