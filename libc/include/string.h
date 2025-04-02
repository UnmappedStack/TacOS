#pragma once
#include <stddef.h>

size_t strlen(const char *str);
int strcmp(const char *s1, const char *s2);
void *memset(void *dest, int x, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int toupper(char ch);
int tolower(char ch);
char* strrchr(char *s, int c);
char* strchr(char *s, int c);
char* strstr(char *str, const char *needle);
char *strcpy(char *restrict dst, const char *restrict src);
int strncmp(const char *s1, const char *s2, size_t n);
char *strncpy(char *restrict dst, const char *restrict src, size_t n);
char* strdup(const char *s);
