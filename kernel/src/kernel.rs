use limine::response::{MemoryMapResponse, FramebufferResponse};
use flanterm::sys::flanterm_context;
use crate::{pmm, idt};

pub struct Kernel<'a> {
    pub hhdm: u64,
    pub memmap: &'a MemoryMapResponse,
    pub pmmlist: Option<*mut pmm::PMMNode>,
    pub fb: &'a FramebufferResponse,
    pub tty: Option<*mut flanterm_context>,
    pub idt: *mut idt::IDTEntry,
}
