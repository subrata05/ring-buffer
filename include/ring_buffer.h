/**
 * @file    ring_buffer.h
 * @author  subrata05
 * @brief   Lock-free single-producer/single-consumer (SPSC) ring buffer
 * 
 * A thread-safe ring buffer implementation using C11 atomics with 
 * acquire-release semantics. Requires buffer size to be power of 2.
 * 
 * @note    This is NOT a multi-producer/multi-consumer queue. Only 
 *          one thread may write and one thread may read concurrently.
 * 
 * @warning Buffer size MUST be power of 2 (e.g., 16, 32, 64, 128, 256, 
 *          512, 1024, etc.) due to bitwise indexing optimization.
 */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Lock-free ring buffer instance structure (Single-Producer/Single-Consumer)
 * 
 * This implementation uses atomic head/tail indices with acquire-release 
 * semantics for thread-safe operation without mutexes. Buffer size must 
 * be a power of 2 to enable fast bitwise masking instead of modulo.
 */
typedef struct {
    uint8_t        *buffer;     // Pointer to the data buffer
    size_t          size;       // Total capacity (must be power of 2)
    size_t          mask;       // Bitmask for fast indexing (size - 1)
    _Atomic size_t  head;       // Write index (only modified by producer)
    _Atomic size_t  tail;       // Read index (only modified by consumer)
} ring_buff_t;

/**
 * Initialize ring buffer
 * 
 * @param rb    Pointer to ring buffer structure to initialize
 * @param buf   Pointer to allocated memory (must be rb->size bytes)
 * @param size  Size of buffer in bytes (must be power of 2, e.g., 16, 32, 64, 128...)
 * @return      true on success, false on invalid parameters or non-power-of-2 size
 * 
 * @note        Size restriction enables bitwise AND masking: index & mask vs index % size
 */
bool ring_buff_init(ring_buff_t *rb, uint8_t *buf, size_t size);

/**
 * Reset ring buffer to empty state (discards all data)
 * 
 * @param rb    Pointer to ring buffer
 * 
 * @warning     Not thread-safe with concurrent operations. Ensure producer 
 *              and consumer are idle before calling. Uses seq_cst fence 
 *              to synchronize with any ongoing atomic operations.
 */
void ring_buff_reset(ring_buff_t *rb);

/**
 * Check if ring buffer is empty
 * 
 * @param rb    Pointer to ring buffer
 * @return      true if empty, false otherwise
 * 
 * @note        Thread-safe for consumer (reader) context. Consumer loads 
 *              tail relaxed, head acquire — sees all prior producer writes.
 */
bool ring_buff_is_empty(const ring_buff_t *rb);

/**
 * Check if ring buffer is full
 * 
 * @param rb    Pointer to ring buffer
 * @return      true if full, false otherwise
 * 
 * @note        Thread-safe for producer (writer) context. Producer loads 
 *              head relaxed, tail acquire — sees all prior consumer reads.
 */
bool ring_buff_is_full(const ring_buff_t *rb);

/**
 * Get number of bytes currently stored in buffer
 * 
 * @param rb    Pointer to ring buffer
 * @return      Number of bytes available to read
 * 
 * @note        Thread-safe for consumer context. May return stale value 
 *              if called concurrently with producer (indicates minimum available).
 */
size_t ring_buff_count(const ring_buff_t *rb);

/**
 * Get available space in buffer
 * 
 * @param rb    Pointer to ring buffer
 * @return      Number of bytes that can be written
 * 
 * @note        Thread-safe for producer context. May return stale value 
 *              if called concurrently with consumer (indicates minimum space).
 */
size_t ring_buff_available(const ring_buff_t *rb);

/**
 * Get total capacity of buffer
 * 
 * @param rb    Pointer to ring buffer
 * @return      Total size of buffer in bytes (power of 2)
 */
size_t ring_buff_capacity(const ring_buff_t *rb);

/**
 * Write a single byte to buffer
 * 
 * @param rb    Pointer to ring buffer
 * @param byte  Byte to write
 * @return      true if written, false if buffer full
 * 
 * @warning     Only call from producer (writer) context. Not reentrant 
 *              for multiple producers — single-producer/single-consumer only.
 */
