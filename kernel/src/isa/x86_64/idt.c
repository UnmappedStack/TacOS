#include <isa/x86_64/idt.h>
#include <util.h>
#include <apic.h>
#include <kernel.h>
#include <kprintf.h>
#include <smp.h>

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
    CPU *cpu = current_processor();
    if (kernel_info.schedulers.least_loaded_processor == NULL ||
            cpu->scheduler->num_threads < kernel_info.schedulers.least_loaded_processor->num_threads)
        kernel_info.schedulers.least_loaded_processor = cpu->scheduler;
    cpu->scheduler->total_ticks++;
    Thread *thread;
    if (cpu->scheduler->total_ticks % 50 == 0)
        thread = migrate_push();
    else thread = thread_select();
    const char *colours[] = {
        "\e[0;31m", // R
        "\e[0;32m", // G
        "\e[0;33m", // Y
        "\e[0;34m", // B
        "\e[0;35m", // P
    };
    if (thread != NULL)
        kprintf("%s%u\e[0m,", colours[cpu->lapic_id], thread->tid);
    end_of_interrupt();
}

__attribute__((interrupt))
void halt_isr(void*) {
    kprintf("Halt CPU%u\n", current_processor()->lapic_id);
    FREEZE_DEVICE();
}

void idt_init(void) {
    IDTR idtr;
    idtr.offset = (uint64_t)kernel_info.idt;
    idtr.size = sizeof(IDTGate)*256-1;
    __asm__ volatile("lidt %0" : : "m"(idtr));

    // lapic timer interrupt
    kernel_info.idt[40] = idt_descriptor((uint64_t) test_isr, 8, 0x8E);
    
    // halt interrupt
    kernel_info.idt[41] = idt_descriptor((uint64_t) halt_isr, 8, 0x8E);

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
    kernel_info.idt[0 ] = idt_descriptor((uint64_t)&divide_exception                     , 0x8, 0x8F);
    kernel_info.idt[1 ] = idt_descriptor((uint64_t)&debug_exception                      , 0x8, 0x8F);
    kernel_info.idt[3 ] = idt_descriptor((uint64_t)&breakpoint_exception                 , 0x8, 0x8F);
    kernel_info.idt[4 ] = idt_descriptor((uint64_t)&overflow_exception                   , 0x8, 0x8F);
    kernel_info.idt[5 ] = idt_descriptor((uint64_t)&bound_range_exceeded_exception       , 0x8, 0x8F);
    kernel_info.idt[6 ] = idt_descriptor((uint64_t)&invalid_opcode_exception             , 0x8, 0x8F);
    kernel_info.idt[7 ] = idt_descriptor((uint64_t)&device_not_avaliable_exception       , 0x8, 0x8F);
    kernel_info.idt[8 ] = idt_descriptor((uint64_t)&double_fault_exception               , 0x8, 0x8F);
    kernel_info.idt[9 ] = idt_descriptor((uint64_t)&coprocessor_segment_overrun_exception, 0x8, 0x8F);
    kernel_info.idt[10] = idt_descriptor((uint64_t)&invalid_TSS_exception                , 0x8, 0x8F);
    kernel_info.idt[11] = idt_descriptor((uint64_t)&segment_not_present_exception        , 0x8, 0x8F);
    kernel_info.idt[12] = idt_descriptor((uint64_t)&stack_segment_fault_exception        , 0x8, 0x8F);
    kernel_info.idt[13] = idt_descriptor((uint64_t)&general_protection_fault_exception   , 0x8, 0x8F);
    kernel_info.idt[14] = idt_descriptor((uint64_t)&page_fault_exception                 , 0x8, 0x8F);
    kernel_info.idt[16] = idt_descriptor((uint64_t)&floating_point_exception             , 0x8, 0x8F);
    kernel_info.idt[17] = idt_descriptor((uint64_t)&alignment_check_exception            , 0x8, 0x8F);
    kernel_info.idt[18] = idt_descriptor((uint64_t)&machine_check_exception              , 0x8, 0x8F);
    kernel_info.idt[19] = idt_descriptor((uint64_t)&simd_floating_point_exception        , 0x8, 0x8F);
    kernel_info.idt[20] = idt_descriptor((uint64_t)&virtualisation_exception             , 0x8, 0x8F);
}
