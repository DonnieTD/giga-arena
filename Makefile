# ============================================================
# Giga Arena — static library + benchmark (C89)
# ============================================================

CC      ?= cc
AR      ?= ar
ARFLAGS := rcs

# ------------------------------------------------------------
# Flags
# ------------------------------------------------------------

CFLAGS_RELEASE := -std=c89 -O2 -Wall -Wextra -Wpedantic
CFLAGS_DEBUG   := -std=c89 -O0 -g  -Wall -Wextra -Wpedantic

INCLUDES := -Iinclude

# ------------------------------------------------------------
# Layout
# ------------------------------------------------------------

SRC_DIR   := .
BUILD_DIR := build
DIST_DIR  := dist

LIB       := $(DIST_DIR)/libgiga-arena.a
BENCH_BIN := arena_bench

SRC       := main.c
OBJ       := $(BUILD_DIR)/arena.o


# ------------------------------------------------------------
# Default
# ------------------------------------------------------------

.PHONY: all
all: lib bench

# ------------------------------------------------------------
# Static library build
# ------------------------------------------------------------

.PHONY: lib
lib: $(LIB)

$(LIB): $(OBJ)
	@mkdir -p $(DIST_DIR)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DIR)/arena.o: $(SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS_RELEASE) $(INCLUDES) \
	      -DGIGA_ARENA_NO_MAIN \
	      -c $< -o $@

# ------------------------------------------------------------
# Benchmark build (keeps main)
# ------------------------------------------------------------

.PHONY: bench
bench:
	$(CC) $(CFLAGS_RELEASE) $(INCLUDES) $(SRC) -o $(BENCH_BIN)

# ------------------------------------------------------------
# Debug benchmark
# ------------------------------------------------------------

.PHONY: debug
debug:
	$(CC) $(CFLAGS_DEBUG) $(INCLUDES) $(SRC) -o $(BENCH_BIN)

# ------------------------------------------------------------
# Utilities
# ------------------------------------------------------------

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR) $(BENCH_BIN)

.PHONY: run
run: bench
	./$(BENCH_BIN)
