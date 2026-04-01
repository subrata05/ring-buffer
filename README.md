# 🔄 Ring Buffer

> A **lock-free** circular buffer implementation in C — built for embedded systems, real-time applications, and high-performance concurrent scenarios.

---

## ✨ Features

| | |
|---|---|
| 🚫 **Zero dynamic allocation** | No `malloc` or `free` — ever |
| ⚡ **Lock-free SPSC** | Single-producer/single-consumer via C11 atomics |
| 🔒 **Thread-safe** | No mutexes needed — acquire-release memory ordering |
| 🔁 **Seamless wrap-around** | Handled automatically, no data copying required |
| 📦 **Zero-copy API** | Direct pointer access for DMA and high-throughput scenarios |
| ⏱️ **Fully deterministic** | Constant-time execution on every operation |
| 🚀 **Fast indexing** | Bitwise masking (power-of-2 sizing) |

---

## ⚠️ Requirements

- **C11 compiler** (`_Atomic`, `stdatomic.h`)
- **Power-of-2 buffer size** (e.g., 16, 32, 64, 128, 256, 512, 1024...)

---

## 🚀 Quick Start

```c
#include "ring_buffer.h"
#include <stdio.h>

int main(void) {
    // Buffer size MUST be power of 2
    uint8_t buffer_memory[256];
    ring_buff_t rb;

    // Initialize
    ring_buff_init(&rb, buffer_memory, sizeof(buffer_memory));

    // Write data
    uint8_t data[] = "Hello, World!";
    ring_buff_write(&rb, data, sizeof(data));

    // Read data back
    uint8_t output[50];
    size_t bytes_read = ring_buff_read(&rb, output, sizeof(output));

    return 0;
}
```

---

## 📖 API Reference

### Initialization & Management

| Function | Description |
|---|---|
| `ring_buff_init(rb, buf, size)` | Initialize buffer with pre-allocated memory (size must be power of 2) |
| `ring_buff_reset(rb)` | Clear all data (not thread-safe with active operations) |
| `ring_buff_capacity(rb)` | Get total buffer size in bytes |
| `ring_buff_count(rb)` | Get bytes stored (consumer context) |
| `ring_buff_available(rb)` | Get bytes writable (producer context) |
| `ring_buff_is_empty(rb)` | Check if buffer has no data |
| `ring_buff_is_full(rb)` | Check if buffer cannot accept more data |

### Data Operations

| Function | Description |
|---|---|
| `ring_buff_write(rb, data, len)` | Write multiple bytes — returns bytes actually written |
| `ring_buff_read(rb, data, len)` | Read and remove bytes — returns bytes actually read |
| `ring_buff_put(rb, byte)` | Write a single byte |
| `ring_buff_get(rb, byte)` | Read a single byte |

### ⚡ Zero-Copy Operations (DMA-friendly)

| Function | Description |
|---|---|
| `ring_buff_get_write_ptr(rb, &ptr)` | Get pointer to contiguous write space |
| `ring_buff_get_read_ptr(rb, &ptr)` | Get pointer to contiguous read space |
| `ring_buff_advance_write(rb, len)` | Commit external write (advance write pointer) |
| `ring_buff_advance_read(rb, len)` | Commit external read (advance read pointer) |

---

## 🧵 Thread Safety Model

**Single-Producer / Single-Consumer (SPSC) only**

| Context | Functions | Safety |
|---|---|---|
| Producer (writer) | `put`, `write`, `get_write_ptr`, `advance_write`, `available`, `is_full` | Call from one thread only |
| Consumer (reader) | `get`, `read`, `get_read_ptr`, `advance_read`, `count`, `is_empty` | Call from one thread only |
| Either | `capacity`, `init`, `reset` | Call when no concurrent access |

> ⚠️ Do not use from multiple producers or multiple consumers simultaneously.

---

## 💡 Usage Examples

### Basic Read / Write

```c
uint8_t buffer[64];  // Power of 2
ring_buff_t rb;
ring_buff_init(&rb, buffer, 64);

// Write a string
const char *msg = "hello";
ring_buff_write(&rb, (uint8_t *)msg, strlen(msg));

// Read into array
uint8_t output[32];
size_t n = ring_buff_read(&rb, output, sizeof(output));
// n == 5, output contains "hello"
```

### Interrupt-Driven UART (SPSC)

```c
// ISR context — producer
void uart_rx_isr(uint8_t byte) {
    ring_buff_put(&uart_rx_buffer, byte);  // Lock-free, safe in ISR
}

// Main loop — consumer
void process_data(void) {
    uint8_t byte;
    while (ring_buff_get(&uart_rx_buffer, &byte)) {
        handle_byte(byte);
    }
}
```

### Zero-Copy DMA Transfer

```c
uint8_t *ptr;
size_t space = ring_buff_get_write_ptr(&rb, &ptr);

// Configure DMA to write 'space' bytes to 'ptr'
// ... DMA completes ...

ring_buff_advance_write(&rb, dma_bytes_written);
```

---

## 🔧 Building & Testing

### Requirements

- GCC or compatible C11 compiler
- GNU Make

### Build

```bash
make          # build tests and example
make test     # run unit tests
make example  # run usage example
make static   # build static library (libringbuffer.a)
make debug    # build with debug symbols
make clean    # remove build artifacts
```

### Running Tests

```bash
$ make test
./build/test_ring_buffer

>> RUNNING RING BUFFER DRIVER TEST
>> Lock-free SPSC version (power-of-2 sizing required)

>> TEST: basic init
PASS: init with power-of-2 should succeed
...

>> YAAY! ALL TESTS PASSED
```

---

## 📊 Memory Footprint

| Metric | Value |
|---|---|
| Code size | ~1–2 KB (depends on functions used) |
| RAM overhead | 24 bytes per instance (32-bit) / 32 bytes (64-bit) |
| Data buffer | User-provided — power of 2 |
| Heap usage | Zero — no `malloc` / `free` |
| Lock overhead | Zero — no mutexes/semaphores |