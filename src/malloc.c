#include "malloc.h"

// Global pointer to store linked list of zones
Zone *base = NULL;

// Global mutex to protect all malloc operations
static pthread_mutex_t malloc_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline void lock_malloc() { pthread_mutex_lock(&malloc_mutex); }
static inline void unlock_malloc() { pthread_mutex_unlock(&malloc_mutex); }

static inline size_t align(size_t value, size_t alignment) {
  return ((value) + (alignment - 1)) & ~(alignment - 1);
}

static size_t get_alloc_blocks_size() {
  size_t size = 0;
  Zone *zone = base;
  while (zone != NULL) {
    Block *block = zone->blocks;
    while (block != NULL) {
      if (block->free == false) {
        size = size + (block->size - sizeof(Block));
      }
      block = block->next;
    }
    zone = zone->next;
  }
  return (size);
}

static size_t get_alloc_zones_size() {
  size_t size = 0;
  Zone *zone = base;
  while (zone != NULL) {
    size = size + zone->size;
    zone = zone->next;
  }
  return (size);
}

static bool can_alloc(size_t size) {
  size_t alloc_size = get_alloc_zones_size();
  struct rlimit limits;
  if (getrlimit(RLIMIT_AS, &limits) != 0) {
    return (false);
  } else if (limits.rlim_cur == RLIM_INFINITY) {
    return (true);
  } else if (alloc_size + size > limits.rlim_cur) {
    return (false);
  } else {
    return (true);
  }
}

static bool is_zone_free(Zone *zone) {
  if (zone == NULL) {
    return (false);
  }
  Block *b = zone->blocks;
  while (b != NULL) {
    if (b->free == false) {
      return (false);
    }
    b = b->next;
  }
  return (true);
}

static size_t get_os_page_size() {
#if defined(__APPLE__) || defined(__MACH__)
  return (getpagesize());
#elif defined(unix) || defined(__unix) || defined(__unix__)
  return (sysconf(_SC_PAGESIZE));
#else
  return (4096);
#endif
}

static Block *get_block_from_ptr(void *ptr) {
  Zone *z = base;
  while (z != NULL) {
    Block *b = z->blocks;
    while (b != NULL) {
      void *start = (char *)b + sizeof(Block);
      void *end = (char *)b + b->size;
      if (start <= ptr && ptr < end) {
        return (b);
      }
      b = b->next;
    }
    z = z->next;
  }
  return (NULL);
}

static Zone *get_zone_from_block(Block *block) {
  Zone *z = base;
  while (z != NULL) {
    Block *b = z->blocks;
    while (b != NULL) {
      if (b == block) {
        return (z);
      }
      b = b->next;
    }
    z = z->next;
  }
  return (NULL);
}

static ZoneType get_zone_type(size_t size) {
  if (size <= TINY_BLOCK_MAX_SIZE) {
    return (TINY);
  } else if (size <= SMALL_BLOCK_MAX_SIZE) {
    return (SMALL);
  } else {
    return (LARGE);
  }
}

