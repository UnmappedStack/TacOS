#![no_std]
#![no_main]

mod bootloader;
mod cpu;
mod drivers;
mod gdt;
use core::fmt::Write;
use drivers::serial;

#[unsafe(no_mangle)]
unsafe extern "C" fn kmain() -> ! {
    assert!(bootloader::BASE_REVISION.is_supported());
    serial::init_serial();
    gdt::init_gdt();
    cpu::halt_device();
}

#[panic_handler]
fn rust_panic(_info: &core::panic::PanicInfo) -> ! {
    println!("PANIC");
    cpu::halt_device();
}
