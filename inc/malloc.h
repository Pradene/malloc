#ifndef MALLOC_H
#define MALLOC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE sysconf(_SC_PAGESIZE)
#define ALIGN(size, alignment) (((size) + (alignment - 1)) & ~(alignment - 1))

// Minimum block size (header + minimum user data)
#define MIN_BLOCK_SIZE (sizeof(Block) + 1)

typedef struct Page {
  void *start;          // pointer returned by mmap()
  size_t size;          // total page size
  struct Block *blocks; // linked list of blocks inside this page
  struct Page *next;
  struct Page *prev;
} Page;

typedef struct Block {
  size_t size;  // size of the data portion
  int free;     // 1 = free, 0 = in use
  int is_large; // 1 = allocated directly via mmap
  struct Block *next;
} Block;

void *ft_malloc(size_t size);
void *ft_realloc();
void ft_free();

#endif
