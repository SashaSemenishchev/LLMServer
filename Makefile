# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -O3
LDFLAGS :=

# Check if DEBUG is set to 1, and add -g flag accordingly
ifeq ($(DEBUG), 1)
	CFLAGS += -g -O0
	LDFLAGS += -g -O0
else
	CFLAGS += -O3
endif

# Directories
SRC_DIR := src
BUILD_DIR := build
OBJ_DIRS := $(shell find $(SRC_DIR) -type d | sed 's/$(SRC_DIR)/$(BUILD_DIR)/g')

# Source files and object files
SRCS := $(shell find $(SRC_DIR) -type f -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Targets
TARGET := main

# Default target
all: $(OBJ_DIRS) $(TARGET)

# Create the object directories if they don't exist
$(OBJ_DIRS):
	mkdir -p $@

# Compile source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files into the final executable
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

# Clean up generated files
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean
