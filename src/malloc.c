#include "malloc.h"

size_t get_alloc_zones_size() {
  size_t size = 0;
  Zone *zone = base;
  while (zone != NULL) {
    size += zone->size;
    zone = zone->next;
  }
  return (size);
}

bool can_alloc(size_t size) {
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

size_t get_os_page_size() {
#if defined(__APPLE__) || defined(__MACH__)
  return (getpagesize());
#elif defined(unix) || defined(__unix) || defined(__unix__)
  return (sysconf(_SC_PAGESIZE));
#else
  return (4096);
#endif
}

Block *get_block_from_ptr(void *ptr) {
  Zone *zone = base;
  while (zone != NULL) {
    Block *block = zone->blocks;
    while (block != NULL) {
      void *start = (char *)block + sizeof(Block);
      void *end = (char *)block + block->size;
      if (start <= ptr && ptr < end) {
        return (block);
      }
      block = block->next;
    }
    zone = zone->next;
  }
  return (NULL);
}

bool is_valid_ptr(void *ptr) { return (get_block_from_ptr(ptr) != NULL); }

ZoneType get_zone_type(size_t size) {
  if (size <= TINY_BLOCK_MAX_SIZE) {
    return (TINY);
  } else if (size <= SMALL_BLOCK_MAX_SIZE) {
    return (SMALL);
  } else {
    return (LARGE);
  }
}

// Get the string representation of zone type
const char *get_zone_type_str(ZoneType type) {
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
Zone *get_zone(ZoneType type, size_t size) {
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
  size_t zone_size = ALIGN(size, get_os_page_size());

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
  zone->start = memory + sizeof(Zone);
  zone->size = zone_size;
  zone->next = NULL;
  zone->type = type;

  Block *block = zone->start;
  block->size = zone_size - sizeof(Zone);
  block->free = true;
  block->next = NULL;

  zone->blocks = block;

  return (zone);
}

void alloc_block(Block *block, size_t size) {
  size_t aligned_size = ALIGN(size, ALIGNMENT);
  size_t remaining_size = block->size - aligned_size;
  if (remaining_size > sizeof(Block)) {
    // Create new block in the remaining space
    Block *new_block = (Block *)((char *)block + aligned_size);

    // Initialize new block
    new_block->size = remaining_size;
    new_block->free = true;
    new_block->next = block->next;

    // Update current block
    block->size = size;
    block->free = false;
    block->next = new_block;
  } else {
    // Not enough space to split, use entire block
    block->size = size;
    block->free = false;
    block->next = NULL;
  }
}

Block *get_free_block_in_zone_type(ZoneType type, size_t size) {
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

void show_alloc_zone(Zone *zone) {
  if (zone == NULL) {
    return;
  }

  Block *block = zone->blocks;
  printf("%s : %p\n", get_zone_type_str(zone->type),
         (char *)zone + sizeof(Zone));
  while (block != NULL) {
    if (block->free == true) {
      break;
    }
    size_t size = block->size;
    void *start = block;
    void *end = start + size;
    printf("%p -> %p : %zu bytes\n", start + sizeof(Block), end,
           size - sizeof(Block));
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

void *malloc(size_t size) {
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

  // Add to global zone list
  zone->next = base;
  base = zone;

  // Use first block in new zone
  block = zone->blocks;
  if (block->free == true && block->size >= size) {
    alloc_block(block, size);
    return (void *)((char *)block + sizeof(Block));
  }

  return (NULL);
}

void free(void *ptr) {
  Block *block = get_block_from_ptr(ptr);
  if (block == NULL) {
    return;
  }
  block->free = true;
}

void *realloc(void *ptr, size_t size) {
  // If ptr is NULL, behave like malloc
  if (ptr == NULL) {
    return (malloc(size));
  }

  // If size is 0, behave like free
  if (size == 0) {
    free(ptr);
    return (NULL);
  }

  Block *block;
  if ((block = get_block_from_ptr(ptr)) == NULL) {
    return (NULL);
  }

  // Determine what zone type the new size needs
  ZoneType new_type = get_zone_type(size);

  // Determine what zone type the current block is in
  ZoneType current_type = get_zone_type(block->size);

  // If the new size fits in the current block and same zone type, try to resize
  if (new_type == current_type && block->size >= size) {
    // Can potentially split the block if it's much larger
    alloc_block(block, size);
    return (ptr);
  }

  // Need to allocate new memory (different zone type or not enough space)
  void *new_ptr = malloc(size);
  if (new_ptr == NULL) {
    return (NULL);
  }

  // Copy the data (copy the minimum of old size and new size)
  size_t copy_size = (block->size < size) ? block->size : size;
  memcpy(new_ptr, ptr, copy_size);

  // Free the old block
  free(ptr);

  return (new_ptr);
}
