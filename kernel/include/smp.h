#pragma once
#include <scheduler.h>

// GSBase is used to store a pointer to the CPU struct for the current processor
// (TODO: move to x86-64 specific stuff, this is not isa agnostic)
#define GSBASE 0xC0000101

typedef enum {
    IPI_NONE, // ignore, like a nop
    IPI_HALT,
} IPIType;

typedef struct {
    IPIType type;
    uint64_t data;
} IPIMessage;

#define MAX_IPI_MESSAGES 32
typedef struct {
    MCSSpinlock lock;

    IPIMessage messages[MAX_IPI_MESSAGES];

    /* new messages will be inserted at this index then the index will be
     * incremented, kind of like a ringbuffer but there isn't a separate
     * reader/writer. it'll loop back once its filled, but that hopefully
     * shouldn't happen often at all. it'll always read from idx 0. */
    size_t upto;
} IPIQueue;

/* All information and status stuff related to one CPU. */
typedef struct {
    uint64_t id;
    ProcessorQueue *scheduler;
    IPIQueue ipi_queue;
} CPU;

void smp_init(void);
CPU *current_processor(void);
void ipi_send(IPIMessage message);
void ipi_handler(void);
void halt_all_processors(void);
