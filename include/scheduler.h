#pragma once
#include <fs/vfs.h>
#include <string.h>
#include <mem/memregion.h>
#include <mem/paging.h>
#include <mem/slab.h>
#include <stdint.h>
#include <list.h>

#define MAX_RESOURCES   20 // TODO: Dynamically allocate this.
#define MAX_CHILDREN    20
#define TASK_PRESENT    0b0001
#define TASK_RUNNING    0b0010
#define TASK_FIRST_EXEC 0b0100
#define TASK_DEAD       0b1000

typedef uint64_t pid_t;
typedef uint8_t task_flags_t;
typedef struct Task Task;

typedef struct {
    pid_t pid;
    int status;
} Child;

typedef struct {
    VfsFile *f;
    size_t offset;
} Resource;

// WARNING: Be super careful when changing this struct and update any changes accordingly in src/tasks/switch.asm
// because otherwise things can break in the context switch.
struct Task {
    struct list   list;
    pid_t         pid;
    uint64_t      pml4;
    uint64_t      rsp;
    void         *entry;
    Task         *parent;
    Memregion    *memregion_list;
    task_flags_t  flags;
    Resource      resources[MAX_RESOURCES];
    Child         children[MAX_CHILDREN];
    uintptr_t     program_break; // used for sbrk
};

typedef struct {
    struct list *list;
    Task        *current_task;
    Cache       *cache;
    pid_t        pid_upto;
} SchedulerQueue;

void init_scheduler(void);
Task *task_add(void);
Task *task_select(void);


