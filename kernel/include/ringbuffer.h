#pragma once
#include <stddef.h>

#define RINGBUF_MAX_LEN 4096 * 1000
typedef struct {
    char data[RINGBUF_MAX_LEN];
    size_t read_at;
    size_t write_at;
    size_t avaliable_to_read;
} RingBuffer;


void ringbuffer_init(RingBuffer *rb);
int ringbuf_read(RingBuffer *rb, size_t len, char *buf);
int ringbuf_write(RingBuffer *rb, size_t len, char *buf);
