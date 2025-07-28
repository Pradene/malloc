#include "malloc.h"

Page *base = NULL;

Block *block_create(void *ptr, size_t size) {
  Block *block = (Block *)ptr;
  block->size = size;
  block->free = 1;
  block->next = NULL;
  return (block);
}

// Create a new page with initial free block
Page *page_get(size_t size) {
  // Calculate total size needed
  size_t total_size = sizeof(Page) + sizeof(Block) + size;

  // Round up to page size
  size_t page_size = ALIGN(total_size, sysconf(_SC_PAGESIZE));

  // Allocate the page using mmap
  int prot = PROT_READ | PROT_WRITE;
  int flags = MAP_PRIVATE | MAP_ANONYMOUS;
  void *memory = mmap(NULL, page_size, prot, flags, -1, 0);
  if (memory == MAP_FAILED) {
    return (NULL);
  }

  // Place Page struct at the beginning
  Page *page = (Page *)memory;
  page->start = memory + sizeof(Page);
  page->size = page_size;
  page->next = NULL;
  page->blocks = block_create(page->start, page_size - ALIGN(sizeof(Page), 8));

  printf("%p: %zu bytes memory page allocated\n", page, page_size);

  return (page);
}

void block_split(Block *block, size_t size) {
  size_t remaining = block->size - size;
  if (remaining >= sizeof(Block) + 1) {
    // Create new block in the remaining space
    Block *new_block = (Block *)((char *)block + size);

    // Initialize new block
    new_block->size = remaining;
    new_block->free = 1;
    new_block->next = block->next;

    // Update current block
    block->size = size;
    block->free = 0;
    block->next = new_block;

    printf("%p -> %p\n", block, new_block);
  } else {
    // Not enough space to split, use entire block
    block->free = 0;
    printf("Used entire block: %p (%zu)\n", block, block->size);
  }
}

Block *block_find(size_t size) {
  Page *page = base;
  while (page != NULL) {
    Block *block = page->blocks;
    while (block != NULL) {
      if (block->free && block->size >= size) {
        printf("Found free block: %p (%zu)\n", block, block->size);
        return (block);
      }
      block = block->next;
    }
    page = page->next;
  }
  return (NULL);
}

void *ft_malloc(size_t size) {
  if (size == 0) {
    return (NULL);
  }

  printf("Requested size: %zu\n", size);

  // Align size
  size = size + sizeof(Block);
  size = ALIGN(size, 8);

  // Try to find existing free block
  Block *block = block_find(size);
  if (block != NULL) {
    block_split(block, size);
    return (void *)((char *)block + sizeof(Block));
  }

  printf("No free blocks found. Creating new page...\n");

  // Create new page
  Page *page = page_get(size);
  if (page == NULL) {
    return (NULL);
  }

  // Add to global page list
  page->next = base;
  base = page;

  // Use first block in new page
  block = page->blocks;
  if (block->free && block->size >= size) {
    block_split(block, size);
    return (void *)((char *)block + sizeof(Block));
  }

  return (NULL);
}

void ft_free(void *ptr) {
  Page *page = base;
  while (page) {
    Block *block = page->blocks;
    while (block) {
      if ((char *)block == (char *)(ptr - sizeof(Block))) {
        printf("%p: Freed\n", block);
        block->free = 1;
      }
      block = block->next;
    }
    page = page->next;
  }
}

void page_print(Page *page) {
  if (page == NULL) {
    return;
  }

  Block *block = page->blocks;
  printf("%p\n", page);
  while (block != NULL) {
    // if (block->free == 1) {
    //   block = block->next;
    //   continue;
    // }

    size_t size = block->size;
    void *start = block;
    void *end = start + size;
    printf("%p -> %p : %zu bytes\n", start, end, size);
    block = block->next;
  }
}

void memory_print() {
  if (base == NULL) {
    return;
  }

  Page *page = base;
  while (page != NULL) {
    page_print(base);
    page = page->next;
  }
}

int main() {
  printf("Size of Block: %zu\n", sizeof(Block));
  printf("Size of Page: %zu\n", sizeof(Page));
  printf("\n");

  char *b1 = (char *)ft_malloc(2048);
  char *b2 = (char *)ft_malloc(2048);
  char *b3 = (char *)ft_malloc(1024);

  memory_print();

  ft_free(b1);
  ft_free(b2);
  ft_free(b3);

  Page *page = NULL;
  while (base) {
    page = base->next;
    munmap(base, base->size);
    base = page;
  }
  return (0);
}
