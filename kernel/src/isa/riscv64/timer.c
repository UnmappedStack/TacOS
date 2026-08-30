#include <isa/cpu.h>
#include <kprintf.h>

uint64_t read_time(void) {
    uint64_t ret;
    __asm__ volatile("rdtime %0\n" : "=r"(ret));
    return ret;
}

void timer_init(void) {
    uint64_t curr_time = read_time();
    kprintf("curr_time=%x\n", curr_time);
    kprintf("Timer global init OK (not really lol TODO)\n");
    for (;;);
}

void timer_local_init(void) {
    kprintf("Timer local init OK\n");
}
