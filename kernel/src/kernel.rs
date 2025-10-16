use limine::response::{MemoryMapResponse, FramebufferResponse};
use flanterm::sys::flanterm_context;
use crate::{pmm, idt, fs::vfs};
use spin::{Lazy, Mutex};

#[derive(Default)]
pub struct Kernel<'a> {
    pub hhdm: u64,
    pub memmap: Option<&'a MemoryMapResponse>,
    pub pmmlist: Option<*mut pmm::PMMNode>,
    pub fb: Option<&'a FramebufferResponse>,
    pub tty: Option<*mut flanterm_context>,
    pub idt: *mut idt::IDTEntry,
    pub kernel_phys_base: u64,
    pub kernel_virt_base: u64,
    pub mountpoint_list: Option<vfs::MountpointList>,
    pub rsdp: u64,
    pub current_pml4: *mut u64,
}
unsafe impl Send for Kernel<'_> {}
unsafe impl Sync for Kernel<'_> {}
    
pub static KERNEL: Lazy<Mutex<Kernel>> = Lazy::new(|| Mutex::new(Kernel::default()));
