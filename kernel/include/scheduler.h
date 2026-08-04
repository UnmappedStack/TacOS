#pragma once
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

typedef struct {
    struct list class_list;  /* linked list of other threads of this class
                              * in the ProcessorQueue it belongs to */
    struct list bucket_list; /* linked list of other threads in this calendar
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
    struct list threads;
} CalendarBucket;

typedef struct {
    struct list list; // the other processor queues as a linked list

    struct list realtime_threads;
    struct list interactive_timeshare_threads;
    struct list timeshare_threads;
    struct list idle_threads;

    CalendarBucket calendar_queue[NUM_BUCKETS];
    uint64_t current_bucket;
    uint64_t bucket_bitmap;

    int least_nice_thread;
    int nicest_thread;
} ProcessorQueue;
static_assert(NUM_BUCKETS <= 64, "bitmap too small for number of threads");

typedef struct {
    Cache *processor_queue_cache;
    Cache *thread_cache;
    struct list processor_queues;
    int tid_upto;
} GlobalSchedulerInfo;

void processor_scheduler_init(void);
void global_scheduler_init(void);
Thread *thread_select(void);
