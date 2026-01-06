#ifndef GIGA_ARENA_H
#define GIGA_ARENA_H

#include <stddef.h>

/* C89: unsigned char is the only guaranteed byte type */
typedef struct Arena {
    unsigned char *base;    /* usable memory start */
    unsigned char *cursor;  /* next allocation */
    unsigned char *commit;  /* committed physical limit */
    unsigned char *limit;   /* end of reservation */

    size_t reserve_size;    /* usable bytes */
    size_t commit_step;     /* commit granularity */
} Arena;

int   arena_init(Arena *a, size_t reserve_size, size_t commit_step);
void  arena_destroy(Arena *a);
void  arena_reset(Arena *a);
void *arena_alloc(Arena *a, size_t size);

#endif /* GIGA_ARENA_H */
