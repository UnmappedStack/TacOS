#![no_std]
#![no_main]

mod bootloader;
mod acpi;
mod err;
mod cpu;
mod drivers;
mod mem;
mod kernel;
mod heap;
mod gdt;
mod utils;
mod idt;
mod panic;
mod fs;
use core::{fmt::Write, ptr::null_mut};
use drivers::{serial, tty};
use fs::vfs;
use kernel::KERNEL;
use mem::{pmm, paging};
extern crate alloc;

fn init_kernel_info() -> kernel::Kernel<'static> {
    assert!(bootloader::BASE_REVISION.is_supported());
    let kernel_addr = bootloader::KERNEL_LOC_REQUEST.get_response().unwrap();
    kernel::Kernel {
        hhdm:    bootloader::HHDM_REQUEST.get_response().unwrap().offset(),
        memmap:  Some(bootloader::MEMMAP_REQUEST.get_response().unwrap()),
        pmmlist: None,
        fb:      Some(bootloader::FRAMEBUFFER_REQUEST.get_response().unwrap()),
        tty:     None,
        idt:     null_mut(),
        kernel_phys_base: kernel_addr.physical_base(),
        kernel_virt_base: kernel_addr.virtual_base(),
        mountpoint_list: None,
        rsdp:   bootloader::KERNEL_RSDP_REQUEST.get_response().unwrap().address() as u64,
        current_pml4: null_mut(),
    }
}

#[unsafe(no_mangle)]
unsafe extern "C" fn kmain() -> ! {
    cpu::disable_interrupts();
    *KERNEL.lock() = init_kernel_info();
    serial::init();
    pmm::init();
    gdt::init();
    idt::init();
    panic::init();
    paging::init();
    heap::init();
    vfs::init();
    acpi::init();
    tty::init();
    tty::write(KERNEL.lock().tty,
        "\x1B[1;32mKernel initiation complete \x1B[22;39m\
         (see serial for logs)\n\
         \n\x1B[1;33mWARNING\x1B[22;39m: You are in the Rust rewrite of TacOS, \
         which is not as complete as the original version written in C (and \
         doesn't have the Doom port).\nIf you want a \
         semi-functioning system, switch to the main branch then build \
         and run again.");
    cpu::halt_device();
}

#[panic_handler]
fn rust_panic(info: &core::panic::PanicInfo) -> ! {
    println!("Rust Panic: {}", info);
    cpu::halt_device();
}
