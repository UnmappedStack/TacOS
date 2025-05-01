#include <apic.h>
#include <io.h>
#include <pit.h>
#include <locks.h>
#include <cpu.h>
#include <printf.h>
#include <kernel.h>
#include <pit.h>

uint32_t read_lapic(uintptr_t lapic_addr, uint64_t reg_offset) {
    uint32_t volatile *lapic_register_addr = (uint32_t volatile *) (((uint64_t)lapic_addr) + reg_offset);
    return (uint32_t) *lapic_register_addr;
}

void write_lapic(uintptr_t lapic_addr, uint64_t reg_offset, uint32_t val) {
    uint32_t volatile *lapic_register_addr = (uint32_t volatile *) (((uint64_t)lapic_addr) + reg_offset);
    *lapic_register_addr = val;
}

uint32_t read_ioapic(void *ioapic_addr, uint32_t reg) {
   uint32_t volatile *ioapic = (uint32_t volatile *)ioapic_addr;
   ioapic[0] = (reg & 0xff);
   return ioapic[4];
}

void write_ioapic(void *ioapic_addr, uint32_t reg, uint32_t value) {
   uint32_t volatile *ioapic = (uint32_t volatile *)ioapic_addr;
   ioapic[0] = (reg & 0xff);
   ioapic[4] = value;
}

void map_ioapic(uint8_t vec, uint32_t irq, uint32_t lapic_id, bool polarity, bool trigger) {
    printf("Global system interrupt base: %i\n", kernel.ioapic_device.global_system_interrupt_base);
    uintptr_t ioapic_addr = (uintptr_t) (((uint64_t) kernel.ioapic_device.ioapic_addr) + kernel.hhdm);
    uint32_t gsi_base = kernel.ioapic_device.global_system_interrupt_base;
    uint32_t entry_num = gsi_base + (irq * 2);
    printf("Entry number: %i\n", entry_num);
    uint32_t reg_nums[2] = {0x10 + entry_num, 0x11 + entry_num};
    printf("Register numbers: %i and %i\n", reg_nums[0], reg_nums[1]);
    uint32_t redirection_entries[2] = {read_ioapic((void*) ioapic_addr, reg_nums[0]), read_ioapic((void*) ioapic_addr, reg_nums[1])};
    printf("Original redirection entries: 0x%x and 0x%x\n", redirection_entries[0], redirection_entries[1]);
    printf("Trying to set entry one...\n");
    redirection_entries[0] = (redirection_entries[0] & ~0xFF) | vec; // set vector number
    redirection_entries[0] &= ~0x700; // set delivery mode to normal
    redirection_entries[0] &= ~0x800; // set destination mode to physical. Probably worse but for now it's just easier.
    if (polarity)
        redirection_entries[0] |= 0x2000; // set polarity to low
    else
        redirection_entries[0] &= ~0x2000; // set polarity to high
    if (trigger)
        redirection_entries[0] |= 0x8000; // set trigger to level
    else
        redirection_entries[0] &= ~0x8000; // set trigger to edge
    redirection_entries[0] &= ~0x10000; // makes sure that it's unmasked
    printf("Done! New value: 0x%x\n", redirection_entries[0]);
    printf("Trying to set entry two...\n");
    redirection_entries[1] = (lapic_id & 0xF) << 28;
    printf("Done, new value: 0x%x\n", redirection_entries[1]);
    printf("Trying to set new entries...\n");
    write_ioapic((void*) ioapic_addr, reg_nums[0], redirection_entries[0]);
    write_ioapic((void*) ioapic_addr, reg_nums[1], redirection_entries[1]);
    printf("Done, this IOAPIC IRQ has been mapped and unmasked.\n");
}

void mask_ioapic(uint8_t irq, uint32_t lapic_id) {
    (void) lapic_id;
    uintptr_t ioapic_addr = (uintptr_t) (((uint64_t) kernel.ioapic_device.ioapic_addr) + kernel.hhdm);
    uint32_t gsi_base = kernel.ioapic_device.global_system_interrupt_base;
    uint32_t entry_num = gsi_base + (irq * 2);
    uint32_t reg_num = 0x10 + entry_num;
    uint32_t redirection_entry = read_ioapic((void*) ioapic_addr, reg_num);
    redirection_entry |= 0x10000;
    write_ioapic((void*)ioapic_addr, reg_num, redirection_entry);
}

