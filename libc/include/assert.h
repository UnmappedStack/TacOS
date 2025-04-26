#pragma once

#define assert(x) \
    if (!x) { \
        fprintf(stderr, "assert failed in " __FILE__ " on line %d\n", __LINE__); \
        exit(1); \
    }
