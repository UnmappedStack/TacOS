#pragma once

#if defined(__x86_64__)
    #include <isa/x86_64/gdt.h>
    #include <isa/x86_64/idt.h>
    #include <isa/x86_64/msr.h>
    #include <isa/x86_64/io.h>
    #include <isa/x86_64/apic.h>
    #include <isa/x86_64/acpi.h>
    #include <isa/x86_64/paging.h>
    #include <isa/x86_64/pit.h>
    #include <isa/x86_64/x86_64.h>
    #include <isa/x86_64/panic.h>
    #define isa_early_init() \
        do { \
            if (!cpu_has_msr()) \
                kpanic("MSRs not supported"); \
            gdt_init(); \
            idt_init(); \
        } while (0)
    #define timer_init() \
        do { \
            pit_init(); \
            init_lapic_timer(); \
        } while (0)
    #define timer_local_init() init_lapic_timer()
    #define power_management_init() acpi_init()
    #define interrupt_controller_init() apic_init()
    #define init_local_interrupt_controller(mmio_addr) init_local_apic(mmio_addr)
    #define get_limine_cpu_id(cpu) (cpu->lapic_id) // TODO: move this to bootloader specific stuff
    // (41 is defined as a halt interrupt, TODO maybe make it a macro) + move this to another file
    #define halt_all_processors() \
        do { \
            send_ipi(kernel_info.lapic_addr, 41, IPI_DELIVERY_FIXED | \
                                                 IPI_DESTINATION_PHYSICAL | \
                                                 IPI_LEVEL_DEASSERT | \
                                                 IPI_TRIGGER_EDGE | \
                                                 IPI_DEST_SHORTHAND_ALL_EXCEPT_SELF); \
        } while(0)
    #define PAUSE() __builtin_ia32_pause()
#elif defined(__riscv)
    static_assert(__riscv_xlen == 64 && "64 bit is supported");
    // there's a lot of stubs here that all are meaningless. TODO: actually do riscv64 stuff
    #include <isa/riscv64/riscv64.h>
    #include <isa/riscv64/sbi.h>
    #include <isa/riscv64/interrupts.h>
    #include <isa/riscv64/csr.h>
    #include <isa/riscv64/panic.h>
    #define PAUSE() __builtin_riscv_pause()
    #define isa_early_init() \
        do { \
            interrupts_init(); \
        } while(0)
    #define exceptions_init() {}
    #define SWITCH_PAGE_TREE(cr3) FREEZE_DEVICE()
    #define SWITCH_STACK(stack_top) FREEZE_DEVICE()
    #define power_management_init() FREEZE_DEVICE()
    #define interrupt_controller_init() FREEZE_DEVICE()
    #define timer_init() FREEZE_DEVICE()
    #define set_current_cpu_info(cpu_info) FREEZE_DEVICE()
    #define init_local_interrupt_controller(mmio_addr) FREEZE_DEVICE()
    #define timer_local_init() FREEZE_DEVICE()
    #define IO_WAIT() FREEZE_DEVICE()
    #define get_limine_cpu_id(cpu) (cpu->hartid) // TODO: move this to bootloader specific stuff
    #define get_current_cpu_info() (0)
    #define halt_all_processors() FREEZE_DEVICE()
    #define PAGE_PRESENT 0
    #define PAGE_WRITE 0
    #define PAGE_USER 0
    #define outb(port, b) \
        do { \
            (void)b; \
            FREEZE_DEVICE(); \
        } while (0)
    #define inb(port) (0)
#endif
