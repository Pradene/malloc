#ifndef MALLOC_H
#define MALLOC_H

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

#define ALIGNMENT 16

#define TINY_BLOCK_MAX_SIZE 256
#define SMALL_BLOCK_MAX_SIZE 4096

#define TINY_ZONE_SIZE TINY_BLOCK_MAX_SIZE * 128
#define SMALL_ZONE_SIZE SMALL_BLOCK_MAX_SIZE * 128

typedef enum { TINY, SMALL, LARGE } ZoneType;

typedef struct __attribute__((aligned(ALIGNMENT))) Block {
  bool free; // is block free
  size_t size;
  struct Block *next;
} Block;

typedef struct __attribute__((aligned(ALIGNMENT))) Zone {
  void *start; // pointer returned by mmap()
  size_t size; // total page size
  ZoneType type;
  Block *blocks; // linked list of blocks inside this page
  struct Zone *next;
} Zone;

Zone *base = NULL;

void *malloc(size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);
void show_alloc_mem();

#endif
