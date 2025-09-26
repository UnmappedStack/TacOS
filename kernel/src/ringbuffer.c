#include <ringbuffer.h>
#include <printf.h>

void ringbuffer_init(RingBuffer *rb) {
    rb->read_at = rb->write_at = rb->avaliable_to_read = 0;
}

int ringbuf_read(RingBuffer *rb, size_t len, char *buf) {
    for (size_t i = 0; i < len; i++) {
        if (!rb->avaliable_to_read) return i;
        buf[i] = rb->data[rb->read_at++];
        if (rb->read_at >= RINGBUF_MAX_LEN) rb->read_at = 0;
        rb->avaliable_to_read--;
    }
    return len;
}

int ringbuf_write(RingBuffer *rb, size_t len, char *buf) {
    for (size_t i = 0; i < len; i++) {
        rb->data[rb->write_at++] = buf[i];
        if (rb->write_at >= RINGBUF_MAX_LEN) rb->write_at = 0;
        rb->avaliable_to_read++;
    }
    return len;
}

