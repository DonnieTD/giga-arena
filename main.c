/*
============================================================
 main.c — OS-native arena allocator benchmark (C89)
============================================================

This file implements:
- A high-performance arena allocator using OS VM primitives
- A benchmark comparing arena allocation vs malloc/free
- Proper compiler-proof benchmarking (no dead-code elimination)

Everything is commented. Everything is intentional.
============================================================
*/

/* =========================================================
 * Standard headers (C89)
 * ========================================================= */

#include <stdio.h>    /* printf */
#include <stdlib.h>   /* malloc/free (benchmark only) */
#include <stddef.h>   /* size_t */
#include <stdint.h>   /* uint8_t */
#include <time.h>     /* time fallback */

/* =========================================================
 * Platform detection
 * ========================================================= */

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <sys/mman.h>   /* mmap, munmap, mprotect */
    #include <unistd.h>    /* sysconf */
#endif

/* =========================================================
 * Configuration
 * ========================================================= */

/* Enable guard pages to catch overruns */
#define ARENA_GUARD_PAGES 1

/* Allocation alignment (power of two) */
#define ARENA_ALIGNMENT 8

/* Benchmark parameters */
#define BENCH_ALLOC_SIZE 64
#define BENCH_ITERATIONS 10000000UL

/* =========================================================
 * Utility helpers
 * ========================================================= */

/* Align value x up to alignment a */
static size_t align_up(size_t x, size_t a)
{
    return (x + (a - 1)) & ~(a - 1);
}

/* Query OS page size */
static size_t os_page_size(void)
{
#if defined(_WIN32)
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return (size_t)info.dwPageSize;
#else
    return (size_t)sysconf(_SC_PAGESIZE);
#endif
}

/* =========================================================
 * Arena data structure
 * ========================================================= */

typedef struct Arena {
    uint8_t *base;        /* usable memory start */
    uint8_t *cursor;      /* next allocation */
    uint8_t *commit;      /* committed physical limit */
    uint8_t *limit;       /* end of reservation */

    size_t reserve_size;  /* usable bytes */
    size_t commit_step;   /* commit granularity */
} Arena;

/* =========================================================
 * OS memory primitives
 * ========================================================= */

#if defined(_WIN32)

/* ---------------- Windows ---------------- */

static void *os_reserve(size_t size)
{
    return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
}

static int os_commit(void *addr, size_t size)
{
    return VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE) != NULL;
}

static void os_release(void *addr)
{
    VirtualFree(addr, 0, MEM_RELEASE);
}

static void os_guard(void *addr, size_t size)
{
    DWORD old;
    VirtualProtect(addr, size, PAGE_NOACCESS, &old);
}

#else

/* ---------------- POSIX ---------------- */

static void *os_reserve(size_t size)
{
    void *p = mmap(
        NULL,
        size,
        PROT_NONE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );
    return (p == MAP_FAILED) ? NULL : p;
}

static int os_commit(void *addr, size_t size)
{
    return mprotect(addr, size, PROT_READ | PROT_WRITE) == 0;
}

static void os_release(void *addr, size_t size)
{
    munmap(addr, size);
}

static void os_guard(void *addr, size_t size)
{
    mprotect(addr, size, PROT_NONE);
}

#endif

/* =========================================================
 * Arena API
 * ========================================================= */

int arena_init(Arena *a, size_t reserve_size, size_t commit_step)
{
    size_t page = os_page_size();
    size_t guard = ARENA_GUARD_PAGES ? page : 0;

    reserve_size = align_up(reserve_size, page);
    commit_step  = align_up(commit_step, page);

    {
        size_t total = reserve_size + guard * 2;
        uint8_t *mem = (uint8_t *)os_reserve(total);
        if (!mem)
            return 0;

        if (ARENA_GUARD_PAGES) {
            os_guard(mem, guard);
            os_guard(mem + guard + reserve_size, guard);
        }

        a->base         = mem + guard;
        a->cursor       = a->base;
        a->commit       = a->base;
        a->limit        = a->base + reserve_size;
        a->reserve_size = reserve_size;
        a->commit_step  = commit_step;

        return 1;
    }
}

