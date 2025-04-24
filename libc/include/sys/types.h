#pragma once
#include <stddef.h>
#include <stdbool.h>

struct dirent {
    char d_name[30];
    bool d_isdir;
    size_t d_fsize;
};
