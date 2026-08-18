#pragma once

#define EXCEPTION_ADDRESS_MISALIGNED       0
#define EXCEPTION_INSTRUCTION_ACCESS_FAULT 1
#define EXCEPTION_ILLEGAL_INSTRUCTION      2
#define EXCEPTION_LOAD_ADDRESS_MISALIGNED  4
#define EXCEPTION_STORE_AMO_ACCESS_FAULT   7
#define EXCEPTION_INSTRUCTION_PAGE_FAULT   12
#define EXCEPTION_LOAD_PAGE_FAULT          13
#define EXCEPTION_STORE_AMO_PAGE_FAULT     15
#define EXCEPTION_SOFTWARE_CHECK           18
#define EXCEPTION_HARDWARE_ERROR           19

void kpanic(const char *s);
void panic_handler(const char *msg, InterruptStackFrame *frame);
