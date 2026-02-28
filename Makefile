CC := gcc
CFLAGS := -g -Wall -Werror -Wextra -pedantic -Iinclude
LDFLAGS :=
LIBS :=
INCLUDE_DIR := include
SRC_DIR := src
BUILD_DIR := build

# headers
HDRS := $(shell find $(INCLUDE_DIR) -name '*.h')

# source files
SRCS := $(shell find $(SRC_DIR) -name '*.c')

# object files
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# executable
EXEC := main

all: $(EXEC)

$(EXEC): $(OBJS) $(HDRS) Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXEC) $(OBJS)
