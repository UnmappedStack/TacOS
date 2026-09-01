#include <isa/cpu.h>
#include <stddef.h>
#include <kernel.h>
#include <stdint.h>
#include <kprintf.h>

// non-exception interrupts
#define INTERRUPT_IPI    1
#define INTERRUPT_EBREAK 3
#define INTERRUPT_TIMER  5

void handle_timer_interrupt(void) {
    DISABLE_INTERRUPTS();
    CPU *cpu = current_processor();
    if (kernel_info.schedulers.least_loaded_processor == NULL ||
            cpu->scheduler->num_threads < kernel_info.schedulers.least_loaded_processor->num_threads)
        kernel_info.schedulers.least_loaded_processor = cpu->scheduler;
    Thread *thread;

    // we only want to load balance on one processor, otherwise it'll be too often
    if (!cpu->id) cpu->scheduler->total_ticks++;
    if (!cpu->id && cpu->scheduler->total_ticks % 50 == 0)
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
        kprintf("%s%u\e[0m,", colours[cpu->id % 5], thread->tid);
}

void handle_exception(InterruptStackFrame *frame) {
    switch (frame->cause) {
    case INTERRUPT_EBREAK:
        kprintf("\n   > EBREAK -> scause=%x\n", frame->cause);
        /* so basically we have to check if the instruction at the return
         * address is aligned. if it is then just increment it by 16 bytes,
         * otherwise by a full 32 bytes. then we can just return to the
         * assembly caller. It is compressed if the low 2 bits are not set. */
        bool is_compressed = (*((uint16_t*)frame->return_addr) & 0b11) != 0b11;
        uint64_t instruction_size = (is_compressed) ? 2 : 4;
        frame->return_addr += instruction_size;
        kprintf("   > %s, instruction increment %u\n\n",
                (is_compressed) ? "compressed" : "non-compressed", instruction_size);
        break;
    case EXCEPTION_ADDRESS_MISALIGNED:
    case EXCEPTION_INSTRUCTION_ACCESS_FAULT:
    case EXCEPTION_ILLEGAL_INSTRUCTION:
    case EXCEPTION_LOAD_ADDRESS_MISALIGNED:
    case EXCEPTION_LOAD_ADDRESS_FAULT:
    case EXCEPTION_STORE_AMO_ACCESS_FAULT:
    case EXCEPTION_INSTRUCTION_PAGE_FAULT:
    case EXCEPTION_LOAD_PAGE_FAULT:
    case EXCEPTION_STORE_AMO_PAGE_FAULT:
    case EXCEPTION_SOFTWARE_CHECK:
    case EXCEPTION_HARDWARE_ERROR:
        panic_handler(NULL, frame);
        break;
    default:
        kprintf("   > unhandled interrupt %u, freeze this cpu (TODO handle this properly)\n", frame->cause);
        FREEZE_DEVICE();
    }
}

void interrupt_handler(InterruptStackFrame *frame) {
    bool exception = !((frame->cause >> 63) & 1);
    
    if (exception) return handle_exception(frame);

    uint64_t cause = frame->cause & ~(1ULL << 63);
    switch (cause) {
    case INTERRUPT_TIMER:
        handle_timer_interrupt();
        timer_set_timeout(PREEMPTION_INTERVAL_MS);
        ENABLE_INTERRUPTS();
        break;
    case INTERRUPT_IPI:
        // this will later have a proper ipi system, but for now we just halt
        // and assume its because of a panic
        kprintf("Halt CPU%u\n", get_current_cpu_info()->id);
        FREEZE_DEVICE();
        break;
    default:
        kprintf("Unexpected interrupt %u\n", cause);
        break;
    }
}

extern void prepare_for_interrupt(void); // asm handler, will call interrupt_handler() as defined above
void interrupts_init(void) {
    csr_write(CSR_REG_STVEC, (uintptr_t)&prepare_for_interrupt);
}
