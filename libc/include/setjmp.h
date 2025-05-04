#pragma once
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uintptr_t rip;
    uintptr_t rsp;
    uintptr_t rax;
    uintptr_t rbx;
    uintptr_t rcx;
    uintptr_t rdx;
    uintptr_t rdi;
    uintptr_t rsi;
    uintptr_t r8;
    uintptr_t r9;
    uintptr_t r10;
    uintptr_t r11;
    uintptr_t r12;
    uintptr_t r13;
    uintptr_t r14;
    uintptr_t r15;
    uintptr_t rbp;
} jmp_buf;

int  __setjmp_internal(jmp_buf *buf);
__attribute__((noreturn)) void __longjmp_internal(jmp_buf *env, int val);

#define setjmp(buf) (__setjmp_internal(&buf))
#define longjmp(buf, val) (__longjmp_internal(&buf, val))
