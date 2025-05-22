#pragma once
#include <stdint.h>

typedef struct {
    uint64_t cr2;
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t type;
    uint64_t code;
    uint64_t rip;
    uint64_t cs;
    uint64_t flags;
    uint64_t rsp;
    uint64_t ss;
} IDTEFrame;

extern void divideException(void);
extern void debugException(void);
extern void breakpointException(void);
extern void overflowException(void);
extern void boundRangeExceededException(void);
extern void invalidOpcodeException(void);
extern void deviceNotAvaliableException(void);
extern void doubleFaultException(void);
extern void coprocessorSegmentOverrunException(void);
extern void invalidTSSException(void);
extern void segmentNotPresentException(void);
extern void stackSegmentFaultException(void);
extern void generalProtectionFaultException(void);
extern void pageFaultException(void);
extern void floatingPointException(void);
extern void alignmentCheckException(void);
extern void machineCheckException(void);
extern void simdFloatingPointException(void);
extern void virtualisationException(void);

void init_exceptions(void);
