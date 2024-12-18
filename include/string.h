#pragma once
#include <stdint.h>
#include <stddef.h>

size_t strlen(const char *str);
void *memset (void *dest, int x, size_t n);
char *strcpy(char *dest, const char *src);
void *memcpy (void *dest, const void *src, size_t n);
void uint64_to_binary_string(uint64_t num, char *buf);
void uint64_to_hex_string(uint64_t num, char *str);
void uint64_to_string(uint64_t num, char* str);
void uint64_to_hex_string_padded(uint64_t num, char *str);
