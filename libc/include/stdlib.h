#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct HeapPool HeapPool;

struct HeapPool {
    uint8_t   verify;
    HeapPool *next;
    size_t    size;
    size_t    required_size;
    bool      free;
    uint8_t   data[0];
} __attribute__((packed));

void *malloc(size_t size);
void free(void *addr);
void exit(int status);
void *calloc(size_t nmemb, size_t sz);
void* realloc(void *addr, size_t sz);
double atof(const char *nptr);
int atoi(const char *nptr);
int system(const char *command);
char *getenv(char *key);
int setenv(char *key, char *val, int overwrite);
