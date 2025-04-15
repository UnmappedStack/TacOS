#pragma once
#include <scheduler.h>

typedef struct {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) CallFrame;

pid_t fork(CallFrame *callframe);
