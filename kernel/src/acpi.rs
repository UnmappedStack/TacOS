use crate::{kernel::KERNEL, cpu};
use alloc::sync::Arc;

const BYTE0_MASK: u32 = 0b1111;
const BYTE1_MASK: u32 = BYTE0_MASK << 8;
const BYTE2_MASK: u32 = BYTE0_MASK << 16;
const BYTE3_MASK: u32 = BYTE0_MASK << 24;

const BYTE_MASKS: [u32; 4] = [BYTE0_MASK, BYTE1_MASK, BYTE2_MASK, BYTE3_MASK];

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
    unsafe fn pci_read(&self, address: uacpi::PCIAddress, offset: usize, byte_width: u8)
                                                -> Result<u64, uacpi::Status> {
        if byte_width > 4 { return Err(uacpi::Status::InvalidArgument); }
        let offset_aligned = (offset & !0b11) as u32;
        let bus  = address.bus()      as u32;
        let dev  = address.device()   as u32;
        let func = address.function() as u32;
        let addr = (bus << 16) | (dev << 11) | 
            (func << 8) | (offset_aligned & 0xFC) | (0x80000000) as u32;
        unsafe {
            cpu::outd(0xCF8, addr);
            let read_back = cpu::ind(0xCFC);
            let ret = (read_back >> (byte_width-1*8)) & 0b1111;
            Ok(ret as u64)
        }
    }
    unsafe fn pci_write(&self, address: uacpi::PCIAddress, offset: usize, byte_width: u8, val: u64)
                                                -> Result<(), uacpi::Status> {
        if byte_width > 4 { return Err(uacpi::Status::InvalidArgument); }
        let offset_aligned = (offset & !0b11) as u32;
        let bus  = address.bus()      as u32;
        let dev  = address.device()   as u32;
        let func = address.function() as u32;
        let addr = (bus << 16) | (dev << 11) | 
            (func << 8) | (offset_aligned & 0xFC) | (0x80000000) as u32;
        unsafe {
            // read to get current value
            cpu::outd(0xCF8, addr);
            let current = cpu::ind(0xCFC);
            // write back
            let mask = BYTE_MASKS[(byte_width-1) as usize];
            let to_write = (current & !mask) | ((val as u32) << ((byte_width-1)*8));
            cpu::outd(0xCF8, addr);
            cpu::outd(0xCFC, to_write);
        }
        Ok(())
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
