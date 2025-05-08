use crate::{println, kernel, idt, cpu};
use core::fmt::Write;

/* this doesn't really need to be public but I'm just keeping it public
 * to avoid warnings */
#[repr(C)]
pub struct IDTEFrame {
    cr2:   u64,
    r15:   u64,
    r14:   u64,
    r13:   u64,
    r12:   u64,
    r11:   u64,
    r10:   u64,
    r9:    u64,
    r8:    u64,
    rbp:   u64,
    rdi:   u64,
    rsi:   u64,
    rdx:   u64,
    rcx:   u64,
    rbx:   u64,
    rax:   u64,
    typ:   u64,
    code:  u64,
    rip:   u64,
    cs:    u64,
    flags: u64,
    rsp:   u64,
    ss:    u64,
}

unsafe extern "C" {
    fn divide_exception();
    fn debug_exception();
    fn breakpoint_exception();
    fn overflow_exception();
    fn bound_range_exceeded_exception();
    fn invalid_opcode_exception();
    fn device_not_avaliable_exception();
    fn double_fault_exception();
    fn coprocessor_segment_overrun_exception();
    fn invalid_tss_exception();
    fn segment_not_present_exception();
    fn stack_segment_fault_exception();
    fn general_protection_fault_exception();
    fn page_fault_exception();
    fn floating_point_exception();
    fn alignment_check_exception(); 
    fn machine_check_exception();
    fn simd_floating_point_exception();
    fn virtualisation_exception();
}

pub fn init(kernel: &mut kernel::Kernel) {
    idt::map_isr(kernel, 0, divide_exception as u64, 0x8e);
    idt::map_isr(kernel, 1, debug_exception as u64, 0x8e);
    idt::map_isr(kernel, 3, breakpoint_exception as u64, 0x8e);
    idt::map_isr(kernel, 4, overflow_exception as u64, 0x8e);
    idt::map_isr(kernel, 5, bound_range_exceeded_exception as u64, 0x8e);
    idt::map_isr(kernel, 6, invalid_opcode_exception as u64, 0x8e);
    idt::map_isr(kernel, 7, device_not_avaliable_exception as u64, 0x8e);
    idt::map_isr(kernel, 8, double_fault_exception as u64, 0x8e);
    idt::map_isr(kernel, 9, coprocessor_segment_overrun_exception as u64, 0x8e);
    idt::map_isr(kernel, 10, invalid_tss_exception as u64, 0x8e);
    idt::map_isr(kernel, 11, segment_not_present_exception as u64, 0x8e);
    idt::map_isr(kernel, 12, stack_segment_fault_exception as u64, 0x8e);
    idt::map_isr(kernel, 13, general_protection_fault_exception as u64, 0x8e);
    idt::map_isr(kernel, 14, page_fault_exception as u64, 0x8e);
    idt::map_isr(kernel, 16, floating_point_exception as u64, 0x8e);
    idt::map_isr(kernel, 17, alignment_check_exception as u64, 0x8e);
    idt::map_isr(kernel, 18, machine_check_exception as u64, 0x8e);
    idt::map_isr(kernel, 19, simd_floating_point_exception as u64, 0x8e);
    idt::map_isr(kernel, 20, virtualisation_exception as u64, 0x8e);
    println!("Exceptions initialised");
}

#[unsafe(no_mangle)]
pub fn exception_handler(_regs: IDTEFrame) -> ! {
    println!("Panic ):");
    cpu::halt_device();
}
