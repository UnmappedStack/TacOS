#include <scheduler.h>
#include <smp.h>
#include <limine.h>
#include <util.h>
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

/* (temporary comment) TODO for a complete scheduler:
 *  [X] Thread creation;
 *  [X] Thread selection;
 *  [ ] SMP support (hopefully easy from how previous steps were designed);
 *  [ ] Load balancing;
 *  [ ] Interactiveness determination;
 */

ProcessorQueue *current_processor_queue(void) {
    ProcessorQueue *ret = current_processor()->scheduler;
    return ret;
}

static const int range_map[][3] = {
    [SCHED_REALTIME] = {0, 63},
    [SCHED_INTERACTIVE_TIMESHARE] = {64, 127},
    [SCHED_TIMESHARE] = {128, 192},
};

// pick a bucket of the calendar queue to insert it into, based on priority and class
void calendar_queue_reinsert_thread(ProcessorQueue *queue, Thread *thread) {
    size_t bucket_idx = (queue->current_bucket + 1 + thread->priority) % NUM_BUCKETS;
    list_insert(&queue->calendar_queue[bucket_idx].threads, &thread->bucket_list);
    queue->bucket_bitmap |= 1ULL << bucket_idx;
}

int calculate_thread_priority(ProcessorQueue *queue, Thread *thread) {
    int nice_diff = thread->nice - queue->least_nice_thread;
    int min = range_map[thread->s_class][0];
    int max = range_map[thread->s_class][1];
    int ret = nice_diff + min;
    if (ret > max)
        ret = max;
    return ret;
}

// recalculates the priorities of all threads on a scheduler's queue
// (only timeshare threads)
void recalculate_queue_priorities(ProcessorQueue *queue) {
    for (struct list *list = queue->timeshare_threads.next;
             list != &queue->timeshare_threads; list = list->next) {
        Thread *thread = CONTAINER_OF(list, Thread, class_list);
        thread->priority = calculate_thread_priority(queue, thread);
    }
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

    if (thread->nice < queue->least_nice_thread)
        queue->least_nice_thread = thread->nice;

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
    
    recalculate_queue_priorities(queue);

    return thread;
}

Thread *create_thread(SchedClass sched_class, int nice, uint8_t flags) {
    Thread *thread = slab_alloc(kernel_info.schedulers.thread_cache);

    thread->flags = flags;
    thread->nice = nice;
    thread->s_class = sched_class;
    thread->tid = kernel_info.schedulers.tid_upto++;

    return thread;
}

/* Selects a thread to run for the current processor by:
 *  (1) check calendar queue to see if there's something to run:
 *          - if there is, reinsert it later in the queue and run it.
 *  (2) if there's nothing in the calendar queue, look for something in
 *      the idle queue to run
 *  (3) if there's nothing to run, do a PULL load balance operation (TODO,
 *      right now we just complain that the cpu is being used inefficiently)
 */
Thread *thread_select(void) {
    ProcessorQueue *current_queue = current_processor_queue();

    // find the first bucket which is not empty (or at least try)
    if (current_queue->bucket_bitmap) {
        /* we know there's *some* bucket avaliable.
         * note that we only want to get set bits after the current bucket, OR
         * in the next "year" of the calendar */

        // first check remaining ones for this year
        int next_available;
        if (current_queue->bucket_bitmap >> current_queue->current_bucket) {
            // see https://gcc.gnu.org/onlinedocs/gcc/Bit-Operation-Builtins.html for __builtin_clzll()
            // (it basically just does the bsf instruction)
            next_available = __builtin_ctzll(current_queue->bucket_bitmap >> current_queue->current_bucket)
                                        + current_queue->current_bucket;
        } else {
            // we need to check for the next 'year'
            next_available = __builtin_ctzll(current_queue->bucket_bitmap);
        }

        CalendarBucket *bucket = &current_queue->calendar_queue[next_available];
        struct list *thread_list = bucket->threads.next;

        list_remove(thread_list);
        if (list_empty(&bucket->threads)) {
            current_queue->bucket_bitmap &= ~(1ULL << next_available);
            current_queue->current_bucket++;
        }
        
        if (current_queue->current_bucket > NUM_BUCKETS-1)
            current_queue->current_bucket = 0;
        Thread *thread = CONTAINER_OF(thread_list, Thread, bucket_list);
        calendar_queue_reinsert_thread(current_queue, thread);
        return thread;
    }

    // nothing in calendar queue, try get something from the idle queue
    if (!list_empty(&current_queue->idle_threads)) {
        struct list *thread_list = current_queue->idle_threads.next;
        list_remove(thread_list);
        list_insert(&current_queue->idle_threads, thread_list);
        Thread *thread = CONTAINER_OF(thread_list, Thread, class_list);
        return thread;
    }

    /* this should do a PULL load balance operation but for now we just complain
     * because having a cpu without threads is a waste of a cpu and is inefficient:
     * resources are there to be used! */
    return NULL;
}

/* Initialises the scheduler on the current processor */
void processor_scheduler_init(void) {
    CPU *processor = current_processor();
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
    new_queue->bucket_bitmap = 0;

    list_insert(&kernel_info.schedulers.processor_queues, &new_queue->list);

    processor->scheduler = new_queue;

    add_thread_to_current_processor(create_thread(SCHED_TIMESHARE, 10, 0));
    add_thread_to_current_processor(create_thread(SCHED_TIMESHARE, 15, 0));
    add_thread_to_current_processor(create_thread(SCHED_TIMESHARE, 20, 0));

    kprintf("Processor scheduler init OK\n");
}

/* Initialises the scaffolding for scheduling needed for all processors */
void global_scheduler_init(void) {
    kernel_info.schedulers.processor_queue_cache = cache_create(sizeof(ProcessorQueue));
    kernel_info.schedulers.thread_cache = cache_create(sizeof(Thread));
    kernel_info.schedulers.tid_upto = 0;

    list_init(&kernel_info.schedulers.processor_queues);
    kernel_info.schedulers.ready = true;
    kprintf("Global scheduler init OK\n");
}
