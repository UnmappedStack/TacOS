#include <scheduler.h>
#include <string.h>
#include <panic.h>
#include <kernel.h>
#include <msr.h>
#include <kprintf.h>

/* A BRIEF OVERVIEW OF THE SCHEDULER */
/* ================================= */
/* Each processor has a ProcessorQueue.
 * It consists of 4 queues:
 *   - Realtime
 *   - Timeshare, interactive
 *   - Timesharing
 *   - Idle
 * Then realtime and timeshare threads are organised into
 * a calendar queue with:
 *   - realtime priorities ∈ [0, 64);
 *   - interactive timeshare priorities ∈ [64, 128);
 *   - timeshare priorities ∈ [128, 192)
 * where the exact priority within that range is based on the nice
 * values. The calendar queue is always checked for the highest priority
 * task on a task select, then idle threads are picked only when there are NO
 * threads on the other queues.
 *
 * Whether a timeshare thread gets to be in the interactive queue or not is based on the following:
 *   scaling_score = threshold/2
 *   if (sleeptime > runtime) score = scaling_score / (sleeptime/runtime)
 *   else score = scaling_score / (runtime/sleeptime) scaling_score
 * Where if score > threshold then the thread is considered interactive. When sleeptime + runtime hit 100, they
 * are both halved.
 *
 * A typical ULE scheduler would also have kernel threads, but at the moment these do not exist.
 * Processor affinity is also supported, where a thread can simply be flagged as affinitive so it will have
 * immunity from the load balancer.
 *
 * Load balancing occurs in two ways:
 *    - Pull migration: when a processor is out of threads, it just takes the highest priority thread
 *      from the processor queue with the highest load (calculated as a number of ticks in a sliding window)
 *    - Push migration: twice a second, the highest and lowest loaded queues are selected, and some threads
 *      are transferred between them.
 */
/* This scheduler is highly inspired from mainly the original ULE scheduler, but with the calendar queue
 * design from modern ULE. */

// GSBase is used to store a pointer to the ProcessorQueue for the current processor
#define GSBASE 0xC0000101

ProcessorQueue *current_processor_queue(void) {
    ProcessorQueue *ret = (ProcessorQueue*) rdmsr(GSBASE);
    if (ret == NULL) {
        kpanic("GSBase not set yet");
    }
    return ret;
}

// pick a bucket of the calendar queue to insert it into, based on priority and class
void calendar_queue_reinsert_thread(ProcessorQueue *queue, Thread *thread) {
    const size_t max_priority = 224;
    size_t bucket_idx = (queue->current_bucket + 1 + max_priority - thread->priority) % NUM_BUCKETS;
    list_insert(&queue->calendar_queue[bucket_idx].threads, &thread->bucket_list);
}

int calculate_thread_priority(ProcessorQueue *queue, Thread *thread) {
    const int range_map[][3] = {
        [SCHED_REALTIME] = {0, 63},
        [SCHED_INTERACTIVE_TIMESHARE] = {64, 127},
        [SCHED_TIMESHARE] = {128, 192},
    };
    int nice_diff = thread->nice - queue->least_nice_thread;
    int min = range_map[thread->s_class][0];
    int max = range_map[thread->s_class][1];
    int ret = nice_diff + min;
    if (ret > max)
        ret = max;
    return ret;
}

/* we just add it to the current queue instead of looking for the least
 * loaded queue or whatever. TODO: add it to the least loaded queue instead.
 *
 * !! This requires the thread to already be set up with nice etc, but it calculates initial priority itself !! */
Thread *add_thread_to_current_processor(Thread *thread) {
    ProcessorQueue *queue = current_processor_queue();
    
    if (thread->nice < queue->least_nice_thread || queue->least_nice_thread < 0)
        queue->least_nice_thread = thread->nice;
    if (thread->nice > queue->nicest_thread)
        queue->nicest_thread = thread->nice;

    thread->priority = calculate_thread_priority(queue, thread);

    switch (thread->s_class) {
        case SCHED_REALTIME:
            list_insert(&queue->realtime_threads, &thread->class_list);
            calendar_queue_reinsert_thread(queue, thread);
            break;
        case SCHED_INTERACTIVE_TIMESHARE:
            list_insert(&queue->interactive_timeshare_threads, &thread->class_list);
            calendar_queue_reinsert_thread(queue, thread);
            break;
        case SCHED_TIMESHARE:
            list_insert(&queue->timeshare_threads, &thread->class_list);
            calendar_queue_reinsert_thread(queue, thread);
            break;
        case SCHED_IDLE:
            list_insert(&queue->timeshare_threads, &thread->class_list);
            break;
        default: kpanic("unreachable (sched)");
    }
    return thread;
}

Thread *create_thread(SchedClass sched_class, int nice, uint8_t flags) {
    Thread *thread = slab_alloc(kernel_info.schedulers.thread_cache);

    thread->flags = flags;
    thread->nice = nice;
    thread->s_class = sched_class;

    return thread;
}

/* Initialises the scheduler on the current processor */
void processor_scheduler_init(void) {
    ProcessorQueue *new_queue = slab_alloc(kernel_info.schedulers.processor_queue_cache);
    memset(new_queue->calendar_queue, 0, sizeof(CalendarBucket) * NUM_BUCKETS);
    new_queue->current_bucket = 0;
    new_queue->least_nice_thread = -1;
    new_queue->nicest_thread = -1;

    list_init(&new_queue->realtime_threads);
    list_init(&new_queue->interactive_timeshare_threads);
    list_init(&new_queue->timeshare_threads);
    list_init(&new_queue->idle_threads);
    for (int i = 0; i < NUM_BUCKETS; i++) {
        list_init(&new_queue->calendar_queue[i].threads);
    }

    list_insert(&kernel_info.schedulers.processor_queues, &new_queue->list);
    wrmsr(GSBASE, (uint64_t)new_queue);

    kprintf("thread priority: %u\n", add_thread_to_current_processor(create_thread(SCHED_TIMESHARE,  10, 0))->priority);
    kprintf("thread priority: %u\n", add_thread_to_current_processor(create_thread(SCHED_TIMESHARE, 100, 0))->priority);
    kprintf("thread priority: %u\n", add_thread_to_current_processor(create_thread(SCHED_TIMESHARE,  50, 0))->priority);
    kprintf("thread priority: %u\n", add_thread_to_current_processor(create_thread(SCHED_TIMESHARE,  50, 0))->priority);

    kprintf("Processor scheduler init OK\n");
}

/* Initialises the scaffolding for scheduling needed for all processors */
void global_scheduler_init(void) {
    kernel_info.schedulers.processor_queue_cache = cache_create(sizeof(ProcessorQueue));
    kernel_info.schedulers.thread_cache = cache_create(sizeof(Thread));

    list_init(&kernel_info.schedulers.processor_queues);

    kprintf("Global scheduler init OK\n");
}
