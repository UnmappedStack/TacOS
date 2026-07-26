#include <idt.h>
#include <kprintf.h>

// interrupts are basically signals that the cpu sends the kernel every time an event happens
// so that you can stop what you are doing and handle it. The IDT is a table of all the interrupts
// and pointers to the handlers.

// offset  -> a pointer to the handler
// segment -> matches to the segment which should be used to respond to it in the gdt
// flags   -> gate type, ring level, and present bit
IDTGate idt_descriptor(uint64_t offset, uint16_t segment, uint8_t flags) {
    IDTGate ret = {0};
    ret.offset1 = offset & 0xffff;
    ret.offset2 = (offset >> 16) & 0xffff;
    ret.offset3 = offset >> 32;
    ret.segment = segment;
    ret.flags   = flags;
    return ret;
}

__attribute__((interrupt))
void test_isr(void*) {
    kprintf("got interrupt call!\n");
}

// once there's smp, these will need to be stored per-cpu
static IDTGate idt[256] = {0};
static IDTR idtr;
void idt_init(void) {
    // idt[isr] will set interrupt vector <isr> to the handler described by idt_descriptor().
    // this is dynamic so we can add to this even after loading the idt.
    idt[0x80] = idt_descriptor((uint64_t)test_isr, 0x08, 0x8F);

    // the idtr is just a small table with the size and location of the idt
    // which is loaded into a register for it using the lidt instruction.
    idtr.offset = (uint64_t)idt;
    idtr.size = sizeof(IDTGate)*256-1;
    __asm__ volatile("lidt %0" : : "m"(idtr));

    kprintf("IDT init OK\n");
}

extern void divide_exception(void);
extern void debug_exception(void);
extern void breakpoint_exception(void);
extern void overflow_exception(void);
extern void bound_range_exceeded_exception(void);
extern void invalid_opcode_exception(void);
extern void device_not_avaliable_exception(void);
extern void double_fault_exception(void);
extern void coprocessor_segment_overrun_exception(void);
extern void invalid_TSS_exception(void);
extern void segment_not_present_exception(void);
extern void stack_segment_fault_exception(void);
extern void general_protection_fault_exception(void);
extern void page_fault_exception(void);
extern void floating_point_exception(void);
extern void alignment_check_exception(void);
extern void machine_check_exception(void);
extern void simd_floating_point_exception(void);
extern void virtualisation_exception(void);

void exceptions_init(void) {
    idt[0 ] = idt_descriptor((uint64_t)&divide_exception                     , 0x8, 0x8F);
    idt[1 ] = idt_descriptor((uint64_t)&debug_exception                      , 0x8, 0x8F);
    idt[3 ] = idt_descriptor((uint64_t)&breakpoint_exception                 , 0x8, 0x8F);
    idt[4 ] = idt_descriptor((uint64_t)&overflow_exception                   , 0x8, 0x8F);
    idt[5 ] = idt_descriptor((uint64_t)&bound_range_exceeded_exception       , 0x8, 0x8F);
    idt[6 ] = idt_descriptor((uint64_t)&invalid_opcode_exception             , 0x8, 0x8F);
    idt[7 ] = idt_descriptor((uint64_t)&device_not_avaliable_exception       , 0x8, 0x8F);
    idt[8 ] = idt_descriptor((uint64_t)&double_fault_exception               , 0x8, 0x8F);
    idt[9 ] = idt_descriptor((uint64_t)&coprocessor_segment_overrun_exception, 0x8, 0x8F);
    idt[10] = idt_descriptor((uint64_t)&invalid_TSS_exception                , 0x8, 0x8F);
    idt[11] = idt_descriptor((uint64_t)&segment_not_present_exception        , 0x8, 0x8F);
    idt[12] = idt_descriptor((uint64_t)&stack_segment_fault_exception        , 0x8, 0x8F);
    idt[13] = idt_descriptor((uint64_t)&general_protection_fault_exception   , 0x8, 0x8F);
    idt[14] = idt_descriptor((uint64_t)&page_fault_exception                 , 0x8, 0x8F);
    idt[16] = idt_descriptor((uint64_t)&floating_point_exception             , 0x8, 0x8F);
    idt[17] = idt_descriptor((uint64_t)&alignment_check_exception            , 0x8, 0x8F);
    idt[18] = idt_descriptor((uint64_t)&machine_check_exception              , 0x8, 0x8F);
    idt[19] = idt_descriptor((uint64_t)&simd_floating_point_exception        , 0x8, 0x8F);
    idt[20] = idt_descriptor((uint64_t)&virtualisation_exception             , 0x8, 0x8F);
}
