#![no_std]
#![no_main]

mod bootloader;
mod cpu;
mod drivers;
mod mem;
mod kernel;
mod heap;
mod utils;
mod gdt;
mod idt;
mod panic;
mod fs;
use core::{fmt::Write, ptr::null_mut};
use drivers::{serial, tty};
use fs::tempfs;
use mem::{pmm, paging};
extern crate alloc;

fn test_tempfs() {
    let fname = "testfile.txt";
    let mut fs = tempfs::new();
    let root = tempfs::openroot(&mut fs);
    println!("Opened root directory of filesystem");
    tempfs::mkdir(root, "testdir");
    println!("Created test directory within root");
    let dir = tempfs::opendir(root, "testdir");
    println!("Opened test directory");
    tempfs::mkfile(dir, fname);
    tempfs::mkfile(dir, "otherthing.txt");
    println!("Created file {}", fname);
    let f = tempfs::openfile(dir, fname);
    let msg = "Hello, world!";
    println!("Writing to {}: {}", fname, msg);
    tempfs::writefile(f, crate::utils::str_as_cstr(msg), 0, msg.len());
    tempfs::writefile(f, crate::utils::str_as_cstr("rust! :)"), 7, 9);
    let mut buf: alloc::vec::Vec<i8> = alloc::vec![0; 17];
    tempfs::readfile(f, &mut buf, 0, 17);
    println!("Read back: {}", crate::utils::cstr_as_string(buf));
    tempfs::closefile(f);
    tempfs::mkdir(dir, "anotherdir");
    println!("Listing dir:");
    let mut buf = alloc::vec![Default::default(); 3];
    tempfs::getdents(dir, &mut buf, 3);
    for i in 0..3 {
        let t = match buf[i].contents {
            tempfs::InodeContents::File(_) => "File",
            tempfs::InodeContents::Dir(_)  => "Directory",
            _ => "Invalid",
        };
        println!(" - {} found: {}", t, buf[i].fname);
    }
    tempfs::closedir(dir);
    tempfs::closedir(root);
}

fn init_kernel_info() -> kernel::Kernel<'static> {
    assert!(bootloader::BASE_REVISION.is_supported());
    let kernel_addr = bootloader::KERNEL_LOC_REQUEST.get_response().unwrap();
    kernel::Kernel {
        hhdm:    bootloader::HHDM_REQUEST.get_response().unwrap().offset(),
        memmap:  bootloader::MEMMAP_REQUEST.get_response().unwrap(),
        pmmlist: None,
        fb:      bootloader::FRAMEBUFFER_REQUEST.get_response().unwrap(),
        tty:     None,
        idt:     null_mut(),
        kernel_phys_base: kernel_addr.physical_base(),
        kernel_virt_base: kernel_addr.virtual_base(),
    }
}

#[unsafe(no_mangle)]
unsafe extern "C" fn kmain() -> ! {
    cpu::disable_interrupts();
    let mut kernel = init_kernel_info();
    serial::init();
    pmm::init(&mut kernel);
    gdt::init(&mut kernel);
    idt::init(&mut kernel);
    panic::init(&mut kernel);
    paging::init(&mut kernel);
    heap::init(&mut kernel);
    test_tempfs();
    tty::init(&mut kernel);
    tty::write(kernel.tty,
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
