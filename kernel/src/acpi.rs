use crate::{kernel::KERNEL, cpu};
use alloc::sync::Arc;

struct AcpiKernelApi;

impl uacpi::kernel_api::KernelApi for AcpiKernelApi {
    unsafe fn raw_memory_read(&self, phys: uacpi::PhysAddr, byte_width: u8)
                                                -> Result<u64, uacpi::Status> {
        let kernel = KERNEL.lock();
        unsafe {
            match byte_width {
                1 => Ok(*((phys.as_u64() + kernel.hhdm) as *const  u8) as u64),
                2 => Ok(*((phys.as_u64() + kernel.hhdm) as *const u16) as u64),
                4 => Ok(*((phys.as_u64() + kernel.hhdm) as *const u32) as u64),
                8 => Ok(*((phys.as_u64() + kernel.hhdm) as *const u64) as u64),
                _ => Err(uacpi::Status::InvalidArgument),
            }
        }
    }
    unsafe fn raw_memory_write(&self, phys: uacpi::PhysAddr, byte_width: u8, val: u64)
                                                -> Result<(), uacpi::Status> {
        let kernel = KERNEL.lock();
        unsafe {
            match byte_width {
                1 => Ok(*((phys.as_u64() + kernel.hhdm) as *mut  u8) = val as u8 ),
                2 => Ok(*((phys.as_u64() + kernel.hhdm) as *mut u16) = val as u16),
                4 => Ok(*((phys.as_u64() + kernel.hhdm) as *mut u32) = val as u32),
                8 => Ok(*((phys.as_u64() + kernel.hhdm) as *mut u64) = val as u64),
                _ => Err(uacpi::Status::InvalidArgument),
            }
        }
    }
    unsafe fn raw_io_read(&self, addr: uacpi::IOAddr, byte_width: u8)
                                                -> Result<u64, uacpi::Status> {
        unsafe {
            match byte_width {
                1 => Ok(cpu::inb(addr.as_u64() as u16) as u64),
                2 => Ok(cpu::inw(addr.as_u64() as u16) as u64),
                4 => Ok(cpu::ind(addr.as_u64() as u16) as u64),
                _ => Err(uacpi::Status::InvalidArgument),
            }
        }
    }
    unsafe fn raw_io_write(&self, addr: uacpi::IOAddr, byte_width: u8, val: u64)
                                                -> Result<(), uacpi::Status> {
        unsafe {
            match byte_width {
                1 => {cpu::outb(addr.as_u64() as u16, val as u8 ); Ok(())},
                2 => {cpu::outw(addr.as_u64() as u16, val as u16); Ok(())},
                4 => {cpu::outd(addr.as_u64() as u16, val as u32); Ok(())},
                _ => Err(uacpi::Status::InvalidArgument),
            }
        }
    }
}

pub fn init() {
    let kernel = KERNEL.lock();
    uacpi::kernel_api::set_kernel_api(Arc::new(AcpiKernelApi));
    let _s = uacpi::init(
        uacpi::PhysAddr::new(kernel.rsdp),
        uacpi::LogLevel::WARN,
        false
    );
}
