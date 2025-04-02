#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <syscall.h>

__attribute__((noreturn))
void exit(int status) {
    __syscall1(4, status);
    for (;;);
}

// TODO: Move heap to another file

#define PAGE_ALIGN_DOWN(addr) ((addr / 4096) * 4096)
#define PAGE_ALIGN_UP(x) ((((x) + 4095) / 4096) * 4096)

extern HeapPool *start_heap;

HeapPool create_pool(uint64_t size, uint64_t required_size, HeapPool *next, bool free) {
    HeapPool pool;
    pool.verify        = 69;
    pool.size          = size;
    pool.required_size = required_size;
    pool.next          = next;
    pool.free          = free;
    return pool;
}

void* split_pool(HeapPool *pool_addr, uint64_t size) {
    HeapPool *new_pool = (HeapPool*) (((uint64_t) pool_addr) + pool_addr->required_size + 1);
    uint64_t new_pool_size = sizeof(HeapPool) + size + 1;
    *new_pool = create_pool(pool_addr->size - pool_addr->required_size, new_pool_size, pool_addr->next, false);
    pool_addr->size = pool_addr->required_size;
    pool_addr->next = new_pool;
    return (void*)new_pool;
}

void* heap_grow(size_t size, HeapPool *this_pool) {
    uint64_t new_pool_size = PAGE_ALIGN_UP(size + sizeof(HeapPool));
    this_pool->next = sbrk(new_pool_size);
    *((HeapPool*) this_pool->next) = create_pool(new_pool_size, size + sizeof(HeapPool), 0, false);
    return (void*) ((HeapPool*) this_pool->next)->data;
}

void* malloc(uint64_t size) {
    HeapPool *this_pool = start_heap;
    for (;;) {
        if (this_pool->free && this_pool->size > size + sizeof(HeapPool)) {
            this_pool->free = false;
            this_pool->required_size = size + sizeof(HeapPool);
            return (void*) this_pool->data;
        } else if (this_pool->size > this_pool->required_size + size + sizeof(HeapPool)) {
            HeapPool *new_pool = (HeapPool*) split_pool(this_pool, size);
            return (void*) new_pool->data;
        } else if (this_pool->next == 0) {
            return heap_grow(size, this_pool);
        }
        this_pool = (HeapPool*) this_pool->next;
    }
}

void free(void *addr) {
    HeapPool *this_pool      = (HeapPool*) (((uint64_t)addr) - sizeof(HeapPool));
    if (this_pool->verify != 69) {
        printf("Heap corruption detected in free()\n");
        exit(1);
    }
    this_pool->free          = true;
    this_pool->required_size = sizeof(HeapPool);
}

void* realloc(void *addr, size_t sz) {
    void *new = malloc(sz);
    HeapPool *this_pool = (HeapPool*) (((uint64_t)addr) - sizeof(HeapPool));
    if (this_pool->verify != 69) {
        printf("Heap corruption detected in realloc()\n");
        exit(1);
    }
    memcpy(new, addr, this_pool->size);
    free(addr);
    return new;
}

void *calloc(size_t nmemb, size_t sz) {
    return malloc(sz * nmemb);
}

double atof(const char *nptr) {
    printf("TODO: atof() is not yet implemented because SSE2 is not supported in TacOS.\n");
    exit(1);
}

int atoi(const char *nptr) {
    int sign = 0; // set to 1 if negative
    while (*nptr == ' ' || *nptr == '\t' || *nptr == '\n') nptr++;
    if (*nptr == '-') sign = 1, nptr++;
    else if (*nptr == '+') nptr++;
    else if (*nptr < '0' || *nptr > '9') return 0;
    int ret = 0;
    while (*nptr >= '0' && *nptr <= '9') {
        ret = ret * 10 + (*nptr - '0'); 
        nptr++;
    }
    if (sign) return -ret;
    else return ret;
}





