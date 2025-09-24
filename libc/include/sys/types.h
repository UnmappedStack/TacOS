#pragma once
#include <stddef.h>
#include <mman.h>
#include <stdbool.h>
#include <stdio.h>

typedef int ino_t;

// structure source: https://stackoverflow.com/a/12991451
struct dirent {
    ino_t          d_ino;       /* inode number */
    off_t          d_off;       /* offset to the next dirent */
    unsigned short d_reclen;    /* length of this record */
    unsigned char  d_type;      /* type of file; not supported
                                   by all file system types */
    char           d_name[256]; /* filename */
};
