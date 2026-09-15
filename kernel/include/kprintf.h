#pragma once
#include <lock.h>

#define LOG_ERROR  1
#define LOG_STATUS 2
#define LOG_WARN   3
#define LOG_DEBUG  4

// TODO: this should maybe be decided from bootloader cmdline/config
#define VERBOSITY_MAX_LEVEL LOG_DEBUG

extern const char *log_level_strings[];
void kprintf(const char *fmt, ...);
void print_string(const char *s);

// stupid c requires me to have 2 macros otherwise x might not get expanded
#define _STR(x) #x
#define STR(x) _STR(x)

extern MCSSpinlock log_lock;
// TODO: timestamps
#define klogf(level, ...) do { \
    if (level <= VERBOSITY_MAX_LEVEL) { \
        MCSSpinlock local_log_lock; \
        mcs_spinlock_acquire(&log_lock, &local_log_lock); \
        print_string(log_level_strings[level]); \
        print_string(" "); \
        kprintf(__VA_ARGS__); \
        mcs_spinlock_release(&log_lock, &local_log_lock); \
    } \
} while (0)

#define HERE(x) klogf(LOG_DEBUG, "HERE: %u\n", x)
