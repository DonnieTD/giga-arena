# OS-Native Arena Allocator (C89)

This project provides a high-performance, OS-native arena allocator
implemented in strict C89, using virtual memory primitives instead of
malloc / free.

It includes a self-contained benchmark executable that compares arena
allocation throughput against traditional malloc/free.

---

## Goals

- Zero per-allocation overhead
- Deterministic memory lifetime
- No fragmentation
- No garbage collection
- No dependency on the libc allocator
- OS-backed safety using guard pages
- Single translation unit
- Educational and auditable

This allocator is designed for:

- Compilers
- Interpreters and VMs
- Static analyzers
- Game engines
- Security tooling
- Time-phased systems (AST → IR → Analysis)

---

## Design Overview

The arena uses virtual memory reservation.

Memory layout:

    [ GUARD ][ RESERVED USABLE SPACE ][ GUARD ]
            ^base                 ^limit

Key properties:

- Virtual address space is reserved once
- Physical memory is committed lazily
- Allocation is a simple pointer bump
- Free is implicit via arena_reset()
- Optional guard pages catch overruns immediately

---

## Platform Support

POSIX systems (Linux, macOS, BSD):

- mmap
- mprotect
- munmap

Windows:

- VirtualAlloc
- VirtualProtect
- VirtualFree

The allocator never calls malloc internally.

---

## File Layout

    .
    ├── main.c
    ├── Makefile
    └── ReadMe.md

Everything is intentionally kept small and readable.

---

## Building

POSIX (Linux / macOS):

    make

Debug build:

    make debug

Run benchmark:

    make run

Windows (MSVC):

    cl /O2 main.c
    arena.exe

---

## Benchmark

The benchmark performs:

- 10,000,000 allocations
- Allocation size: 64 bytes
- Compares:
  - Arena allocation
  - malloc/free

Typical results (machine dependent):

    ARENA
      alloc/sec : hundreds of millions

    MALLOC/FREE
      alloc/sec : tens of millions

The arena is faster because:

- No locks
- No metadata per allocation
- No frees
- No fragmentation
- Minimal branching

---

## API Summary

    int   arena_init(Arena *a, size_t reserve, size_t commit_step);
    void  arena_destroy(Arena *a);
    void *arena_alloc(Arena *a, size_t size);
    void  arena_reset(Arena *a);

Usage pattern:

    Arena arena;
    arena_init(&arena, 512 * 1024 * 1024, 64 * 1024);

    Foo *f = arena_alloc(&arena, sizeof(Foo));

    arena_reset(&arena);
    arena_destroy(&arena);

---

## Philosophy

This allocator embraces time-based memory ownership.

Memory does not belong to objects.
Memory belongs to phases.

This model scales better, debugs better, and performs better than
ownership-heavy allocation strategies.

---

## License

Public domain / Unlicense / MIT — your choice.

This code is intended to be studied, adapted, and embedded into
serious systems.

---

## Status

- Production-ready
- Auditable
- Portable
- Benchmark-backed
- Strict C89

# Latest Benchmark Results

```sh
make && make run

cc -std=c89 -O2 -Wall -Wextra -Wpedantic main.c -o arena_bench

./arena_bench

============================================
 OS-Native Arena Allocator Benchmark (C89)
============================================

alloc size : 64 bytes
iterations : 10000000

ARENA
  time      : 0.021 sec
  alloc/sec : 476190476

MALLOC/FREE
  time      : 0.152 sec
  alloc/sec : 65975681
```