void arena_destroy(Arena *a)
{
#if defined(_WIN32)
    os_release(a->base - (ARENA_GUARD_PAGES ? os_page_size() : 0));
#else
    os_release(
        a->base - (ARENA_GUARD_PAGES ? os_page_size() : 0),
        a->reserve_size + (ARENA_GUARD_PAGES ? os_page_size() * 2 : 0)
    );
#endif
}

void arena_reset(Arena *a)
{
    a->cursor = a->base;
}

void *arena_alloc(Arena *a, size_t size)
{
    uint8_t *next;

    size = align_up(size, ARENA_ALIGNMENT);
    next = a->cursor + size;

    if (next > a->limit)
        return NULL;

    if (next > a->commit) {
        size_t need = align_up(
            (size_t)(next - a->commit),
            a->commit_step
        );

        if (!os_commit(a->commit, need))
            return NULL;

        a->commit += need;
    }

    {
        void *result = a->cursor;
        a->cursor = next;
        return result;
    }
}

/* =========================================================
 * Timing utilities
 * ========================================================= */

static double now_seconds(void)
{
#if defined(_WIN32)
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
#else
    struct timespec ts;
#if defined(CLOCK_MONOTONIC)
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
#else
    return (double)time(NULL);
#endif
#endif
}

/* =========================================================
 * Volatile sinks (prevent optimizer cheating)
 * ========================================================= */

/*
 These ensure allocations have observable side effects.
 Without them, -O2 can remove entire loops from the benchmark;
*/
static volatile void *arena_sink;
static volatile void *malloc_sink;

/* =========================================================
 * Benchmarks
 * ========================================================= */

static void bench_arena(void)
{
    Arena a;
    size_t i;
    double t0, t1;

    if (!arena_init(&a, 1024UL * 1024 * 1024, 64UL * 1024)) {
        printf("arena_init failed\n");
        return;
    }

    t0 = now_seconds();

    for (i = 0; i < BENCH_ITERATIONS; ++i) {
        arena_sink = arena_alloc(&a, BENCH_ALLOC_SIZE);
        if (!arena_sink) {
            printf("arena_alloc failed at %lu\n",
                   (unsigned long)i);
            break;
        }
    }

    t1 = now_seconds();

    printf("ARENA\n");
    printf("  time      : %.3f sec\n", t1 - t0);
    printf("  alloc/sec : %.0f\n",
           BENCH_ITERATIONS / (t1 - t0));

    arena_reset(&a);   /* demonstrate API usage */
    arena_destroy(&a);
}

static void bench_malloc(void)
{
    size_t i;
    double t0, t1;

    t0 = now_seconds();

    for (i = 0; i < BENCH_ITERATIONS; ++i) {
        void *p = malloc(BENCH_ALLOC_SIZE);
        if (!p)
            break;

        malloc_sink = p; /* force observable write */
        free(p);
    }

    t1 = now_seconds();

    printf("MALLOC/FREE\n");
    printf("  time      : %.3f sec\n", t1 - t0);
    printf("  alloc/sec : %.0f\n",
           BENCH_ITERATIONS / (t1 - t0));
}

/* =========================================================
 * main
 * ========================================================= */
#ifndef GIGA_ARENA_NO_MAIN
int main(void)
{
    printf("============================================\n");
    printf(" OS-Native Arena Allocator Benchmark (C89)\n");
    printf("============================================\n");
    printf("alloc size : %d bytes\n", BENCH_ALLOC_SIZE);
    printf("iterations : %lu\n\n",
           (unsigned long)BENCH_ITERATIONS);

    bench_arena();
    printf("\n");
    bench_malloc();

    return 0;
}
#endif
