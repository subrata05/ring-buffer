# 🔄 Ring Buffer

> A lightweight, zero-allocation circular buffer implementation in C — built for embedded systems and high-performance applications.

---

## ✨ Features

| | |
|---|---|
| 🚫 **Zero dynamic allocation** | No `malloc` or `free` — ever |
| ⚡ **Interrupt-safe** | Safe for single-producer/single-consumer (SPSC) use |
| 🔁 **Seamless wrap-around** | Handled automatically, no data copying required |
| 📦 **Zero-copy API** | Direct pointer access for DMA and high-throughput scenarios |
| ⏱️ **Fully deterministic** | Constant-time execution on every operation |

---

## 🚀 Quick Start

```c
#include "ring_buffer.h"
#include <stdio.h>

int main(void) {
    // Allocate buffer memory (static, stack, or global — your choice)
    uint8_t buffer_memory[256];
    ring_buffer_t rb;

    // Initialize
    ring_buffer_init(&rb, buffer_memory, sizeof(buffer_memory));

    // Write data
    uint8_t data[] = "Hello, World!";
    ring_buffer_write(&rb, data, sizeof(data));

    // Read data back
    uint8_t output[50];
    size_t bytes_read = ring_buffer_read(&rb, output, sizeof(output));

    return 0;
}
```

---

## 📖 API Reference

### Initialization & Management

| Function | Description |
|---|---|
| `ring_buffer_init(rb, buf, size)` | Initialize buffer with pre-allocated memory |
| `ring_buffer_reset(rb)` | Clear all data and reset to empty state |
| `ring_buffer_capacity(rb)` | Get total buffer size in bytes |
| `ring_buffer_count(rb)` | Get number of bytes currently stored |
| `ring_buffer_available(rb)` | Get number of bytes that can still be written |
| `ring_buffer_is_empty(rb)` | Check if buffer has no data |
| `ring_buffer_is_full(rb)` | Check if buffer cannot accept more data |

### Data Operations

| Function | Description |
|---|---|
| `ring_buffer_write(rb, data, len)` | Write multiple bytes — returns bytes actually written |
| `ring_buffer_read(rb, data, len)` | Read and remove bytes — returns bytes actually read |
| `ring_buffer_peek(rb, data, len)` | Non-destructive read (data remains in buffer) |
| `ring_buffer_skip(rb, len)` | Discard bytes without reading |
| `ring_buffer_put(rb, byte)` | Write a single byte |
| `ring_buffer_get(rb, byte)` | Read a single byte |

### ⚡ Zero-Copy Operations *(Advanced)*

| Function | Description |
|---|---|
| `ring_buffer_get_write_ptr(rb, &ptr)` | Get pointer to contiguous write space |
| `ring_buffer_get_read_ptr(rb, &ptr)` | Get pointer to contiguous read space |
| `ring_buffer_advance_write(rb, len)` | Advance write pointer after external write |
| `ring_buffer_advance_read(rb, len)` | Advance read pointer after external read |

---

## 💡 Usage Examples

### Basic Read / Write

```c
uint8_t buffer[64];
ring_buffer_t rb;
ring_buffer_init(&rb, buffer, 64);

// write a string
const char *msg = "hello";
ring_buffer_write(&rb, (uint8_t *)msg, strlen(msg));

// read into array
uint8_t output[32];
size_t n = ring_buffer_read(&rb, output, sizeof(output));
// n == 5, output contains "hello"
```

### Single Byte Operations *(UART-style)*

```c
// ISR or polling loop — receiving bytes
void uart_rx_isr(uint8_t byte) {
    ring_buffer_put(&uart_rx_buffer, byte);
}

// main loop: processing received bytes
void process_data(void) {
    uint8_t byte;
    while (ring_buffer_get(&uart_rx_buffer, &byte)) {
        handle_byte(byte);
    }
}
```

### Wrap-Around Handling

When the write pointer reaches the end of the physical buffer, it wraps automatically to the beginning — no user intervention, no data copying, no surprises.

---

## 🏗️ Architecture

### Internal Structure

![ring buffer structure](.rb-structure.png)

### Visual Representation

![ring buffer layout](.rb-layout.png)

---

## 🔧 Building & Testing

### Requirements

- `gcc` or compatible C compiler
- `make`
- Standard C library

### Build Targets

```bash
make          # build tests and example
make test     # run unit tests
make example  # run usage example
make static   # build static library (libringbuffer.a)
make debug    # build with debug symbols
make clean    # remove build artifacts and .gdbhistory
```

### Running Tests

```bash
$ make test
./build/test_ring_buffer

>> Running ring buffer driver tests

>> Test: basic init
   PASS: init should succeed
   PASS: should be empty after init
   ...

>> Yaay! All tests passed!
```

---

## 📊 Memory Footprint

| Metric | Value |
|---|---|
| Code size | ~1–2 KB *(depends on functions used)* |
| RAM overhead | **24 bytes** per instance *(32-bit systems)* |
| Data buffer | User-provided — any size |
| Heap usage | **Zero** — no `malloc` / `free` |

---
