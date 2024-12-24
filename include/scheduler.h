#pragma once
#include <mem/paging.h>
#include <mem/slab.h>
#include <stdint.h>
#include <list.h>

#define MAX_RESOURCES   20 // TODO: Dynamically allocate this.
#define TASK_PRESENT    0b001
#define TASK_RUNNING    0b010
#define TASK_FIRST_EXEC 0b100

typedef uint64_t task_id_t;
typedef uint8_t task_flags_t;
typedef struct Task Task;

struct Task {
    struct list   list;
    task_id_t     tid;
    uint64_t      pml4;
    void         *entry;
    Task         *parent;
    task_flags_t  flags;
};

typedef struct {
    struct list *list;
    Task        *current_task;
    Cache       *cache;
    task_id_t    tid_upto;
} SchedulerQueue;

void init_scheduler();
Task *task_add();
Task *task_select();