void unmask_ioapic(uint8_t irq, uint32_t lapic_id) {
    (void) lapic_id;
    uintptr_t ioapic_addr = (uintptr_t) (((uint64_t) kernel.ioapic_device.ioapic_addr) + kernel.hhdm);
    uint32_t gsi_base = kernel.ioapic_device.global_system_interrupt_base;
    uint32_t entry_num = gsi_base + (irq * 2);
    uint32_t reg_num = 0x10 + entry_num;
    uint32_t redirection_entry = read_ioapic((void*) ioapic_addr, reg_num);
    redirection_entry &= ~0x10000;
    write_ioapic((void*)ioapic_addr, reg_num, redirection_entry);
}


void init_local_apic(uintptr_t lapic_addr) {
    printf("Local APIC vaddr: 0x%x\n", lapic_addr);
    printf("Setting task priority of LAPIC...\n");
    write_lapic(lapic_addr, LAPIC_TASK_PRIORITY_REGISTER, 0);
    printf("Setting LAPIC destination format to flat mode...\n");
    write_lapic(lapic_addr, LAPIC_DESTINATION_FORMAT_REGISTER, 0xF0000000);
    printf("Setting spurious interrupt vector (and enabling this LAPIC)...\n");
    printf("Low value: 0b%b, high value: 0b%b\n", 0xFF, 0x100);
    write_lapic(lapic_addr, LAPIC_SPURIOUS_INTERRUPT_VECTOR_REGISTER, 0xFF | 0x100);
    printf("This LAPIC was successfully set up!\n");
}

Spinlock current_cpu_lock;

uint64_t get_current_processor() {
    spinlock_aquire(&current_cpu_lock);
    uint64_t to_return = read_lapic(kernel.lapic_addr, LAPIC_ID_REGISTER) >> 24;
    spinlock_release(&current_cpu_lock);
    return to_return;
}

void init_lapic_timer() {
    uintptr_t lapic_addr = kernel.lapic_addr;
    printf("Initiating LAPIC timer...\n");
    printf("Got LAPIC registers address (vmem): 0x%x\n", lapic_addr);
    write_lapic(lapic_addr, LAPIC_TIMER_INITIAL_COUNT_REGISTER, 0);
    write_lapic(lapic_addr, LAPIC_TIMER_DIVIDER_REGISTER, 3);
    write_lapic(lapic_addr, LAPIC_TIMER_INITIAL_COUNT_REGISTER, 0xFFFFFFFF);
    pit_wait(10); // wait & calibrate to 10 ms
    printf("Back here, reading from lapic...\n");
    uint32_t current_count = read_lapic(lapic_addr, LAPIC_TIMER_CURRENT_COUNT_REGISTER);
    printf("done, Writing to lapic...\n");
    write_lapic(lapic_addr, LAPIC_TIMER_INITIAL_COUNT_REGISTER, 0);
    uint32_t num_ticks = 0xFFFFFFFF - current_count;
    printf("Previous LVT register value: 0b%b\n", read_lapic(lapic_addr, LAPIC_TIMER_LVT_REGISTER));
    write_lapic(lapic_addr, LAPIC_TIMER_LVT_REGISTER, 40 | 0x20000);
    printf("New LVT register value: 0b%b\n", read_lapic(lapic_addr, LAPIC_TIMER_LVT_REGISTER));
    printf("Error status register value: 0b%x\n", read_lapic(lapic_addr, LAPIC_ERROR_STATUS_REGISTER));
    write_lapic(lapic_addr, LAPIC_TIMER_DIVIDER_REGISTER, 3);
    printf("Num ticks: 0x%x\n", num_ticks);
    printf("Current count: 0x%x\n", current_count);
    write_lapic(lapic_addr, LAPIC_TIMER_INITIAL_COUNT_REGISTER, num_ticks);
    printf("New count: 0x%x\n", read_lapic(lapic_addr, LAPIC_TIMER_LVT_REGISTER));
}

