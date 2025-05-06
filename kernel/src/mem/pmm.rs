use crate::println;
use core::fmt::Write;
use limine::response::MemoryMapResponse;
use limine::memory_map::EntryType;

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

pub fn init_pmm(memmap: &MemoryMapResponse) {
    let entries = memmap.entries();
    for entry in entries {
        println!(" -> Base: 0x{:0X}, Length: 0x{:0X}, Type: {}", entry.base, entry.length, entrytype_as_str(entry.entry_type));
    }
}
