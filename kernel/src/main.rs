#![no_std]
#![no_main]

mod bootloader;
mod cpu;
mod drivers;
mod mem;
use core::fmt::Write;
use drivers::serial;
use mem::pmm;

#[unsafe(no_mangle)]
unsafe extern "C" fn kmain() -> ! {
    assert!(bootloader::BASE_REVISION.is_supported());
    serial::init_serial();
    pmm::init_pmm(bootloader::MEMMAP_REQUEST.get_response().unwrap());
    cpu::halt_device();
}

#[panic_handler]
fn rust_panic(info: &core::panic::PanicInfo) -> ! {
    println!("Rust Panic: {}", info);
    cpu::halt_device();
}
