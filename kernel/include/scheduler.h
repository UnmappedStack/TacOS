#pragma once
#include <lock.h>
#include <slab.h>
#include <list.h>
#include <stdint.h>
#include <stddef.h>

typedef enum {
    SCHED_REALTIME,
    SCHED_INTERACTIVE_TIMESHARE,
    SCHED_TIMESHARE,
    SCHED_IDLE,
} SchedClass;

/* for the flags field of Thread */
#define THREAD_FLAG_AFFINITIVE  0b01
#define THREAD_FLAG_INTERACTIVE 0b10

#define NUM_BUCKETS 64

#define PREEMPTION_INTERVAL_MS 10

typedef struct {
    LList class_list;  /* linked list of other threads of this class
                              * in the ProcessorQueue it belongs to */
    LList bucket_list; /* linked list of other threads in this calendar
                                queue bucket */

    /* misc info */
    size_t tid;
    uint8_t flags;

    /* priority related stuff */
    int nice;
    SchedClass s_class;
    int priority; // based on nice values and class
} Thread;

typedef struct {
    LList threads;
} CalendarBucket;

typedef struct {
    LList list; // the other processor queues as a linked list

    LList realtime_threads;
    LList interactive_timeshare_threads;
    LList timeshare_threads;
    LList idle_threads;

    CalendarBucket calendar_queue[NUM_BUCKETS];
    uint64_t current_bucket;
    uint64_t bucket_bitmap;

    int least_nice_thread;
    int nicest_thread;
    int num_threads;

    uint64_t total_ticks;
    Spinlock lock; // should only be locked for load balancing, *not* thread selection
} ProcessorQueue;
static_assert(NUM_BUCKETS <= 64, "bitmap too small for number of threads");

typedef struct {
    Cache *processor_queue_cache;
    Cache *thread_cache;
    LList processor_queues;
    int tid_upto;
    bool ready;
    ProcessorQueue *least_loaded_processor;
    ProcessorQueue  *most_loaded_processor; /* TODO: This is currently based on which
                                             * has the most threads. It should change to whichever has
                                             * the most ticks in a sliding window at some point (TM). */
} GlobalSchedulerInfo;

void processor_scheduler_init(void);
void global_scheduler_init(void);
Thread *thread_select(void);
void smp_init(void);
Thread *add_thread_to_current_processor(Thread *thread);
Thread *create_thread(SchedClass sched_class, int nice, uint8_t flags);
Thread *migrate_push(void); // only push must be exposed in the header, as pull is called
                            // within the scheduler only
