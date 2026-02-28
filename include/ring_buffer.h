#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ring buff instance structure */

typedef struct {
    uint8_t *buffer;      // ptr to the data buff
    size_t head;          // wr index
    size_t tail;          // rd index
    size_t size;          // total capacity of buff
    size_t count;         // current number of elements
    bool full;            // flag to distinguish full vs empty when head == tail
} ring_buff_t;

/**
 * init ring buff
 * 
 * @param rb    ptr to ring buff structure to initialize
 * @param buf   ptr to allocated memory for the buff (must be rb->size bytes)
 * @param size  size of the buff in bytes
 * @return      true on success, false on invalid parameters
 */
bool ring_buff_init(ring_buff_t *rb, uint8_t *buf, size_t size);

/**
 * reset ring buff to empty state (discards all data)
 * 
 * @param rb    ptr to ring buff
 */
void ring_buff_reset(ring_buff_t *rb);

/**
 * check if ring buff is empty
 * 
 * @param rb    ptr to ring buff
 * @return      true if empty, false otherwise
 */
bool ring_buff_is_empty(const ring_buff_t *rb);

/**
 * check if ring buf is full
 * 
 * @param rb    prt to ring buff
 * @return      true if full, false otherwise
 */
bool ring_buff_is_full(const ring_buff_t *rb);

/**
 * get num of bytes currently stored in buff
 * 
 * @param rb    ptr to ring buff
 * @return      num of bytes available to read
 */
size_t ring_buff_count(const ring_buff_t *rb);

/**
 * get available space in buff
 * 
 * @param rb    ptr to ring buff
 * @return      num of bytes that can be written
 */
size_t ring_buff_available(const ring_buff_t *rb);

/**
 * get total capacity of buff
 * 
 * @param rb    ptr to ring buff
 * @return      total size of buf in bytes
 */
size_t ring_buff_capacity(const ring_buff_t *rb);

/**
 * wr data to ring buff
 * 
 * @param rb    ptr to ring buff
 * @param data  ptr to data to write
 * @param len   num of bytes to write
 * @return      num of bytes actually written (may be less than len if buff full)
 */
size_t ring_buff_write(ring_buff_t *rb, const uint8_t *data, size_t len);

/**
 * read data from ring buff
 * 
 * @param rb    ptr to ring buff
 * @param data  ptr to buff to store read data
 * @param len   max num of bytes to read
 * @return      num of bytes actually read
 */
size_t ring_buff_read(ring_buff_t *rb, uint8_t *data, size_t len);

/**
 * peek at data without removing it from buff
 * 
 * @param rb    ptr to ring buff
 * @param data  ptr to buf to store peeked data
 * @param len   max num of bytes to peek
 * @return      num of bytes actually peeked
 */
size_t ring_buff_peek(const ring_buff_t *rb, uint8_t *data, size_t len);

/**
 * skip or discard bytes from buff without reading
 * 
 * @param rb    ptr to ring buff
 * @param len   num of bytes to skip
 * @return      num of bytes actually skipped
 */
size_t ring_buff_skip(ring_buff_t *rb, size_t len);

/**
 * write a single byte to buff
 * 
 * @param rb    ptr to ring buff
 * @param byte  byte to write
 * @return      true if written, false if buff full
 */
bool ring_buff_put(ring_buff_t *rb, uint8_t byte);

/**
 * read a single byte from buff
 * 
 * @param rb    ptr to ring buff
 * @param byte  ptr to store read byte
 * @return      true if read, false if buff empty
 */
bool ring_buff_get(ring_buff_t *rb, uint8_t *byte);

/**
 * get ptr to linear contiguous read buff (if available)
 * useful in DMA or zero-copy operations
 * 
 * @param rb    ptr to ring buff
 * @param ptr   ptr to store buf ptr
 * @return      num of contiguous bytes available to read
 */
size_t ring_buff_get_read_ptr(const ring_buff_t *rb, uint8_t **ptr);

/**
 * get ptr to linear contiguous write buff (if available)
 * useful in DMA or zero-copy operations
 * 
 * @param rb    ptr to ring buffer
 * @param ptr   ptr to store buffer ptr
 * @return      num of contiguous bytes available to write
 */
size_t ring_buff_get_write_ptr(const ring_buff_t *rb, uint8_t **ptr);

/**
 * advance read ptr after external read (use with get_read_ptr)
 * 
 * @param rb    ptr to ring buffer
 * @param len   num of bytes to advance
 */
void ring_buff_advance_read(ring_buff_t *rb, size_t len);

/**
 * advanced write ptr after external write (use with get_write_ptr)
 * 
 * @param rb    ptr to ring buffer
 * @param len   num of bytes to advance
 */
void ring_buff_advance_write(ring_buff_t *rb, size_t len);

#ifdef __cplusplus
}
#endif

#endif // RING_BUFFER_H