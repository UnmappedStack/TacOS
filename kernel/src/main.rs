#![no_std]
#![no_main]

mod bootloader;
mod cpu;
mod drivers;
mod mem;
mod kernel;
use core::fmt::Write;
use drivers::serial;
use mem::pmm;

fn init_kernel_info() -> kernel::Kernel<'static> {
    assert!(bootloader::BASE_REVISION.is_supported());
    kernel::Kernel {
        hhdm: bootloader::HHDM_REQUEST.get_response().unwrap().offset(),
        memmap: bootloader::MEMMAP_REQUEST.get_response().unwrap(),
    }
}

#[unsafe(no_mangle)]
unsafe extern "C" fn kmain() -> ! {
    let kernel = init_kernel_info();
    serial::init_serial();
    pmm::init_pmm(kernel);
    cpu::halt_device();
}

#[panic_handler]
fn rust_panic(info: &core::panic::PanicInfo) -> ! {
    println!("Rust Panic: {}", info);
    cpu::halt_device();
}
