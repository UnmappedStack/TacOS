#include <isa/cpu.h>
#include <stddef.h>
#include <lock.h>
#include <assets.h>
#include <kprintf.h>

static const char *exceptions[] = {
    "Instruction address misaligned",
    "Instrction access fault",
    "Illegal instruction",
    "Breakpoint",
    "Load address misaligned",
    "Load access fault",
    "Store/AMO address misaligned",
    "Store/AMO access fault",
    "Env. call from U-mode",
    "Env. call from S-mode",
    "rsvd", "rsvd",
    "Instruction page fault",
    "Load page fault",
    "rsvd",
    "Store/AMO page fault",
    "rsvd", "rsvd",
    "Software check",
    "Hardware check",
};

#define SPP_MASK (1 << 8)

Spinlock panic_lock = {0};
void panic_handler(const char *msg, InterruptStackFrame *frame) {
    DISABLE_INTERRUPTS();
    kprintf("\n === KERNEL PANIC ENTERED === \n\n");
    spinlock_acquire(&panic_lock); // never released

    int i = 0;
#define ASCII_ART_LINE() kprintf(ascii_art[i++]);
#define ASCII_ART_NEWLINE() \
    do { \
        ASCII_ART_LINE(); \
        kprintf("\n"); \
    } while (0)
    const char *error_type;
    if (msg != NULL)
        error_type = msg;
    else if (frame->cause <= 18)
        error_type = exceptions[frame->cause];
    else error_type = "Unknown exception";

    for (int n = 0; n < 7; n++) ASCII_ART_NEWLINE();
    ASCII_ART_LINE(); kprintf(" WOAH! You messed this all up!\n");
    ASCII_ART_LINE(); kprintf(" This is ALL your fault. I take ZERO responsibility!\n");
    ASCII_ART_NEWLINE();
    char privilege_level = (frame->sstatus & SPP_MASK) ? 'S' : 'U';
    ASCII_ART_LINE(); kprintf(" Exception type: %s in %c-mode\n", error_type, privilege_level);
    ASCII_ART_LINE(); kprintf(" Cause %u, value %u\n", frame->cause, frame->val);
    ASCII_ART_NEWLINE();
    if (msg != NULL) {
        ASCII_ART_LINE(); kprintf(" (Manually induced panic so registers may be null)\n");
    }
    while(i < (int)(sizeof(ascii_art)/sizeof(ascii_art[0]))) ASCII_ART_NEWLINE();
    FREEZE_DEVICE();
}

void kpanic(const char *s) {
    kprintf("STUB: kernel panic occurred: %s\n", s);
    FREEZE_DEVICE();
}
