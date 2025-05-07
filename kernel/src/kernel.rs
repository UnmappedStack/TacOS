use limine::response::MemoryMapResponse;

pub struct Kernel<'a> {
    pub hhdm: u64,
    pub memmap: &'a MemoryMapResponse,
}
