#include <isa/cpu.h>
#include <kprintf.h>
#include <kernel.h>

uint64_t read_time(void) {
    uint64_t ret;
    __asm__ volatile("rdtime %0\n" : "=r"(ret));
    return ret;
}

#define MS_PER_SEC 1000
uint64_t ms_to_ticks(uint64_t ms, uint64_t freq) {
    return freq * ms / MS_PER_SEC;
}

void timer_set_timeout(uint64_t ms) {
    uint64_t curr_time = read_time();
    uint64_t wait_until = curr_time + ms_to_ticks(ms, kernel_info.timebase_freq);
    csr_write(CSR_REG_STIMECMP, wait_until);
}

void timer_global_init(void) {
    if (!kernel_info.timebase_freq) kpanic("no timebase freq dt entry found");
    klogf(LOG_STATUS, "Timer global init OK\n");
}

void timer_local_init(void) {
    timer_set_timeout(PREEMPTION_INTERVAL_MS);
    csr_write(CSR_REG_SIE, STIE | SSIP);
}
