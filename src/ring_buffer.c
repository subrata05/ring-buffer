/**
 * @file    ring_buffer.c
 * @author  subrata05
 * @brief   Lock-free SPSC ring buffer implementation
 */

#include "ring_buffer.h"
#include <string.h>

static inline bool is_power_of_two(size_t n)
{
    return (n != 0u) && ((n & (n - 1u)) == 0u);
}

/* Unsigned subtraction handles wraparound automatically */
static inline size_t count_from_heads(size_t head, size_t tail)
{
    return head - tail;
}

bool ring_buff_init(ring_buff_t *rb, uint8_t *buf, size_t size)
{
    if (rb == NULL || buf == NULL || !is_power_of_two(size)) {
        return false;
    }
    rb->buffer = buf;
    rb->size = size;
    rb->mask = size - 1u;
    atomic_init(&rb->head, 0u);
    atomic_init(&rb->tail, 0u);
    return true;
}

void ring_buff_reset(ring_buff_t *rb)
{
    if (rb == NULL) return;
    atomic_store_explicit(&rb->head, 0u, memory_order_relaxed);
    atomic_store_explicit(&rb->tail, 0u, memory_order_relaxed);
    atomic_thread_fence(memory_order_seq_cst);
}

bool ring_buff_is_empty(const ring_buff_t *rb)
{
    if (rb == NULL) return true;
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);
    return (head == tail);
}

bool ring_buff_is_full(const ring_buff_t *rb)
{
    if (rb == NULL) return false;
    size_t head = atomic_load_explicit(&rb->head, memory_order_relaxed);
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);
    return (count_from_heads(head, tail) >= rb->size);
}

size_t ring_buff_count(const ring_buff_t *rb)
{
    if (rb == NULL) return 0u;
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);
    return count_from_heads(head, tail);
}

size_t ring_buff_available(const ring_buff_t *rb)
{
    if (rb == NULL) return 0u;
    size_t head = atomic_load_explicit(&rb->head, memory_order_relaxed);
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);
    return rb->size - count_from_heads(head, tail);
}

size_t ring_buff_capacity(const ring_buff_t *rb)
{
    if (rb == NULL) return 0u;
    return rb->size;
}

/* Producer: load head relaxed, tail acquire (see consumer's progress), release head */
bool ring_buff_put(ring_buff_t *rb, uint8_t byte)
{
    if (rb == NULL) return false;
    size_t head = atomic_load_explicit(&rb->head, memory_order_relaxed);
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);
    if (count_from_heads(head, tail) >= rb->size) return false;
    rb->buffer[head & rb->mask] = byte;
    atomic_store_explicit(&rb->head, head + 1u, memory_order_release);
    return true;
}

/* Consumer: load tail relaxed, head acquire (see producer's progress), release tail */
bool ring_buff_get(ring_buff_t *rb, uint8_t *byte)
{
    if (rb == NULL || byte == NULL) return false;
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);
    if (tail == head) return false;
    *byte = rb->buffer[tail & rb->mask];
    atomic_store_explicit(&rb->tail, tail + 1u, memory_order_release);
    return true;
}

/* Write in two chunks if wrapping, single release commits all */
size_t ring_buff_write(ring_buff_t *rb, const uint8_t *data, size_t len)
{
    if (rb == NULL || data == NULL || len == 0u) return 0u;
    size_t head = atomic_load_explicit(&rb->head, memory_order_relaxed);
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);
    size_t available = rb->size - count_from_heads(head, tail);
    size_t to_write = (len < available) ? len : available;
    if (to_write == 0u) return 0u;
    
    size_t head_idx = head & rb->mask;
    size_t first_part = rb->size - head_idx;
    if (first_part > to_write) first_part = to_write;
    
    memcpy(&rb->buffer[head_idx], data, first_part);
    if (first_part < to_write) {
        memcpy(rb->buffer, &data[first_part], to_write - first_part);
    }
    
    atomic_store_explicit(&rb->head, head + to_write, memory_order_release);
    return to_write;
}

/* Read in two chunks if wrapping, single release commits all */
size_t ring_buff_read(ring_buff_t *rb, uint8_t *data, size_t len)
{
    if (rb == NULL || data == NULL || len == 0u) return 0u;
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);
    size_t count = count_from_heads(head, tail);
    size_t to_read = (len < count) ? len : count;
    if (to_read == 0u) return 0u;
    
    size_t tail_idx = tail & rb->mask;
    size_t first_part = rb->size - tail_idx;
    if (first_part > to_read) first_part = to_read;
    
    memcpy(data, &rb->buffer[tail_idx], first_part);
    if (first_part < to_read) {
        memcpy(&data[first_part], rb->buffer, to_read - first_part);
    }
    
    atomic_store_explicit(&rb->tail, tail + to_read, memory_order_release);
    return to_read;
}

/* Returns contiguous chunk at tail, clamped to available count */
size_t ring_buff_get_read_ptr(const ring_buff_t *rb, uint8_t **ptr)
{
    if (ptr != NULL) *ptr = NULL;
    if (rb == NULL || ptr == NULL) return 0u;
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);
    size_t count = count_from_heads(head, tail);
    if (count == 0u) return 0u;
    size_t tail_idx = tail & rb->mask;
    size_t contiguous = rb->size - tail_idx;
    if (contiguous > count) contiguous = count;
    *ptr = &rb->buffer[tail_idx];
    return contiguous;
}

/* Returns contiguous space at head, clamped to available space */
size_t ring_buff_get_write_ptr(const ring_buff_t *rb, uint8_t **ptr)
{
    if (ptr != NULL) *ptr = NULL;
    if (rb == NULL || ptr == NULL) return 0u;
    size_t head = atomic_load_explicit(&rb->head, memory_order_relaxed);
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);
    size_t available = rb->size - count_from_heads(head, tail);
    if (available == 0u) return 0u;
    size_t head_idx = head & rb->mask;
    size_t contiguous = rb->size - head_idx;
    if (contiguous > available) contiguous = available;
    *ptr = &rb->buffer[head_idx];
    return contiguous;
}

void ring_buff_advance_read(ring_buff_t *rb, size_t len)
{
    if (rb == NULL || len == 0u) return;
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);
    size_t count = count_from_heads(head, tail);
    if (len > count) len = count;
    atomic_store_explicit(&rb->tail, tail + len, memory_order_release);
}

void ring_buff_advance_write(ring_buff_t *rb, size_t len)
{
    if (rb == NULL || len == 0u) return;
    size_t head = atomic_load_explicit(&rb->head, memory_order_relaxed);
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);
    size_t available = rb->size - count_from_heads(head, tail);
    if (len > available) len = available;
    atomic_store_explicit(&rb->head, head + len, memory_order_release);
}