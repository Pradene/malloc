#include "malloc.h"

// Create a new page with initial free block
Page *page_get(size_t size) {
  // Calculate total size needed
  size_t total_size = sizeof(Page) + sizeof(Block) + ALIGN(size);

  // Round up to page size
  size_t page_size = ALIGN(total_size);

  // Allocate the page using mmap
  int prot = PROT_READ | PROT_WRITE;
  int flags = MAP_PRIVATE | MAP_ANONYMOUS;
  void *memory = mmap(NULL, page_size, prot, flags, -1, 0);
  if (memory == MAP_FAILED) {
    return NULL;
  }

  // Place Page struct at the beginning
  Page *page = (Page *)memory;
  page->start = memory + sizeof(Page);
  page->size = page_size;
  page->next = NULL;
  page->prev = NULL;

  // Place first Block right after the Page struct
  Block *block = (Block *)page->start;
  block->size = page_size - sizeof(Page) - sizeof(Block);
  block->free = 1;
  block->is_large = 0;
  block->next = NULL;

  page->blocks = block;

  return page;
}

int main() {
  size_t size = sysconf(_SC_PAGESIZE);
  Page *page = page_get(size);
  if (page == NULL) {
    return (1);
  }

  // Write to the allocatable area
  strcpy((char *)page->start + sizeof(Block), "Hello World!");
  printf("%s\n", (char *)page->start + sizeof(Block));

  munmap((char *)page, page->size);
  return (0);
}
