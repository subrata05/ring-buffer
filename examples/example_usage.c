#include <stdio.h>
#include <string.h>
#include "ring_buffer.h"

int main(void) {
    uint8_t buff_mem[64];
    ring_buff_t rb;
    
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
    
    // read and process
    printf("\nreading messages:\n");
    uint8_t temp[32];
    while (!ring_buff_is_empty(&rb)) {
        // peek to see what's available
        size_t available = ring_buff_get_read_ptr(&rb, &(uint8_t*){temp});
        printf("  contiguous block available: %zu bytes\n", available);
        
        // read a chunk
        uint8_t chunk[10];
        size_t read = ring_buff_read(&rb, chunk, sizeof(chunk) - 1);
        chunk[read] = '\0';  // null terminate for printing
        printf("  read: '%s'\n", chunk);
    }
    
    printf("\nbuff empty: %s\n", ring_buff_is_empty(&rb) ? "yes" : "no");
    
    return 0;
}