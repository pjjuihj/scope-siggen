# Test Makefile for host-side unit testing
# Usage: make test

CC = /d/Dev-Cpp/TDM-GCC-64/bin/gcc.exe
CFLAGS = -I stubs -I ../Core/Inc -DVERSION_STRING='"1.4.0"' -Wall -Wextra
BUILD_DIR = /tmp/test_build

# Source files (relative to tests/ directory)
TEST_SRCS = test_main.c test_ring_buffer.c test_version.c test_framework.c test_uart_tx.c test_config.c test_debug.c test_error_tracker.c test_key_handler.c test_uart_protocol.c test_signal_gen.c test_oscilloscope.c test_display.c test_app_init.c test_upper_computer.c test_code_reviewer.c test_unit_test.c
SRC_SRCS = ../Core/Src/ring_buffer.c ../Core/Src/version.c

# Object files
TEST_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SRCS))
SRC_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC_SRCS))

# Target
TARGET = $(BUILD_DIR)/test_runner.exe

.PHONY: test clean

test: $(TARGET)
	$(TARGET)

$(TARGET): $(TEST_OBJ) $(SRC_OBJ)
	$(CC) $^ -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
