#include "malloc.h"

// Global pointer to store linked list of zones
static Zone *base = NULL;

// Global mutex to protect all malloc operations
static pthread_mutex_t malloc_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline void lock_malloc() { pthread_mutex_lock(&malloc_mutex); }
static inline void unlock_malloc() { pthread_mutex_unlock(&malloc_mutex); }

static inline size_t align(size_t value, size_t alignment) {
  return ((value) + (alignment - 1)) & ~(alignment - 1);
}

static void *get_block_start(Block *block) {
  return ((char *)block + sizeof(Block));
}

static void *get_zone_start(Zone *zone) {
  return ((char *)zone + sizeof(Zone));
}

static size_t get_block_size(Block *block) {
  return (block->size - sizeof(Block));
}

static size_t get_alloc_blocks_size() {
  size_t size = 0;
  Zone *zone = base;
  while (zone != NULL) {
    Block *block = zone->blocks;
    while (block != NULL) {
      if (block->status == ALLOCATED) {
        size = size + get_block_size(block);
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
  if (ptr == NULL) {
    return (NULL);
  }
  Zone *z = base;
  while (z != NULL) {
    // Validate zone pointer before dereferencing
    if (z->size == 0) {
      break;
    }
    // Check if ptr is within this zone's bounds
    void *zone_start = get_zone_start(z);
    void *zone_end = (char *)z + z->size;
    if (ptr >= zone_start && ptr < zone_end) {
      Block *b = z->blocks;
      int block_count = 0;
      const int max_blocks = (z->size - sizeof(Zone)) / sizeof(Block);
      while (b != NULL && block_count < max_blocks) {
        // Validate block pointer is within zone
        if ((void *)b < zone_start || (void *)b >= zone_end) {
          break;
        }
        void *start = get_block_start(b);
        if (start == ptr) {
          return (b);
        }
        b = b->next;
        block_count++;
      }
      // If we reach here, ptr is in zone but not found (corrupted)
      return (NULL);
    }
    z = z->next;
  }
  return (NULL);
}

static Zone *get_zone_from_block(Block *block) {
  if (block == NULL) {
    return (NULL);
  }
  Zone *z = base;
  while (z != NULL) {
    // Check if block is within this zone's bounds
    void *zone_start = (char *)z + sizeof(Zone);
    void *zone_end = (char *)z + z->size;
    if ((void *)block >= zone_start && (void *)block < zone_end) {
      return (z);
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
// If type is large you must provide the size
// Else doesn't need to send size because it is calculated
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
    errno = ENOMEM;
    return (NULL);
  }

  // Allocate the page using mmap
  int prot = PROT_READ | PROT_WRITE;
  int flags = MAP_PRIVATE | MAP_ANONYMOUS;
  void *memory = mmap(NULL, zone_size, prot, flags, -1, 0);
  if (memory == MAP_FAILED) {
    errno = ENOMEM;
    return (NULL);
  }

  // Place Zone struct at the beginning
  Zone *zone = (Zone *)memory;
  zone->size = zone_size;
  zone->type = type;
  if (base == NULL || zone < base) {
    // Insert at beginning (empty list or lowest address)
    zone->prev = NULL;
    zone->next = base;
    if (base != NULL) {
      base->prev = zone;
    }
    base = zone;
  } else {
    // Find correct position in sorted list
    Zone *current = base;
    while (current->next != NULL && current->next < zone) {
      current = current->next;
    }
    // Insert zone after current
    zone->next = current->next;
    zone->prev = current;
    if (current->next != NULL) {
      current->next->prev = zone;
    }
    current->next = zone;
  }

  Block *block = (Block *)((char *)memory + sizeof(Zone));
  block->size = zone_size - sizeof(Zone);
  block->status = FREE;
  block->prev = NULL;
  block->next = NULL;

  zone->blocks = block;

  return (zone);
}

static void fragment_block(Block *block, size_t size) {
  if (block == NULL || size < sizeof(Block)) {
    return;
  }

  size_t aligned_size = align(size, ALIGNMENT);

  // Ensure we don't exceed block size
  if (aligned_size > block->size) {
    aligned_size = block->size;
  }

  size_t remaining_size = block->size - aligned_size;

  // Need enough space for new Block header + minimum alignment
  if (remaining_size >= sizeof(Block) + ALIGNMENT) {
    // Create new block in the remaining space
    Block *new_block = (Block *)((char *)block + aligned_size);

    // Initialize new block
    new_block->size = remaining_size;
    new_block->status = FREE;
    new_block->next = block->next;
    new_block->prev = block;

    // Update next block's prev pointer if it exists
    if (block->next != NULL) {
      block->next->prev = new_block;
    }

    // Update current block
    block->size = size;
    block->next = new_block;
  }

  block->status = ALLOCATED;
  if (MALLOC_PERTURB != 0) {
    ft_memset(get_block_start(block), ~(0xFF & MALLOC_PERTURB), get_block_size(block));
  }
}

static void coalesce_block(Block *block) {
  if (block == NULL || block->status == ALLOCATED) {
    return;
  }

  // Set status to FREE if it was FREED
  if (block->status == FREED) {
    block->status = FREE;
  }

  // Coalesce with next block if it's free and exactly adjacent
  while (block->next != NULL && block->next->status != ALLOCATED) {
    Block *next_block = block->next;
    void *block_end = (char *)block + block->size;
    // Check if blocks are exactly adjacent in memory
    if ((char *)next_block - (char *)block_end < ALIGNMENT) {
      // Merge blocks
      void *next_block_end = (char *)next_block + next_block->size;
      block->size = (char *)next_block_end - (char *)block;
      block->next = next_block->next;
      if (next_block->next != NULL) {
        next_block->next->prev = block;
      }
    } else {
      // Blocks are not adjacent, stop coalescing
      break;
    }
  }

  // Coalesce with previous block if it's free and exactly adjacent
  if (block->prev != NULL && block->prev->status != ALLOCATED) {
    Block *prev_block = block->prev;
    void *prev_end = (char *)prev_block + prev_block->size;

    // Check if blocks are exactly adjacent
    if ((char *)block == prev_end) {
      // Merge with previous block
      prev_block->size += block->size;
      prev_block->next = block->next;
      if (block->next != NULL) {
        block->next->prev = prev_block;
      }
    }
  }
}

static Block *get_free_block_in_zone_type(ZoneType type, size_t size) {
  Zone *zone = base;

  while (zone != NULL) {
    if (zone->type == type && zone->size > 0) {
      Block *block = zone->blocks;
      int block_count = 0;
      const int max_blocks = (zone->size - sizeof(Zone)) / sizeof(Block);

      void *zone_start = get_zone_start(zone);
      void *zone_end = (char *)zone + zone->size;

      while (block != NULL && block_count < max_blocks) {
        // Validate block is within zone bounds
        if ((void *)block < zone_start || (void *)block >= zone_end) {
          break;
        }

        if (block->status == FREE && block->size >= size) {
          return (block);
        }
        block = block->next;
        block_count++;
      }
    }
    zone = zone->next;
  }
  return (NULL);
}

static void print_hex_dump(void *ptr, size_t size) {
  unsigned char *data = (unsigned char *)ptr;
  size_t bytes_per_line = 16;

  for (size_t i = 0; i < size; i += bytes_per_line) {
    ft_printf("%p: ", (void *)((char *)ptr + i));
    for (size_t j = 0; j < bytes_per_line && (i + j) < size; j++) {
      ft_printf("%02x ", data[i + j]);
    }
    for (size_t j = size - i; j < bytes_per_line && i + bytes_per_line >= size; j++) {
      ft_printf("   ");
    }
    ft_printf("|");
    for (size_t j = 0; j < bytes_per_line && (i + j) < size; j++) {
      unsigned char c = data[i + j];
      ft_printf("%c", (c >= 32 && c <= 126) ? c : '.');
    }
    ft_printf("|");
    ft_printf("\n");
  }
}

static void show_alloc_zone(Zone *zone) {
  if (zone == NULL) {
    return;
  }

  ft_printf("%s : %p\n", get_zone_type_str(zone->type), get_zone_start(zone));
  Block *block = zone->blocks;
  int block_count = 0;
  const int max_blocks = zone->size / sizeof(Block);

  while (block != NULL && block_count < max_blocks) {
    if (block->status != ALLOCATED) {
      goto continuing;
    }
    void *start = get_block_start(block);
    size_t size = get_block_size(block);
    void *end = start + size;
    ft_printf("%p -> %p : %z bytes\n", start, end, size);
    if (MALLOC_HEXDUMP != 0) {
      print_hex_dump(start, size);
    }
  continuing:
    block = block->next;
    block_count++;
  }
}

void show_alloc_mem() {
  lock_malloc();
  Zone *zone = base;
  while (zone != NULL) {
    show_alloc_zone(zone);
    zone = zone->next;
  }
  ft_printf("Total : %z bytes\n", get_alloc_blocks_size());
  unlock_malloc();
}

void *malloc(size_t size) {
  if (size == 0) {
    return (NULL);
  }

  lock_malloc();

  size_t total_size = size + sizeof(Block);
  if (total_size < size) {
    // Overflow
    unlock_malloc();
    errno = ENOMEM;
    return (NULL);
  }

  // Try to find existing free block
  ZoneType type = get_zone_type(total_size);
  Block *block = get_free_block_in_zone_type(type, total_size);
  if (block != NULL) {
    fragment_block(block, total_size);
    void *result = get_block_start(block);
    unlock_malloc();
    return result;
  }

  // Create new zone
  Zone *zone = get_zone(type, total_size);
  if (zone == NULL) {
    errno = ENOMEM;
    unlock_malloc();
    return (NULL);
  }

  // Use first block in new zone
  block = zone->blocks;
  if (block->status == FREE && block->size >= total_size) {
    fragment_block(block, total_size);
    void *result = get_block_start(block);
    unlock_malloc();
    return result;
  }

  errno = ENOMEM;
  unlock_malloc();
  return (NULL);
}

void free(void *ptr) {
  if (ptr == NULL) {
    return;
  }

  lock_malloc();

  Block *block = get_block_from_ptr(ptr);
  if (block == NULL) {
    unlock_malloc();
    if ((MALLOC_CHECK >> 2) & 1) {
      ft_printf("free(): Invalid pointer: %p\n", ptr);
    } else if ((MALLOC_CHECK >> 0) & 1) {
      ft_printf("free(): Invalid pointer\n");
    }
    if ((MALLOC_CHECK >> 1) & 1) {
      abort();
    }
    return;
  }

  switch (block->status) {
    case FREE:
    case FREED:
      if ((MALLOC_CHECK >> 2) & 1) {
        ft_printf("free(): Double free: %p\n", ptr);
      } else if ((MALLOC_CHECK >> 0) & 1) {
        ft_printf("free(): Double free\n");
      }
      if ((MALLOC_CHECK >> 1) & 1) {
        abort();
      }
      unlock_malloc();
      return;
    case ALLOCATED:
      // Mark block as freed and clear data
      block->status = FREED;
      if (MALLOC_PERTURB != 0) {
        ft_memset(get_block_start(block), (0xFF & MALLOC_PERTURB), get_block_size(block));
      }
      coalesce_block(block);
      break;
  }

  unlock_malloc();
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

  lock_malloc();

  Block *block = get_block_from_ptr(ptr);
  if (block == NULL || block->status != ALLOCATED) {
    errno = EINVAL;
    unlock_malloc();
    return (NULL);
  }

  size_t new_total_size = align(size + sizeof(Block), ALIGNMENT);
  size_t current_user_size = get_block_size(block);

  // Determine zone types
  ZoneType new_type = get_zone_type(new_total_size);
  Zone *current_zone = get_zone_from_block(block);
  ZoneType current_type = current_zone ? current_zone->type : LARGE;

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
  void *new_ptr = malloc(size);
  if (new_ptr == NULL) {
    errno = ENOMEM;
    return (NULL);
  }

  // Copy minimum of old and new user data sizes
  size_t copy_size = (current_user_size < size) ? current_user_size : size;
  ft_memcpy(new_ptr, ptr, copy_size);

  free(ptr);
  return (new_ptr);
}

__attribute__((constructor))
static void init() {
  lock_malloc();
  get_zone(TINY, 0);
  get_zone(SMALL, 0);
  unlock_malloc();
}

__attribute__((destructor))
static void clean() {
  lock_malloc();
  Zone *zone = NULL;
  while (base != NULL) {
    zone = base->next;
    munmap(base, base->size);
    base = zone;
  }
  base = NULL;
  unlock_malloc();
  pthread_mutex_destroy(&malloc_mutex);
}
