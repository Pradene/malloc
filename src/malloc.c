#include "malloc.h"

Zone *base = NULL;

static inline size_t align(size_t value, size_t alignment) {
  return ((value) + (alignment - 1)) & ~(alignment - 1);
}

// static size_t get_alloc_blocks_size() {
//   size_t size = 0;
//   Zone *zone = base;
//   while (zone != NULL) {
//     Block *block = zone->blocks;
//     while (block != NULL) {
//       if (block->free == false) {
//         size = size + block->size;
//       }
//       block = block->next;
//     }
//     zone = zone->next;
//   }
//   return (size);
// }

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

// static bool is_valid_ptr(void *ptr) {
//   return (get_block_from_ptr(ptr) != NULL);
// }

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
  size += sizeof(Zone);
  size_t zone_size = align(size, get_os_page_size());

  if (can_alloc(size) == false) {
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

  Block *block = (Block *)(memory + sizeof(Zone));
  block->size = zone_size - sizeof(Zone);
  block->free = true;
  block->prev = NULL;
  block->next = NULL;

  zone->blocks = block;

  return (zone);
}

static void alloc_block(Block *block, size_t size) {
  if (block == NULL) {
    return;
  }

  size_t aligned_size = align(size, ALIGNMENT);
  size_t remaining_size = block->size - aligned_size;

  if (remaining_size > sizeof(Block)) {
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
    block->size = aligned_size;
    block->free = false;
    block->next = new_block;
  } else {
    // Not enough space to split, use entire block
    block->free = false;
  }
}

// Coalesce adjacent free blocks for defragmentation
static void coalesce_blocks(Block *block) {
  if (block == NULL || !block->free) {
    return;
  }

  // Coalesce with next block if it's free and adjacent
  while (block->next != NULL && block->next->free) {
    Block *next_block = block->next;

    // Check if blocks are actually adjacent in memory
    void *block_end = (char *)block + block->size;
    if (block_end == (void *)next_block) {
      // Merge blocks
      block->size += next_block->size;
      block->next = next_block->next;

      // Update prev pointer of the block after next_block
      if (next_block->next != NULL) {
        next_block->next->prev = block;
      }
    } else {
      break; // Blocks aren't adjacent
    }
  }

  // Coalesce with previous block if it's free and adjacent
  while (block->prev != NULL && block->prev->free) {
    Block *prev_block = block->prev;

    // Check if blocks are actually adjacent in memory
    void *prev_end = (char *)prev_block + prev_block->size;
    if (prev_end == (void *)block) {
      // Merge blocks
      prev_block->size += block->size;
      prev_block->next = block->next;

      // Update prev pointer of the block after current block
      if (block->next != NULL) {
        block->next->prev = prev_block;
      }

      block = prev_block; // Continue from the merged block
    } else {
      break; // Blocks aren't adjacent
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

static void show_alloc_zone(Zone *zone) {
  if (zone == NULL) {
    return;
  }

  printf("%s : %p\n", get_zone_type_str(zone->type), zone);
  Block *block = zone->blocks;
  while (block != NULL) {
    size_t size = block->size;
    void *start = block;
    void *end = start + size;
    printf("%p -> %p : %zu bytes (%s)\n", start, end, size,
           block->free ? "FREE" : "ALLOCATED");
    block = block->next;
  }
}

void show_alloc_mem() {
  Zone *zone = base;
  while (zone != NULL) {
    show_alloc_zone(zone);
    zone = zone->next;
  }
}

void *ft_malloc(size_t size) {
  if (size == 0) {
    return (NULL);
  }

  size = size + sizeof(Block);

  // Try to find existing free block
  ZoneType type = get_zone_type(size);
  Block *block = get_free_block_in_zone_type(type, size);
  if (block != NULL) {
    alloc_block(block, size);
    return (void *)((char *)block + sizeof(Block));
  }

  // Create new zone
  Zone *zone = get_zone(type, size);
  if (zone == NULL) {
    return (NULL);
  }

  // Use first block in new zone
  block = zone->blocks;
  if (block->free == true && block->size >= size) {
    alloc_block(block, size);
    return (void *)((char *)block + sizeof(Block));
  }

  return (NULL);
}

void ft_free(void *ptr) {
  if (ptr == NULL) {
    return;
  }

  Block *block = get_block_from_ptr(ptr);
  if (block == NULL) {
    return; // Invalid pointer
  }

  Zone *zone = get_zone_from_block(block);
  if (zone == NULL) {
    return;
  }

  // Mark block as free
  block->free = true;

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

  Block *block;
  if ((block = get_block_from_ptr(ptr)) == NULL) {
    return (NULL);
  }

  size_t new_size = size + sizeof(Block);

  // Determine what zone type the new size needs
  ZoneType new_type = get_zone_type(new_size);

  // Determine what zone type the current block is in
  ZoneType current_type = get_zone_type(block->size);

  // If the new size fits in the current block and same zone type, try to resize
  if (new_type == current_type && block->size >= new_size) {
    // Can potentially split the block if it's much larger
    alloc_block(block, new_size);
    return (ptr);
  }

  // Need to allocate new memory (different zone type or not enough space)
  void *new_ptr = ft_malloc(size);
  if (new_ptr == NULL) {
    return (NULL);
  }

  // Copy the data (copy the minimum of old size and new size)
  size_t copy_size =
      (block->size - sizeof(Block) < size) ? block->size - sizeof(Block) : size;
  memcpy(new_ptr, ptr, copy_size);

  // Free the old block
  ft_free(ptr);

  return (new_ptr);
}
