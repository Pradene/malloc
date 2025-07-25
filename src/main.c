#include "malloc.h"

Page *base = NULL;

Block *block_create(void *ptr, size_t size) {
  Block *block = (Block *)ptr;
  block->size = size - sizeof(Block);
  block->free = 1;
  block->is_large = 0;
  block->next = NULL;
  return (block);
}

// Create a new page with initial free block
Page *page_get(size_t size) {
  // Calculate total size needed
  size_t total_size = sizeof(Page) + sizeof(Block) + size;

  // Round up to page size
  size_t page_size = ALIGN(total_size, PAGE_SIZE);

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
  page->prev = NULL;
  page->blocks = block_create(page->start, page_size - sizeof(Page));

  printf("%p: %zu bytes memory page allocated\n", page, page_size);

  return (page);
}

void block_split(Block *block, size_t size) {
  size_t remaining = block->size - size;
  if (remaining >= MIN_BLOCK_SIZE) {
    // Create new block in the remaining space
    Block *new_block = (Block *)((char *)block + sizeof(Block) + size);

    // Initialize new block
    new_block->size = remaining - sizeof(Block);
    new_block->free = 1;
    new_block->is_large = 0;
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

  // Align size
  size = ALIGN(size, 8);
  printf("Requested size: %zu\n", size);

  // Try to find existing free block
  Block *block = block_find(size);
  if (block != NULL) {
    block_split(block, size);
    return (void *)((char *)block + sizeof(Block));
  }

  printf("No free blocks found. Creating new page...\n");

  // Create new page
  Page *new_page = page_get(size);
  if (new_page == NULL) {
    return (NULL);
  }

  // Add to global page list
  new_page->next = base;
  if (base != NULL) {
    base->prev = new_page;
  }
  base = new_page;

  // Use first block in new page
  block = new_page->blocks;
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

int main() {
  char *b1 = (char *)ft_malloc(1024);
  char *b2 = (char *)ft_malloc(1024);

  sprintf(b1, "Hello %s\n", "World");
  sprintf(b2, "Hello %s\n", "World");

  ft_free(b1);
  ft_free(b2);

  Page *page = NULL;
  while (base) {
    page = base->next;
    munmap(base, base->size);
    base = page;
  }
  return (0);
}
