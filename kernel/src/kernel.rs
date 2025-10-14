use limine::response::{MemoryMapResponse, FramebufferResponse};
use flanterm::sys::flanterm_context;
use crate::{pmm, idt, fs::vfs};

pub struct Kernel<'a> {
    pub hhdm: u64,
    pub memmap: &'a MemoryMapResponse,
    pub pmmlist: Option<*mut pmm::PMMNode>,
    pub fb: &'a FramebufferResponse,
    pub tty: Option<*mut flanterm_context>,
    pub idt: *mut idt::IDTEntry,
    pub kernel_phys_base: u64,
    pub kernel_virt_base: u64,
    pub mountpoint_list: Option<vfs::MountpointList>,
}
