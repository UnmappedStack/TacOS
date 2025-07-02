#pragma once
#include <fs/vfs.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    int (*read)(void *file, char *buffer, size_t max_len, size_t offset);
    int (*write)(void *file, char *buffer, size_t len, size_t offset);
    int (*open)(void **buf, void *file);
    int (*close)(void *file);
} DeviceOps;

void init_devices(void);
VfsFile *mkdevice(char *path, DeviceOps ops);
