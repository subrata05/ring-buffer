/**
 * @file    test_ring_buffer.c
 * @author  subrata05
 * @brief   Test suite for lock-free SPSC ring buffer
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "ring_buffer.h"

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s (line %d)\n", msg, __LINE__); \
        return 1; \
    } else { \
        printf("PASS: %s\n", msg); \
    } \
} while(0)

int test_basic_init(void) {
    printf("\n>> TEST: basic init\n");
    
    uint8_t buf[64];  // Power of 2
    ring_buff_t rb;
    
    TEST_ASSERT(ring_buff_init(&rb, buf, 64) == true, "init with power-of-2 should succeed");
    TEST_ASSERT(ring_buff_is_empty(&rb) == true, "should be empty after init");
    TEST_ASSERT(ring_buff_is_full(&rb) == false, "should not be full after init");
    TEST_ASSERT(ring_buff_count(&rb) == 0, "count should be 0");
    TEST_ASSERT(ring_buff_available(&rb) == 64, "available should be 64");
    TEST_ASSERT(ring_buff_capacity(&rb) == 64, "capacity should be 64");
    
    // Test non-power-of-2 rejection
    uint8_t buf2[100];
    ring_buff_t rb2;
    TEST_ASSERT(ring_buff_init(&rb2, buf2, 100) == false, "init with non-power-of-2 should fail");
    
    return 0;
}

int test_single_byte_operations(void) {
    printf("\n>> TEST: single byte operations\n");
    
    uint8_t buf[16];  // Power of 2
    ring_buff_t rb;
    uint8_t byte;
    
    ring_buff_init(&rb, buf, 16);
    
    // test put/get
    TEST_ASSERT(ring_buff_put(&rb, 0x42) == true, "put should succeed");
    TEST_ASSERT(ring_buff_count(&rb) == 1, "count should be 1");
    TEST_ASSERT(ring_buff_get(&rb, &byte) == true, "get should succeed");
    TEST_ASSERT(byte == 0x42, "data should match");
    TEST_ASSERT(ring_buff_is_empty(&rb) == true, "should be empty after get");
    
    // test fill to capacity
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(ring_buff_put(&rb, (uint8_t)i) == true, "fill buffer");
    }
    TEST_ASSERT(ring_buff_is_full(&rb) == true, "should be full");
    TEST_ASSERT(ring_buff_put(&rb, 0xFF) == false, "put on full should fail");
    
    // empty it
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(ring_buff_get(&rb, &byte) == true, "empty buffer");
        TEST_ASSERT(byte == (uint8_t)i, "data should be FIFO order");
    }
    TEST_ASSERT(ring_buff_get(&rb, &byte) == false, "get on empty should fail");
    
    return 0;
}

int test_bulk_operations(void) {
    printf("\n>> TEST: bulk operations\n");
    
    uint8_t buf[128];  // Power of 2
    ring_buff_t rb;
    uint8_t write_data[] = "Hello, This is a test script for this ring buffer written by subrata!";
    uint8_t read_data[128] = {0};
    size_t written, read;
    
    ring_buff_init(&rb, buf, 128);
    
    // write data
    written = ring_buff_write(&rb, write_data, strlen((char*)write_data) + 1);
    printf("wrote %zu bytes\n", written);
    TEST_ASSERT(written == strlen((char*)write_data) + 1, "should write all data");
    TEST_ASSERT(ring_buff_count(&rb) == written, "count should match written");
    
    // read data back
    read = ring_buff_read(&rb, read_data, sizeof(read_data));
    TEST_ASSERT(read == written, "should read same amount");
    TEST_ASSERT(strcmp((char*)read_data, (char*)write_data) == 0, "data should match");
    
    return 0;
}

int test_wrap_around(void) {
    printf("\n>> TEST: wrap around\n");
    
    uint8_t buf[8];
    ring_buff_t rb;
    
    ring_buff_init(&rb, buf, 8);
    
    // fill buffer
    for (int i = 0; i < 8; i++) {
        ring_buff_put(&rb, (uint8_t)(i + 1));
    }
    
    // remove 4 items (tail moves to 4)
    for (int i = 0; i < 4; i++) {
        uint8_t b;
        ring_buff_get(&rb, &b);
    }
    
    // add 4 more items (head wraps around)
    for (int i = 0; i < 4; i++) {
        ring_buff_put(&rb, (uint8_t)(i + 9));
    }
    
    // buffer should now be: [9,10,11,12,5,6,7,8]
    // read all and verify
    uint8_t expected[] = {5, 6, 7, 8, 9, 10, 11, 12};
    for (int i = 0; i < 8; i++) {
        uint8_t b;
        ring_buff_get(&rb, &b);
        TEST_ASSERT(b == expected[i], "wrap-around order should be correct");
    }
    
    return 0;
}

int test_zero_copy_api(void) {
    printf("\n>> TEST: zero copy API\n");
    
    uint8_t buf[16];
    ring_buff_t rb;
    uint8_t *ptr;
    size_t len;
    
    ring_buff_init(&rb, buf, 16);
    
    // get write pointer and write directly
    len = ring_buff_get_write_ptr(&rb, &ptr);
    TEST_ASSERT(len == 16, "should have 16 bytes contiguous space");
    
    // simulate DMA write
    memcpy(ptr, "directWrite", 11);
    ring_buff_advance_write(&rb, 11);
    TEST_ASSERT(ring_buff_count(&rb) == 11, "count should be 11");
    
    // get read pointer and read directly
    len = ring_buff_get_read_ptr(&rb, &ptr);
    TEST_ASSERT(len == 11, "should have 11 bytes to read");
    TEST_ASSERT(memcmp(ptr, "directWrite", 11) == 0, "data should match");
    
    ring_buff_advance_read(&rb, 5);
    TEST_ASSERT(ring_buff_count(&rb) == 6, "count should be 6");
    
    return 0;
}

int test_edge_cases(void) {
    printf("\n>> TEST: edge cases\n");
    
    ring_buff_t rb;
    uint8_t buf[8];  // Power of 2
    uint8_t data[10];
    
    // test NULL pointers
    TEST_ASSERT(ring_buff_init(NULL, buf, 8) == false, "init with NULL rb should fail");
    TEST_ASSERT(ring_buff_init(&rb, NULL, 8) == false, "init with NULL buffer should fail");
    TEST_ASSERT(ring_buff_init(&rb, buf, 0) == false, "init with size 0 should fail");
    
    ring_buff_init(&rb, buf, 8);
    
    // test operations on empty
    TEST_ASSERT(ring_buff_get(&rb, data) == false, "get on empty should fail");
    TEST_ASSERT(ring_buff_read(&rb, data, 10) == 0, "read on empty should return 0");
    
    // test write larger than buffer
    uint8_t large_data[] = "this is way more than 8 bytes";
    size_t written = ring_buff_write(&rb, large_data, sizeof(large_data));
    TEST_ASSERT(written == 8, "should only write 8 bytes");
    TEST_ASSERT(ring_buff_is_full(&rb) == true, "should be full");
    
    // test reset
    ring_buff_reset(&rb);
    TEST_ASSERT(ring_buff_is_empty(&rb) == true, "should be empty after reset");
    TEST_ASSERT(ring_buff_count(&rb) == 0, "count should be 0 after reset");
    
    return 0;
}

int test_partial_operations(void) {
    printf("\n>> TEST: partial operations\n");
    
    uint8_t buf[16];
    ring_buff_t rb;
    uint8_t write_data[] = "IDIDNOTSLEEPTONIGHT:)";
    uint8_t read_data[32];
    
    ring_buff_init(&rb, buf, 16);
    
    // Write 12 bytes
    ring_buff_write(&rb, write_data, 12);
    
    // try to read more than available (request 32, only 12 available)
    size_t read = ring_buff_read(&rb, read_data, 32);
    TEST_ASSERT(read == 12, "should only read available bytes");
    TEST_ASSERT(memcmp(read_data, "IDIDNOTSLEEP", 12) == 0, "data should be correct");
    
    // fill completely
    ring_buff_write(&rb, write_data, 16);
    
    // try to write more than available
    size_t written = ring_buff_write(&rb, write_data, 10);
    TEST_ASSERT(written == 0, "should write 0 when full");
    
    return 0;
}

int test_wraparound_zero_copy(void) {
    printf("\n>> TEST: wraparound with zero-copy API\n");
    
    uint8_t buf[8];
    ring_buff_t rb;
    uint8_t *ptr;
    size_t len;
    
    ring_buff_init(&rb, buf, 8);
    
    // Fill half, remove half (head=4, tail=4, empty)
    for (int i = 0; i < 4; i++) ring_buff_put(&rb, (uint8_t)i);
    for (int i = 0; i < 4; i++) {
        uint8_t b;
        ring_buff_get(&rb, &b);
    }
    
    // Now head=4, tail=4. Add 6 more (wraps: indices 4,5,6,7,0,1)
    uint8_t data[] = {10, 20, 30, 40, 50, 60};
    ring_buff_write(&rb, data, 6);
    
    // Zero-copy read should return contiguous chunk at tail (indices 4-7 = 4 bytes)
    len = ring_buff_get_read_ptr(&rb, &ptr);
    TEST_ASSERT(len == 4, "first contiguous chunk should be 4 bytes");
    TEST_ASSERT(ptr[0] == 10 && ptr[1] == 20 && ptr[2] == 30 && ptr[3] == 40, "data correct");
    
    // Advance 4, next read should wrap to index 0
    ring_buff_advance_read(&rb, 4);
    len = ring_buff_get_read_ptr(&rb, &ptr);
    TEST_ASSERT(len == 2, "second chunk should be 2 bytes at wrap");
    TEST_ASSERT(ptr[0] == 50 && ptr[1] == 60, "wrapped data correct");
    
    return 0;
}

int main(void) {
    printf("\n");
    printf(">> RUNNING RING BUFFER DRIVER TEST\n");
    printf(">> Lock-free SPSC version (power-of-2 sizing required)\n");
    printf("\n");
    
    int failures = 0;
    
    failures += test_basic_init();
    failures += test_single_byte_operations();
    failures += test_bulk_operations();
    failures += test_wrap_around();
    failures += test_zero_copy_api();
    failures += test_edge_cases();
    failures += test_partial_operations();
    failures += test_wraparound_zero_copy();  // New test for wraparound zero-copy
    
    printf("\n");
    if (failures == 0) {
        printf(">> YAAY! ALL TESTS PASSED\n");
    } else {
        printf(">> BRUH! TESTS FAILED: %d\n", failures);
    }
    printf("\n");
    
    return failures;
}