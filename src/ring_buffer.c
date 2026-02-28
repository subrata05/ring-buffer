#include "ring_buffer.h"
#include <string.h>

bool ring_buff_init(ring_buff_t *rb, uint8_t *buf, size_t size)
{
    if (rb == NULL || buf == NULL || size == 0) {
        return false;
    }
    
    rb->buffer = buf;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    rb->full = false;
    
    return true;
}

void ring_buff_reset(ring_buff_t *rb)
{
    if (rb == NULL) {
        return;
    }
    
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    rb->full = false;
}

bool ring_buff_is_empty(const ring_buff_t *rb)
{
    if (rb == NULL) {
        return true;
    }
    return (rb->count == 0);
}

bool ring_buff_is_full(const ring_buff_t *rb)
{
    if (rb == NULL) {
        return false;
    }
    return (rb->count == rb->size);
}

size_t ring_buff_count(const ring_buff_t *rb)
{
    if (rb == NULL) {
        return 0;
    }
    return rb->count;
}

size_t ring_buff_available(const ring_buff_t *rb)
{
    if (rb == NULL) {
        return 0;
    }
    return rb->size - rb->count;
}

size_t ring_buff_capacity(const ring_buff_t *rb)
{
    if (rb == NULL) {
        return 0;
    }
    return rb->size;
}

bool ring_buff_put(ring_buff_t *rb, uint8_t byte)
{
    if (rb == NULL || rb->count >= rb->size) {
        return false;
    }
    
    rb->buffer[rb->head] = byte;
    rb->head = (rb->head + 1) % rb->size;
    rb->count++;
    
    return true;
}

bool ring_buff_get(ring_buff_t *rb, uint8_t *byte)
{
    if (rb == NULL || byte == NULL || rb->count == 0) {
        return false;
    }
    
    *byte = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->size;
    rb->count--;
    
    return true;
}

size_t ring_buff_write(ring_buff_t *rb, const uint8_t *data, size_t len)
{
    if (rb == NULL || data == NULL || len == 0) {
        return 0;
    }
    
    size_t available = rb->size - rb->count;
    size_t to_write = (len < available) ? len : available;
    
    if (to_write == 0) {
        return 0;
    }
    
    // handle wrap-around: write in two parts if necessary
    size_t first_part = rb->size - rb->head;
    if (first_part > to_write) {
        first_part = to_write;
    }
    
    memcpy(&rb->buffer[rb->head], data, first_part);
    
    if (first_part < to_write) {
        // wrap around to beginning
        size_t second_part = to_write - first_part;
        memcpy(rb->buffer, &data[first_part], second_part);
        rb->head = second_part;
    } else {
        rb->head = (rb->head + to_write) % rb->size;
    }
    
    rb->count += to_write;
    
    return to_write;
}

size_t ring_buff_read(ring_buff_t *rb, uint8_t *data, size_t len)
{
    if (rb == NULL || data == NULL || len == 0 || rb->count == 0) {
        return 0;
    }
    
    size_t to_read = (len < rb->count) ? len : rb->count;
    
    // handle wrap-around: read in two parts if necessary
    size_t first_part = rb->size - rb->tail;
    if (first_part > to_read) {
        first_part = to_read;
    }
    
    memcpy(data, &rb->buffer[rb->tail], first_part);
    
    if (first_part < to_read) {
        // wrap around to beginning
        size_t second_part = to_read - first_part;
        memcpy(&data[first_part], rb->buffer, second_part);
        rb->tail = second_part;
    } else {
        rb->tail = (rb->tail + to_read) % rb->size;
    }
    
    rb->count -= to_read;
    
    return to_read;
}

size_t ring_buff_peek(const ring_buff_t *rb, uint8_t *data, size_t len)
{
    if (rb == NULL || data == NULL || len == 0 || rb->count == 0) {
        return 0;
    }
    
    size_t to_peek = (len < rb->count) ? len : rb->count;
    size_t tail = rb->tail;
    
    // randle wrap-around: peek in two parts if necessary
    size_t first_part = rb->size - tail;
    if (first_part > to_peek) {
        first_part = to_peek;
    }
    
    memcpy(data, &rb->buffer[tail], first_part);
    
    if (first_part < to_peek) {
        size_t second_part = to_peek - first_part;
        memcpy(&data[first_part], rb->buffer, second_part);
    }
    
    return to_peek;
}

size_t ring_buff_skip(ring_buff_t *rb, size_t len)
{
    if (rb == NULL || len == 0 || rb->count == 0) {
        return 0;
    }
    
    size_t to_skip = (len < rb->count) ? len : rb->count;
    rb->tail = (rb->tail + to_skip) % rb->size;
    rb->count -= to_skip;
    
    return to_skip;
}

size_t ring_buff_get_read_ptr(const ring_buff_t *rb, uint8_t **ptr)
{
    if (rb == NULL || ptr == NULL || rb->count == 0) {
        if (ptr != NULL) {
            *ptr = NULL;
        }
        return 0;
    }
    
    *ptr = &rb->buffer[rb->tail];
    
    // return contiguous bytes available (until wrap or end of data)
    size_t contiguous = rb->size - rb->tail;
    if (contiguous > rb->count) {
        contiguous = rb->count;
    }
    
    return contiguous;
}

size_t ring_buff_get_write_ptr(const ring_buff_t *rb, uint8_t **ptr)
{
    if (rb == NULL || ptr == NULL || rb->count >= rb->size) {
        if (ptr != NULL) {
            *ptr = NULL;
        }
        return 0;
    }
    
    *ptr = &rb->buffer[rb->head];
    
    // return contiguous space available (until wrap or full)
    size_t contiguous = rb->size - rb->head;
    size_t available = rb->size - rb->count;
    if (contiguous > available) {
        contiguous = available;
    }
    
    return contiguous;
}

void ring_buff_advance_read(ring_buff_t *rb, size_t len)
{
    if (rb == NULL || len == 0) {
        return;
    }
    
    if (len > rb->count) {
        len = rb->count;
    }
    
    rb->tail = (rb->tail + len) % rb->size;
    rb->count -= len;
}

void ring_buff_advance_write(ring_buff_t *rb, size_t len)
{
    if (rb == NULL || len == 0) {
        return;
    }
    
    size_t available = rb->size - rb->count;
    if (len > available) {
        len = available;
    }
    
    rb->head = (rb->head + len) % rb->size;
    rb->count += len;
}