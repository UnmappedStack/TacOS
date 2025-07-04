#pragma once
#include <stddef.h>

char* strndup(char *s, size_t n);
size_t strlen(const char *str);
int strcmp(const char *s1, const char *s2);
void *memset(void *dest, int x, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
char* strrchr(char *s, int c);
char* strchr(char *s, int c);
char* strstr(const char *str, const char *needle);
char *strcpy(char *restrict dst, const char *restrict src);
int strncmp(const char *s1, const char *s2, size_t n);
char *strncpy(char *restrict dst, const char *restrict src, size_t n);
char* strdup(const char *s);
char* memchr(const char *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
char *strerror(int errnum);
char *strpbrk(const char *s, const char *accept);
size_t strspn(const char *s, const char *accept);
int strcoll(const char *s1, const char *s2);