bool ring_buff_put(ring_buff_t *rb, uint8_t byte);

/**
 * Read a single byte from buffer
 * 
 * @param rb    Pointer to ring buffer
 * @param byte  Pointer to store read byte
 * @return      true if read, false if buffer empty
 * 
 * @warning     Only call from consumer (reader) context. Not reentrant 
 *              for multiple consumers — single-producer/single-consumer only.
 */
bool ring_buff_get(ring_buff_t *rb, uint8_t *byte);

/**
 * Write multiple bytes to buffer
 * 
 * @param rb    Pointer to ring buffer
 * @param data  Pointer to data to write
 * @param len   Number of bytes to write
 * @return      Number of bytes actually written (may be less than len if buffer full)
 * 
 * @warning     Only call from producer context. Handles wrap-around internally 
 *              via two-part memcpy. Uses release semantics to ensure data 
 *              is visible to consumer before head index update.
 */
size_t ring_buff_write(ring_buff_t *rb, const uint8_t *data, size_t len);

/**
 * Read multiple bytes from buffer
 * 
 * @param rb    Pointer to ring buffer
 * @param data  Pointer to buffer to store read data
 * @param len   Maximum number of bytes to read
 * @return      Number of bytes actually read
 * 
 * @warning     Only call from consumer context. Handles wrap-around internally 
 *              via two-part memcpy. Uses release semantics to ensure tail 
 *              update is visible to producer.
 */
size_t ring_buff_read(ring_buff_t *rb, uint8_t *data, size_t len);

/**
 * Get pointer to linear contiguous read buffer (zero-copy/DMA support)
 * 
 * @param rb    Pointer to ring buffer
 * @param ptr   Pointer to store buffer pointer (NULL if no data available)
 * @return      Number of contiguous bytes available to read (until wrap or empty)
 * 
 * @note        Returns pointer to raw buffer — data remains in ring buffer 
 *              until ring_buff_advance_read() or ring_buff_read() called.
 *              Useful for DMA transfers or zero-copy processing.
 * 
 * @warning     Only call from consumer context. Returned pointer valid until 
 *              next consumer operation. Concurrent producer may write new data 
 *              after returned count (check ring_buff_count() for true available).
 */
size_t ring_buff_get_read_ptr(const ring_buff_t *rb, uint8_t **ptr);

/**
 * Get pointer to linear contiguous write buffer (zero-copy/DMA support)
 * 
 * @param rb    Pointer to ring buffer
 * @param ptr   Pointer to store buffer pointer (NULL if buffer full)
 * @return      Number of contiguous bytes available to write (until wrap or full)
 * 
 * @note        Returns pointer to raw buffer — space reserved but not marked 
 *              as written until ring_buff_advance_write() called. Useful for 
 *              DMA transfers or external data production.
 * 
 * @warning     Only call from producer context. Do not write beyond returned 
 *              count. Call ring_buff_advance_write() after writing to commit.
 */
size_t ring_buff_get_write_ptr(const ring_buff_t *rb, uint8_t **ptr);

/**
 * Advance read pointer after external read (use with ring_buff_get_read_ptr)
 * 
 * @param rb    Pointer to ring buffer
 * @param len   Number of bytes to advance (clamped to available count)
 * 
 * @note        Commits external read operation. Must be called after 
 *              ring_buff_get_read_ptr() to release buffer space to producer.
 */
void ring_buff_advance_read(ring_buff_t *rb, size_t len);

/**
 * Advance write pointer after external write (use with ring_buff_get_write_ptr)
 * 
 * @param rb    Pointer to ring buffer
 * @param len   Number of bytes to advance (clamped to available space)
 * 
 * @note        Commits external write operation. Must be called after 
 *              ring_buff_get_write_ptr() to make data visible to consumer.
 */
void ring_buff_advance_write(ring_buff_t *rb, size_t len);

#ifdef __cplusplus
}
#endif

#endif // RING_BUFFER_H