#pragma once

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

// TODO: timestamps
#define klogf(level, ...) do { \
    if (level <= VERBOSITY_MAX_LEVEL) { \
        print_string(log_level_strings[level]); \
        print_string(" " __FILE__ ":" STR(__LINE__) ": "); \
        kprintf(__VA_ARGS__); \
    } \
} while (0)
