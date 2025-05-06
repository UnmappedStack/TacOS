#![no_std]
#![no_main]

mod bootloader;

use core::arch::asm;

#[unsafe(no_mangle)]
unsafe extern "C" fn kmain() -> ! {
    assert!(bootloader::BASE_REVISION.is_supported());
    hcf();
}

#[panic_handler]
fn rust_panic(_info: &core::panic::PanicInfo) -> ! {
    hcf();
}

fn hcf() -> ! {
    loop {
        unsafe {
            asm!("hlt");
        }
    }
}
