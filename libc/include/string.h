#pragma once
#include <stddef.h>

size_t strlen(const char *str);
int strcmp(const char *s1, const char *s2);
void *memset(void *dest, int x, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
