#pragma once
#include <stdint.h>
#include <stddef.h>

int memcmp(char *str1, char *str2, size_t bytes);
void *memmove(void *dest, const void *src, size_t n);
size_t oct2bin(char *str, int size);
size_t strlen(const char *str);
void *memset (void *dest, int x, size_t n);
int strcmp(char *str1, char *str2);
char *strcpy(char *dest, const char *src);
void *memcpy (void *dest, const void *src, size_t n);
void uint64_to_binary_string(uint64_t num, char *buf);
void uint64_to_hex_string(uint64_t num, char *str);
void uint64_to_string(uint64_t num, char* str);
void uint64_to_hex_string_padded(uint64_t num, char *str);
int strcontains(char *s, char c);
uint64_t str_to_u64(const char *str);
