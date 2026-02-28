CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -O2 -Iinclude
DEBUG_CFLAGS = -g -O0 -DDEBUG

SRC_DIR = src
INC_DIR = include
TEST_DIR = tests
EXAMPLE_DIR = examples
BUILD_DIR = build

SOURCES = $(SRC_DIR)/ring_buffer.c
OBJECTS = $(BUILD_DIR)/ring_buffer.o

TEST_SOURCES = $(TEST_DIR)/test_ring_buffer.c
EXAMPLE_SOURCES = $(EXAMPLE_DIR)/example_usage.c

# targets
.PHONY: all clean test example debug

all: $(BUILD_DIR) $(BUILD_DIR)/test_ring_buffer $(BUILD_DIR)/example_usage

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/ring_buffer.o: $(SRC_DIR)/ring_buffer.c $(INC_DIR)/ring_buffer.h
	$(CC) $(CFLAGS) -c $< -o $@

# test executable
$(BUILD_DIR)/test_ring_buffer: $(TEST_SOURCES) $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

# example executable
$(BUILD_DIR)/example_usage: $(EXAMPLE_SOURCES) $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

debug: CFLAGS += $(DEBUG_CFLAGS)
debug: all

test: $(BUILD_DIR)/test_ring_buffer
	./$(BUILD_DIR)/test_ring_buffer

example: $(BUILD_DIR)/example_usage
	./$(BUILD_DIR)/example_usage

clean:
	rm -rf $(BUILD_DIR) .gdb_history

# static library
$(BUILD_DIR)/libringbuffer.a: $(OBJECTS)
	ar rcs $@ $^

static: $(BUILD_DIR) $(BUILD_DIR)/libringbuffer.a