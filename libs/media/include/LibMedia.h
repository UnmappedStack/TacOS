#pragma once
#include <stdint.h>
#include <stddef.h>

uint32_t *decode_qoi(const char *path, size_t *width, size_t *height);
