#ifndef MALLOC_H
#define MALLOC_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

#define DEBUG 0

#define ALIGN(size, alignment) (((size) + (alignment - 1)) & ~(alignment - 1))

#define TINY_MAX 128
#define SMALL_MAX 4096

#define TINY_SIZE (TINY_MAX + sizeof(Block)) * 128
#define SMALL_SIZE (SMALL_MAX + sizeof(Block)) * 128

typedef enum { TINY, SMALL, LARGE } PageType;

typedef struct Page {
  void *start; // pointer returned by mmap()
  size_t size; // total page size
  PageType type;
  struct Block *blocks; // linked list of blocks inside this page
  struct Page *next;
} Page;

Page *base = NULL;

typedef struct Block {
  size_t size;
  bool free;
  struct Block *next;
} Block;

void *ft_malloc(size_t size);
void *ft_realloc(void *ptr, size_t size);
void ft_free(void *ptr);
void show_alloc_mem();

#endif