// Get the string representation of zone type
static const char *get_zone_type_str(ZoneType type) {
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

// Create a new zone with initial free block
static Zone *get_zone(ZoneType type, size_t size) {
  // Calculate total size needed
  switch (type) {
  case TINY:
    size = TINY_ZONE_SIZE;
    break;
  case SMALL:
    size = SMALL_ZONE_SIZE;
    break;
  default:
    break;
  }

  // Round up to page size
  size_t zone_size = align(size + sizeof(Zone), get_os_page_size());

  if (can_alloc(zone_size) == false) {
    return (NULL);
  }

  // Allocate the page using mmap
  int prot = PROT_READ | PROT_WRITE;
  int flags = MAP_PRIVATE | MAP_ANONYMOUS;
  void *memory = mmap(NULL, zone_size, prot, flags, -1, 0);
  if (memory == MAP_FAILED) {
    return (NULL);
  }

  // Place Zone struct at the beginning
  Zone *zone = (Zone *)memory;
  zone->size = zone_size;
  zone->type = type;
  zone->prev = NULL;
  zone->next = base;
  if (base != NULL) {
    base->prev = zone;
  }
  base = zone;

  Block *block = (Block *)((char *)memory + sizeof(Zone));
  block->size = zone_size - sizeof(Zone);
  block->free = true;
  block->prev = NULL;
  block->next = NULL;

  zone->blocks = block;

  return (zone);
}

static void fragment_block(Block *block, size_t size) {
  if (block == NULL) {
    return;
  }

  size_t aligned_size = align(size, ALIGNMENT);
  size_t remaining_size = block->size - aligned_size;

  // Need enough space for new Block header + minimum alignment
  if (remaining_size >= sizeof(Block) + ALIGNMENT) {
    // Create new block in the remaining space
    Block *new_block = (Block *)((char *)block + aligned_size);

    // Initialize new block
    new_block->size = remaining_size;
    new_block->free = true;
    new_block->next = block->next;
    new_block->prev = block;

    // Update next block's prev pointer if it exists
    if (block->next != NULL) {
      block->next->prev = new_block;
    }

    // Update current block
    block->size = size;
    block->free = false;
    block->next = new_block;
  } else {
    // Not enough space to split, use entire block
    block->free = false;
  }
}

// Coalesce adjacent free blocks for defragmentation
static void coalesce_block(Block *block) {
  if (block == NULL || !block->free) {
    return;
  }

  // Coalesce with next block if it's free and adjacent
  while (block->next != NULL && block->next->free) {
    Block *next_block = block->next;
    // Check if blocks are adjacent in memory
    void *block_end = (char *)block + block->size;
    if ((char *)next_block - (char *)block_end < ALIGNMENT) {
      // Merge blocks
      void *next_block_end = (char *)next_block + next_block->size;
      block->size = (char *)next_block_end - (char *)block;

      block->next = next_block->next;
      if (next_block->next != NULL) {
        next_block->next->prev = block; // Continue coalescing block
      }
    } else {
      // Blocks are not adjacent, stop coalescing
      break;
    }
  }

  // Coalesce with previous block if it's free and adjacent
  while (block->prev != NULL && block->prev->free) {
    Block *prev_block = block->prev;
    void *prev_end = (char *)prev_block + prev_block->size;
    if ((char *)block - (char *)prev_end < ALIGNMENT) {
      void *block_end = (char *)block + block->size;
      prev_block->size = (char *)block_end - (char *)prev_block;
      prev_block->next = block->next;
      if (block->next != NULL) {
        block->next->prev = prev_block;
      }
      block = prev_block; // Continue coalescing block
    } else {
      break;
    }
  }
}

static Block *get_free_block_in_zone_type(ZoneType type, size_t size) {
  Zone *zone = base;
  while (zone != NULL) {
    if (zone->type == type) {
      Block *block = zone->blocks;
      while (block != NULL) {
        if (block->free && block->size >= size) {
          return (block);
        }
        block = block->next;
      }
    }
    zone = zone->next;
  }
  return (NULL);
}

static void print_hex_dump(void *ptr, size_t size) {
  unsigned char *data = (unsigned char *)ptr;
  size_t bpl = 16;

  for (size_t i = 0; i < size; i += bpl) {
    // Print address
    printf("%p: ", (void *)((char *)ptr + i));

    // Print hex bytes
    for (size_t j = 0; j < bpl && (i + j) < size; j++) {
      printf("%02x ", data[i + j]);
    }

    // Pad with spaces if last line is incomplete
    for (size_t j = size - i; j < bpl && i + bpl >= size; j++) {
      printf("   ");
    }

    // Print ASCII representation
    printf("|");
    for (size_t j = 0; j < bpl && (i + j) < size; j++) {
      unsigned char c = data[i + j];
      printf("%c", (c >= 32 && c <= 126) ? c : '.');
    }
    printf("|");
    printf("\n");
  }
}

static void show_alloc_zone(Zone *zone) {
  if (zone == NULL) {
    return;
  }

  printf("%s : %p\n", get_zone_type_str(zone->type),
         (char *)zone + sizeof(Zone));
  Block *block = zone->blocks;
  while (block != NULL) {
    if (block->free == true) {
      goto continuing;
    }
    size_t size = block->size - sizeof(Block);
    void *start = (char *)block + sizeof(Block);
    void *end = start + size;
    printf("%p -> %p : %zu bytes\n", start, end, size);
    print_hex_dump(start, size);
  continuing:
    block = block->next;
  }
}

void show_alloc_mem() {
  lock_malloc();
  Zone *zone = base;
  while (zone != NULL) {
    show_alloc_zone(zone);
    zone = zone->next;
  }
  printf("Total : %zu bytes\n", get_alloc_blocks_size());
  unlock_malloc();
}

void *ft_malloc(size_t size) {
  if (size == 0) {
    return (NULL);
  }

  lock_malloc();

  size_t total_size = size + sizeof(Block);

  // Try to find existing free block
  ZoneType type = get_zone_type(total_size);
  Block *block = get_free_block_in_zone_type(type, total_size);
  if (block != NULL) {
    fragment_block(block, total_size);
    void *result = (void *)((char *)block + sizeof(Block));
    unlock_malloc();
    return result;
  }

  // Create new zone
  Zone *zone = get_zone(type, total_size);
  if (zone == NULL) {
    unlock_malloc();
    return (NULL);
  }

  // Use first block in new zone
  block = zone->blocks;
  if (block->free == true && block->size >= total_size) {
    fragment_block(block, total_size);
    void *result = (void *)((char *)block + sizeof(Block));
    unlock_malloc();
    return result;
  }

  unlock_malloc();
  return (NULL);
}

void ft_free(void *ptr) {
  if (ptr == NULL) {
    return;
  }

  lock_malloc();

  Block *block = get_block_from_ptr(ptr);
  if (block == NULL) {
    unlock_malloc();
    return; // Invalid pointer
  }

  Zone *zone = get_zone_from_block(block);
  if (zone == NULL) {
    unlock_malloc();
    return;
  }

  // Mark block as free
  block->free = true;
  coalesce_block(block);

  // Check if entire zone is free
  if (is_zone_free(zone) == true) {
    // Remove zone from the doubly linked list
    if (zone->prev != NULL) {
      zone->prev->next = zone->next;
    } else {
      // This zone is the base (first zone) - update base
      base = zone->next;
    }

    if (zone->next != NULL) {
      zone->next->prev = zone->prev;
    }

    // Unmap the zone
    munmap(zone, zone->size);
  }

  unlock_malloc();
}

void *ft_realloc(void *ptr, size_t size) {
  // If ptr is NULL, behave like malloc
  if (ptr == NULL) {
    return (ft_malloc(size));
  }

  // If size is 0, behave like free
  if (size == 0) {
    ft_free(ptr);
    return (NULL);
  }

  lock_malloc();

  Block *block = get_block_from_ptr(ptr);
  if (block == NULL) {
    unlock_malloc();
    return (NULL);
  }

  size_t new_total_size = align(size + sizeof(Block), ALIGNMENT);
  size_t current_user_size = block->size - sizeof(Block);

  // Determine zone types
  ZoneType new_type = get_zone_type(new_total_size);
  ZoneType current_type = get_zone_type(block->size);

  // If same zone type and current block is large enough
  if (new_type == current_type && block->size >= new_total_size) {
    // Try to fragment if significantly larger
    if (block->size - new_total_size >= sizeof(Block) + ALIGNMENT) {
      fragment_block(block, new_total_size);
    }
    unlock_malloc();
    return (ptr);
  }

  unlock_malloc();

  // Need new allocation - unlock first to avoid deadlock
  void *new_ptr = ft_malloc(size);
  if (new_ptr == NULL) {
    return (NULL);
  }

  // Copy minimum of old and new user data sizes
  size_t copy_size = (current_user_size < size) ? current_user_size : size;
  memcpy(new_ptr, ptr, copy_size);

  ft_free(ptr);
  return (new_ptr);
}
