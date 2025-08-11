#ifndef MALLOC_H
#define MALLOC_H

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

#ifndef MALLOC_ARENA_MAX
#define MALLOC_ARENA_MAX 0
#endif
#ifndef MALLOC_ARENA_TEST
#define MALLOC_ARENA_TEST 8
#endif
#ifndef MALLOC_CHECK_
#define MALLOC_CHECK_ 0
#endif
#ifndef MALLOC_MMAP_MAX_
#define MALLOC_MMAP_MAX_ 65536
#endif
#ifndef MALLOC_PERTURB_
#define MALLOC_PERTURB_ 0
#endif

#define ALIGNMENT 16

#define TINY_BLOCK_MAX_SIZE 256
#define SMALL_BLOCK_MAX_SIZE 4096

#define TINY_ZONE_SIZE TINY_BLOCK_MAX_SIZE * 512
#define SMALL_ZONE_SIZE SMALL_BLOCK_MAX_SIZE * 128

typedef enum { TINY, SMALL, LARGE } ZoneType;

typedef struct __attribute__((aligned(ALIGNMENT))) Block {
  bool free; // is block free
  size_t size;
  struct Block *prev;
  struct Block *next;
} Block;

typedef struct __attribute__((aligned(ALIGNMENT))) Zone {
  size_t size; // total page size
  ZoneType type;
  Block *blocks; // linked list of blocks inside this page
  struct Zone *prev;
  struct Zone *next;
} Zone;

void *ft_malloc(size_t size);
void *ft_realloc(void *ptr, size_t size);
void ft_free(void *ptr);
void show_alloc_mem();

#endif
