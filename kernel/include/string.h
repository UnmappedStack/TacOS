#pragma once
#include <stddef.h>
#include <stdint.h>

void uint64_to_hex_string(uint64_t num, char *str);
void uint64_to_string(uint64_t num, char *str);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *dest, int ch, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
uint64_t str_to_u64(const char *str);
uint64_t strlen(const char *str);
int strcmp(const char *s1, const char *s2);
