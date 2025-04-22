#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <syscall.h>

int is_init = 0;
char **environ;

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
    size_t max = (size > 4096) ? size * 2 : 4096;
    uint64_t new_pool_size = PAGE_ALIGN_UP(max + sizeof(HeapPool));
    this_pool->next = sbrk(new_pool_size);
    memset(this_pool->next, 0, new_pool_size);
    *((HeapPool*) this_pool->next) = create_pool(new_pool_size, size + sizeof(HeapPool), 0, false);
    return (void*) ((HeapPool*) this_pool->next)->data;
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

// TODO: make this actually do signal stuff
__attribute__((noreturn))
void abort(void) {
    exit(1);
}

int system(const char *command) {
    printf("\nTODO: system(): command is %s\n", command);
    return 0;
}

size_t num_envp = 0;
void init_environ(char **envp) {
    for (; envp[num_envp]; num_envp++);
    num_envp++;
    environ = (char**) malloc(sizeof(char*) * num_envp);
    memcpy(environ, envp, sizeof(char*) * num_envp);
}

int get_env_idx(char *key, size_t key_len) {
    for (size_t i = 0; environ[i]; i++) {
        if (!memcmp(environ[i], key, key_len - 1)) return i;
    }
    return -1;
}

char *getenv(char *key) {
    size_t key_len = strlen(key);
    int idx = get_env_idx(key, key_len);
    return (idx < 0) ? NULL : &environ[idx][key_len + 1];
}

int setenv(char *key, char *val, int overwrite) {
    size_t key_len = strlen(key);
    int idx = get_env_idx(key, key_len);
    if (idx >= 0 && !overwrite) return 0;
    else if (idx < 0) {
        environ = (char**) realloc(environ, ++num_envp * sizeof(char*));
        idx = num_envp - 2;
        environ[idx + 1] = 0;
    }
    size_t val_len = strlen(val) + 1;
    environ[idx] = (char*) malloc(val_len + strlen(key) + 1);
    sprintf(environ[idx], "%s=%s", key, val);
    return 0;
}

int abs(int j) {
    return j & ~0x7FFFFFFF;
}