void lock_lapic_timer() {
    write_lapic(kernel.lapic_addr, LAPIC_TIMER_LVT_REGISTER, read_lapic(kernel.lapic_addr, LAPIC_TIMER_LVT_REGISTER) & ~0x20000);
}

void unlock_lapic_timer() {
    write_lapic(kernel.lapic_addr, LAPIC_TIMER_LVT_REGISTER, read_lapic(kernel.lapic_addr, LAPIC_TIMER_LVT_REGISTER) | 0x20000);
}

void end_of_interrupt() {
    write_lapic(kernel.lapic_addr, LAPIC_END_OF_INTERRUPT_REGISTER, 0);
}

bool verify_apic() {
   uint32_t eax, edx;
   CPUID(1, &eax, &edx);
   return edx & (1 << 9);
}

// sorry for the bulky name
void map_apic_into_task(uint64_t task_cr3_phys) {
    map_pages((uint64_t*) (task_cr3_phys + kernel.hhdm), (uint64_t) kernel.ioapic_addr + kernel.hhdm, (uint64_t) kernel.ioapic_addr, 1, KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_WRITE);
    map_pages((uint64_t*) (task_cr3_phys + kernel.hhdm), (uint64_t) kernel.lapic_addr, (uint64_t) kernel.lapic_addr - kernel.hhdm, 1, KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_WRITE);
}

void init_apic() {
    printf("Initiating APIC...\n");
    printf("Checking that APIC is avaliable...\n");
    if (verify_apic()) {
        printf("Success, APIC is avaliable, setting it up now.\n");
    } else {
        printf("This device does not support APIC, but rather only legacy PIC. Halting.\n");
        HALT_DEVICE();
    }
    // disable pic
    outb(0x21, 0xff);
    outb(0xA1, 0xff);
    MADT *madt = (MADT*) find_MADT(kernel.rsdt);
    if (!madt) {
        printf("MADT entry not found in ACPI tables, halting.\n");
        HALT_DEVICE();
    }
    printf("MADT at %p\n", madt);
    printf("Local APIC paddr: 0x%x\n", madt->local_apic_addr);
    // map the lapic addr
    map_pages((uint64_t*) (kernel.cr3 + kernel.hhdm), (uint64_t) madt->local_apic_addr + kernel.hhdm, (uint64_t) madt->local_apic_addr, 1, KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_WRITE);
    uint64_t lapic_registers_virt = (uint64_t) madt->local_apic_addr + kernel.hhdm;
    kernel.lapic_addr = lapic_registers_virt;
    init_local_apic(lapic_registers_virt);
    MADTEntryHeader *entry = (MADTEntryHeader*) (((uint64_t) madt) + sizeof(MADT));
    uint64_t incremented = sizeof(MADT);
    printf("Enumerating %i bytes of MADT entries...\n", madt->header.length);
    while (incremented < madt->header.length) {
        if (entry->entry_type == IOAPIC) {
            IOApic *this_ioapic = (IOApic*) entry;
            printf("I/O APIC device found. Information:\n");
            printf(" - I/O APIC ID: %i\n", this_ioapic->ioapic_id);
            printf(" - I/O APIC address: 0x%x\n", this_ioapic->ioapic_addr);
            printf(" - Global system interrupt base: %i\n", this_ioapic->global_system_interrupt_base);
            map_pages((uint64_t*) (kernel.cr3 + kernel.hhdm), (uint64_t) this_ioapic->ioapic_addr + kernel.hhdm, (uint64_t) this_ioapic->ioapic_addr, 1, KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_WRITE);
            kernel.ioapic_device = *this_ioapic;
            kernel.ioapic_addr   = this_ioapic->ioapic_addr;
        } else if (entry->entry_type == LOCAL_APIC) {
            ProcessorLocalAPIC *this_local_apic = (ProcessorLocalAPIC*) entry;
            printf("Processor local APIC device found. Information:\n");
            printf(" - Processor ID: %i\n", this_local_apic->processor_id);
            printf(" - APIC ID: %i\n", this_local_apic->apic_id);
        }
        entry = (MADTEntryHeader*) (((uint64_t) entry) + entry->record_length);
        incremented += entry->record_length;
        printf("Increment by %i, entry is %p\n", entry->record_length, entry);
    }
    printf("APIC set up successfully.\n");
}
