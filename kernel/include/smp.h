#pragma once
#include <scheduler.h>

// GSBase is used to store a pointer to the CPU struct for the current processor
// (TODO: move to x86-64 specific stuff, this is not isa agnostic)
#define GSBASE 0xC0000101

#define CPU_ALL (-1)

typedef enum {
    IPI_NONE, // ignore, like a nop
    IPI_HALT,
} IPIType;

typedef struct {
    /* an IPI sender doesn't need to set these two,
     * they're just used internally by the ipi system */
    LList list;
    size_t *countdown;

    IPIType type;
    uint64_t data;
} IPIMessage;

typedef struct {
    MCSSpinlock lock;
    LList(IPIMessage) messages;
} IPIQueue;

/* All information and status stuff related to one CPU. */
typedef struct {
    uint64_t id;
    ProcessorQueue *scheduler;
    IPIQueue ipi_queue;
} CPU;

void smp_init(void);
CPU *current_processor(void);
void ipi_send(int cpu, IPIMessage message, bool sync);
void ipi_handler(void);
void halt_all_processors(void);
