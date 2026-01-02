# ============================================================
# Makefile — OS-native Arena Allocator Benchmark
# ============================================================

# Compiler
CC ?= cc

# Build modes
CFLAGS_RELEASE := -std=c89 -O2 -Wall -Wextra -Wpedantic
CFLAGS_DEBUG   := -std=c89 -O0 -g  -Wall -Wextra -Wpedantic

# Output binary
BIN := arena_bench

# Source files
SRC := main.c

# Default target
.PHONY: all
all: release

# ------------------------------------------------------------
# Release build (benchmarking)
# ------------------------------------------------------------
.PHONY: release
release:
	$(CC) $(CFLAGS_RELEASE) $(SRC) -o $(BIN)

# ------------------------------------------------------------
# Debug build
# ------------------------------------------------------------
.PHONY: debug
debug:
	$(CC) $(CFLAGS_DEBUG) $(SRC) -o $(BIN)

# ------------------------------------------------------------
# Clean
# ------------------------------------------------------------
.PHONY: clean
clean:
	rm -f $(BIN)

# ------------------------------------------------------------
# Run benchmark
# ------------------------------------------------------------
.PHONY: run
run: release
	./$(BIN)

