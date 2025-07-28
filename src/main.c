#include "malloc.h"
#include <sys/resource.h>

Page *base = NULL;

PageType get_page_type(size_t size) {
  if (size <= TINY_MAX) {
    return (TINY);
  } else if (size <= SMALL_MAX) {
    return (SMALL);
  } else {
    return (LARGE);
  }
}

// Get the string representation of page type
const char *get_page_type_str(PageType type) {
  switch (type) {
  case TINY:
    return ("TINY");
  case SMALL:
    return ("SMALL");
  case LARGE:
    return ("LARGE");
  default:
    return ("UNKNOWN");
  }
}

// Create a new page with initial free block
Page *get_page(PageType type, size_t size) {
  // Calculate total size needed
  switch (type) {
  case TINY:
    size = TINY_SIZE;
    break;
  case SMALL:
    size = SMALL_SIZE;
    break;
  default:
    break;
  }

  size += sizeof(Page);

  // Round up to page size
  size_t page_size = ALIGN(size, sysconf(_SC_PAGESIZE));

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
  page->type = type;

  Block *block = page->start;
  size = page_size - sizeof(Page);
  block->size = ALIGN(size, 16);
  block->free = 1;
  block->next = NULL;

  page->blocks = block;

  if (DEBUG) {
    printf("%p: %s page (%zu bytes)\n", page, get_page_type_str(page->type),
           page_size);
  }

  return (page);
}

void split_block(Block *block, size_t size) {
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

    if (DEBUG) {
      printf("%p -> %p\n", block, new_block);
    }
  } else {
    // Not enough space to split, use entire block
    block->free = 0;
    if (DEBUG) {
      printf("Used entire block: %p (%zu)\n", block, block->size);
    }
  }
}

Block *get_free_block_in_page_type(PageType type, size_t size) {
  Page *page = base;
  while (page != NULL) {
    if (page->type == type) {
      Block *block = page->blocks;
      while (block != NULL) {
        if (block->free && block->size >= size) {
          return (block);
        }
        block = block->next;
      }
    }
    page = page->next;
  }
  return (NULL);
}
void *ft_malloc(size_t size) {
  if (size == 0) {
    return (NULL);
  }

  if (DEBUG) {
    printf("%zu bytes requested\n", size);
  }

  // Align size
  size = size + sizeof(Block);
  size = ALIGN(size, 16);

  PageType type = get_page_type(size);

  // Try to find existing free block
  Block *block = get_free_block_in_page_type(type, size);
  if (block != NULL) {
    split_block(block, size);
    return (void *)((char *)block + sizeof(Block));
  }

  // Create new page
  Page *page = get_page(type, size);
  if (page == NULL) {
    return (NULL);
  }

  // Add to global page list
  page->next = base;
  base = page;

  // Use first block in new page
  block = page->blocks;
  if (block->free && block->size >= size) {
    split_block(block, size);
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
        if (DEBUG) {
          printf("%p: Freed\n", block);
        }
        block->free = 1;
      }
      block = block->next;
    }
    page = page->next;
  }
}

void print_page(Page *page) {
  if (page == NULL) {
    return;
  }

  Block *block = page->blocks;
  printf("%s : %p\n", get_page_type_str(page->type), page);
  while (block != NULL) {
    size_t size = block->size;
    void *start = block;
    void *end = start + size;
    printf("%p -> %p : %zu bytes %s\n", start, end, size,
           block->free ? "(Free)" : "");
    block = block->next;
  }
}

void print_mem() {
  Page *page = base;
  while (page != NULL) {
    print_page(page);
    page = page->next;
  }
}

int main() {
  struct rlimit limit = {0};
  getrlimit(RLIMIT_AS, &limit);
  if (DEBUG) {
    printf("Current %zu\n", limit.rlim_cur);
    printf("Max %zu\n", limit.rlim_max);
  }

  void *p1 = ft_malloc(2048);
  void *p2 = ft_malloc(2048);
  void *p3 = ft_malloc(2048);
  void *p4 = ft_malloc(2048);
  void *p5 = ft_malloc(64);
  void *p6 = ft_malloc(8192);

  print_mem();

  ft_free(p1);
  ft_free(p2);
  ft_free(p3);
  ft_free(p4);
  ft_free(p5);
  ft_free(p6);

  Page *page = NULL;
  while (base) {
    page = base->next;
    munmap(base, base->size);
    base = page;
  }
  return (0);
}
