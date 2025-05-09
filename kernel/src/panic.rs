use crate::{println, kernel, idt, cpu, serial};
use core::{fmt::Write, ptr::null_mut};

/* this doesn't really need to be public but I'm just keeping it public
 * to avoid warnings */
#[repr(C)]
pub struct IDTEFrame {
    cr3:   u64,
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

#[repr(C)]
struct StackFrame {
    rbp: *const StackFrame,
    rip: u64,
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

#[allow(dead_code)]
pub fn cause_exception() {
    unsafe {
        let ptr = 1 as *mut i8;
        *ptr = 1;
    }
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
    println!("Exceptions initialised.");
}

/* sorry for the >80 character lines :( */
fn regdump(regs: &IDTEFrame) {
    println!("CR2: {:018p}\n", regs.cr2 as *const u8);
    println!("RSP: {:018p} | RAX: {:018p}", regs.rsp as *const u8, regs.rax as *const u8);
    println!("RBP: {:018p} | RBX: {:018p}", regs.rbp as *const u8, regs.rbx as *const u8);
    println!("CR2: {:018p} | RCX: {:018p}", regs.cr2 as *const u8, regs.rcx as *const u8);
    println!("RDI: {:018p} | RDX: {:018p}", regs.rdi as *const u8, regs.rdx as *const u8);
    println!("RSI: {:018p} | CR3: {:018p}", regs.rsi as *const u8, regs.cr3 as *const u8);
    println!(" R8: {:018p} |  R9: {:018p}", regs.r8  as *const u8, regs.r9  as *const u8);
}

fn stack_trace(rbp: u64, rip: u64) {
    println!("\nStack Trace (Most recent call first):");
    println!(" {:p}", rip as *const u8);
    let mut stack = rbp as *const StackFrame;
    unsafe {
        while stack != null_mut() {
            println!(" {:018p}", (*stack).rip as *const u8);
            if (*(*stack).rbp).rip == (*stack).rip {
                println!(" ...recursive call");
                return;
            }
            stack = (*stack).rbp;
        }
    }
}

static mut IN_PANIC: bool = false;

#[unsafe(no_mangle)]
unsafe extern "C" fn exception_handler(regs: IDTEFrame) -> ! {
    unsafe {
        if IN_PANIC { cpu::halt_device(); }
        IN_PANIC = true;
    }
    println!("");
    for _ in 0..25 {
        serial::writechar(' ');
    }
    println!("\x1B[1;33m### KERNEL PANIC ###\x1B[31m");
    println!("  Something went seriously wrong and \
        the system cannot continue.\n\x1B[39m");
    println!(" === Debug Information: ===\x1B[22m");
    match regs.typ {
        13 => println!("Exception type: General protection fault"),
        14 => println!("Exception type: Page fault"),
        _  => println!("Exception type: {}", regs.typ),
    };
    println!("Error code: 0b{:b}", regs.code as u16);
    regdump(&regs);
    println!("Exception occurred in ring {}", regs.ss & 0b11);
    stack_trace(regs.rbp, regs.rip);
    println!("");
    cpu::halt_device();
}
