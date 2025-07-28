#ifndef MALLOC_H
#define MALLOC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

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

typedef struct Block {
  size_t size; // size of the data portion
  int free;    // 1 = free, 0 = in use
  struct Block *next;
} Block;

void *ft_malloc(size_t size);
void *ft_realloc();
void ft_free();

#endif
