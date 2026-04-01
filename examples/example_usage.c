/**
 * @file    example.c
 * @author  subrata05
 * @brief   Basic usage example for lock-free ring buffer
 */

#include <stdio.h>
#include <string.h>
#include "ring_buffer.h"

int main(void) {
    uint8_t buff_mem[64];
    ring_buff_t rb;
    uint8_t *ptr;  // For zero-copy API
    
    ring_buff_init(&rb, buff_mem, sizeof(buff_mem));
    
    printf("ring buffer example\n");
    printf("capacity: %zu bytes\n\n", ring_buff_capacity(&rb));
    
    // simulate receiving data (e.g., from UART)
    const char *messages[] = {
        "Hello",
        "World",
        "Ring",
        "Buffer",
        "Subrata",
        "FAAAAH"
    };
    
    // write messages
    printf("writing messages:\n");
    for (int i = 0; i < 6; i++) {
        size_t written = ring_buff_write(&rb, (uint8_t*)messages[i], strlen(messages[i]));
        printf("  wrote '%s' (%zu bytes)\n", messages[i], written);
    }
    
    printf("\nbuff status: %zu/%zu bytes used\n", 
           ring_buff_count(&rb), ring_buff_capacity(&rb));
    
    // read and process using zero-copy API
    printf("\nreading messages (zero-copy):\n");
    while (!ring_buff_is_empty(&rb)) {
        // get pointer to contiguous data without copying
        size_t available = ring_buff_get_read_ptr(&rb, &ptr);
        printf("  contiguous block available: %zu bytes\n", available);
        
        // print directly from buffer (no memcpy needed for display)
        printf("  data: '");
        for (size_t i = 0; i < available; i++) {
            putchar(ptr[i]);
        }
        printf("'\n");
        
        // advance to consume what we just printed
        ring_buff_advance_read(&rb, available);
    }
    
    printf("\nbuff empty: %s\n", ring_buff_is_empty(&rb) ? "yes" : "no");
    
    // demonstrate bulk read alternative
    printf("\n--- bulk read demo ---\n");
    ring_buff_write(&rb, (uint8_t*)"bulk demo", 9);
    
    uint8_t chunk[32];
    size_t read = ring_buff_read(&rb, chunk, sizeof(chunk));
    chunk[read] = '\0';
    printf("bulk read: '%s'\n", chunk);
    
    printf("\nbuff empty: %s\n", ring_buff_is_empty(&rb) ? "yes" : "no");
    
    return 0;
}