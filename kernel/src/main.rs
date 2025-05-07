#![no_std]
#![no_main]

mod bootloader;
mod cpu;
mod drivers;
mod mem;
mod kernel;
mod tty;
mod heap;
mod utils;
use core::fmt::Write;
use drivers::serial;
use mem::pmm;
extern crate alloc;

fn init_kernel_info() -> kernel::Kernel<'static> {
    assert!(bootloader::BASE_REVISION.is_supported());
    kernel::Kernel {
        hhdm: bootloader::HHDM_REQUEST.get_response().unwrap().offset(),
        memmap: bootloader::MEMMAP_REQUEST.get_response().unwrap(),
        pmmlist: None, // not initialised yet
        fb: bootloader::FRAMEBUFFER_REQUEST.get_response().unwrap(),
        tty: None,
    }
}

#[unsafe(no_mangle)]
unsafe extern "C" fn kmain() -> ! {
    let mut kernel = init_kernel_info();
    serial::init();
    pmm::init(&mut kernel);
    heap::init(&mut kernel);
    tty::init(&mut kernel);
    tty::write(kernel.tty, "Kernel initiation complete (see serial for logs)");
    cpu::halt_device();
}

#[panic_handler]
fn rust_panic(info: &core::panic::PanicInfo) -> ! {
    println!("Rust Panic: {}", info);
    cpu::halt_device();
}
