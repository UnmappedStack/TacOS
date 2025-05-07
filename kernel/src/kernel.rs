use limine::response::MemoryMapResponse;
use crate::pmm;

pub struct Kernel<'a> {
    pub hhdm: u64,
    pub memmap: &'a MemoryMapResponse,
    pub pmmlist: Option<*mut pmm::PMMNode>,
}
