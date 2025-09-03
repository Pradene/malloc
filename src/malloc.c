#include "malloc.h"

static Zone *base = NULL;
static pthread_mutex_t malloc_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline void lock_malloc() { pthread_mutex_lock(&malloc_mutex); }
static inline void unlock_malloc() { pthread_mutex_unlock(&malloc_mutex); }

static inline size_t align(size_t value, size_t alignment) {
    return ((value + alignment - 1) & ~(alignment - 1));
}

static inline void *get_block_start(Block *block) {
    return (char *)block + sizeof(Block);
}

static inline void *get_zone_start(Zone *zone) {
    return (char *)zone + sizeof(Zone);
}

static inline size_t get_block_size(Block *block) {
    return block->size - sizeof(Block);
}

static void add_to_free_list(Zone *zone, Block *block) {
    if (!zone || !block) return;

    block->free_next = zone->free_blocks;
    block->free_prev = NULL;

    if (zone->free_blocks) {
        zone->free_blocks->free_prev = block;
    }
    zone->free_blocks = block;
}

static void remove_from_free_list(Zone *zone, Block *block) {
    if (!zone || !block) return;

    if (block->free_prev) {
        block->free_prev->free_next = block->free_next;
    } else {
        zone->free_blocks = block->free_next;
    }

    if (block->free_next) {
        block->free_next->free_prev = block->free_prev;
    }

    block->free_prev = NULL;
    block->free_next = NULL;
}

static bool has_zone_cycle(Zone *start) {
    if (!start || !start->next) return false;

    Zone *slow = start;
    Zone *fast = start->next;

    do {
        if (!fast || !fast->next) return false;
        slow = slow->next;
        fast = fast->next->next;
    } while (slow != fast);

    return true;
}

static bool has_block_cycle(Block *start) {
    if (!start || !start->next) return false;

    Block *slow = start;
    Block *fast = start->next;

    do {
        if (!fast || !fast->next) return false;
        slow = slow->next;
        fast = fast->next->next;
    } while (slow != fast);

    return true;
}

static size_t get_alloc_blocks_size(void) {
    size_t size = 0;
    Zone *zone = base;

    if (has_zone_cycle(zone)) return 0;

    while (zone) {
        Block *block = zone->blocks;
        if (has_block_cycle(block)) break;

        while (block) {
            if (block->status == ALLOCATED) {
                size += get_block_size(block);
            }
            block = block->next;
        }
        zone = zone->next;
    }
    return size;
}

static size_t get_alloc_zones_size(void) {
    size_t size = 0;
    Zone *zone = base;

    if (has_zone_cycle(zone)) return 0;

    while (zone) {
        size += zone->size;
        zone = zone->next;
    }
    return size;
}

static bool can_alloc(size_t size) {
    struct rlimit limits;
    if (getrlimit(RLIMIT_AS, &limits) != 0) return false;
    if (limits.rlim_cur == RLIM_INFINITY) return true;

    return (get_alloc_zones_size() + size <= limits.rlim_cur);
}

static size_t get_os_page_size(void) {
    static size_t page_size = 0;

    if (page_size == 0) {
#if defined(__APPLE__) || defined(__MACH__)
        page_size = getpagesize();
#elif defined(unix) || defined(__unix) || defined(__unix__)
        page_size = sysconf(_SC_PAGESIZE);
#else
        page_size = 4096;
#endif
    }

    return page_size;
}

static Block *get_block_from_ptr(void *ptr) {
    if (!ptr) return NULL;

    Zone *zone = base;
    if (has_zone_cycle(zone)) return NULL;

    while (zone) {
        if (!zone->size) break;

        void *zone_start = get_zone_start(zone);
        void *zone_end = (char *)zone + zone->size;

        if (ptr >= zone_start && ptr < zone_end) {
            Block *block = zone->blocks;
            if (has_block_cycle(block)) return NULL;

            while (block) {
                // Bounds and sanity checks
                if ((void *)block < zone_start || (void *)block >= zone_end ||
                    block->size < sizeof(Block) || block->size > zone->size) {
                    break;
                }

                if (get_block_start(block) == ptr) {
                    return block;
                }
                block = block->next;
            }
            return NULL;
        }
        zone = zone->next;
    }
    return NULL;
}

static Zone *get_zone_from_block(Block *block) {
    if (!block) return NULL;

    Zone *zone = base;
    if (has_zone_cycle(zone)) return NULL;

    while (zone) {
        void *zone_start = get_zone_start(zone);
        void *zone_end = (char *)zone + zone->size;

        if ((void *)block >= zone_start && (void *)block < zone_end) {
            return zone;
        }
        zone = zone->next;
    }
    return NULL;
}

static inline ZoneType get_zone_type(size_t size) {
    return (size <= TINY_BLOCK_MAX_SIZE) ? TINY :
           (size <= SMALL_BLOCK_MAX_SIZE) ? SMALL : LARGE;
}

static const char *get_zone_type_str(ZoneType type) {
    static const char *type_names[] = {"TINY", "SMALL", "LARGE", "UNKNOWN"};
    return type_names[(type < 3) ? type : 3];
}

static Zone *get_zone(ZoneType type, size_t size) {
    size_t zone_size;

    switch (type) {
        case TINY: zone_size = TINY_ZONE_SIZE; break;
        case SMALL: zone_size = SMALL_ZONE_SIZE; break;
        default: zone_size = size; break;
    }

    zone_size = align(zone_size + sizeof(Zone), get_os_page_size());

    if (!can_alloc(zone_size)) {
        errno = ENOMEM;
        return NULL;
    }

    void *memory = mmap(NULL, zone_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) {
        errno = ENOMEM;
        return NULL;
    }

    Zone *zone = (Zone *)memory;
    zone->size = zone_size;
    zone->type = type;
    zone->free_blocks = NULL;

    if (!base || zone < base) {
        zone->prev = NULL;
        zone->next = base;
        if (base) base->prev = zone;
        base = zone;
    } else {
        Zone *current = base;
        if (has_zone_cycle(current)) {
            munmap(memory, zone_size);
            return NULL;
        }

        while (current->next && current->next < zone) {
            current = current->next;
        }

        zone->next = current->next;
        zone->prev = current;
        if (current->next) current->next->prev = zone;
        current->next = zone;
    }

    Block *block = (Block *)get_zone_start(zone);
    block->size = zone_size - sizeof(Zone);
    block->status = FREE;
    block->prev = NULL;
    block->next = NULL;
    block->free_prev = NULL;
    block->free_next = NULL;
    zone->blocks = block;

    add_to_free_list(zone, block);

    return zone;
}

static void fragment_block(Block *block, size_t size) {
    if (!block || size < sizeof(Block)) return;

    Zone *zone = get_zone_from_block(block);
    if (!zone) return;

    size_t aligned_size = align(size, ALIGNMENT);
    if (aligned_size > block->size) aligned_size = block->size;

    size_t remaining = block->size - aligned_size;

    remove_from_free_list(zone, block);

    if (remaining >= sizeof(Block) + ALIGNMENT) {
        Block *new_block = (Block *)((char *)block + aligned_size);
        new_block->size = remaining;
        new_block->status = FREE;
        new_block->next = block->next;
        new_block->prev = block;
        new_block->free_prev = NULL;
        new_block->free_next = NULL;

        if (block->next) block->next->prev = new_block;
        block->next = new_block;

        add_to_free_list(zone, new_block);
    }

    block->size = aligned_size;
    block->status = ALLOCATED;

    if (MALLOC_PERTURB) {
        ft_memset(get_block_start(block), ~(0xFF & MALLOC_PERTURB), get_block_size(block));
    }
}

static Block *get_free_block_in_zone_type(ZoneType type, size_t size) {
    Zone *zone = base;
    if (has_zone_cycle(zone)) return NULL;

    while (zone) {
        if (zone->type == type && zone->size > 0) {
            Block *block = zone->free_blocks;

            while (block) {
                void *zone_start = get_zone_start(zone);
                void *zone_end = (char *)zone + zone->size;

                if ((void *)block < zone_start || (void *)block >= zone_end ||
                    block->size < sizeof(Block) || block->size > zone->size) {
                    break;
                }

                if (block->status == FREE && block->size >= size) {
                    return block;
                }
                block = block->free_next;
            }
        }
        zone = zone->next;
    }
    return NULL;
}

static void coalesce_free_blocks(Zone *zone, Block *block) {
    if (!zone || !block || block->status != FREE) return;

    while (block->next && block->next->status == FREE) {
        Block *next = block->next;

        if ((char *)block + block->size == (char *)next) {
            remove_from_free_list(zone, next);

            block->size += next->size;
            block->next = next->next;
            if (next->next) next->next->prev = block;
        } else {
            break;
        }
    }

    while (block->prev && block->prev->status == FREE) {
        Block *prev = block->prev;

        if ((char *)prev + prev->size == (char *)block) {
            remove_from_free_list(zone, block);

            prev->size += block->size;
            prev->next = block->next;
            if (block->next) block->next->prev = prev;

            block = prev;
        } else {
            break;
        }
    }
}

static void print_hex_dump(void *ptr, size_t size) {
    unsigned char *data = (unsigned char *)ptr;
    const size_t bytes_per_line = 16;

    for (size_t i = 0; i < size; i += bytes_per_line) {
        ft_printf("%p: ", (char *)ptr + i);

        size_t line_bytes = (size - i < bytes_per_line) ? size - i : bytes_per_line;

        // Hex output
        for (size_t j = 0; j < bytes_per_line; j++) {
            if (j < line_bytes) {
                ft_printf("%02x ", data[i + j]);
            } else {
                ft_printf("   ");
            }
        }

        // ASCII output
        ft_printf("|");
        for (size_t j = 0; j < line_bytes; j++) {
            unsigned char c = data[i + j];
            ft_printf("%c", (c >= 32 && c <= 126) ? c : '.');
        }
        ft_printf("|\n");
    }
}

static void show_alloc_zone(Zone *zone, bool hex) {
    if (!zone) return;

    ft_printf("%s : %p\n", get_zone_type_str(zone->type), get_zone_start(zone));
    Block *block = zone->blocks;

    if (has_block_cycle(block)) {
        ft_printf("Error: Corrupted block list detected\n");
        return;
    }

    while (block) {
        if (block->status == ALLOCATED) {
            void *start = get_block_start(block);
            size_t size = get_block_size(block);
            ft_printf("%p -> %p : %z bytes\n", start, (char *)start + size, size);

            if (hex) print_hex_dump(start, size);
        }
        block = block->next;
    }
}

void show_alloc_mem(void) {
    lock_malloc();

    if (has_zone_cycle(base)) {
        ft_printf("Error: Corrupted zone list detected\n");
        unlock_malloc();
        return;
    }

    Zone *zone = base;
    while (zone) {
        show_alloc_zone(zone, false);
        zone = zone->next;
    }
    ft_printf("Total : %z bytes\n", get_alloc_blocks_size());
    unlock_malloc();
}

void show_alloc_mem_ex(void) {
    lock_malloc();

    if (has_zone_cycle(base)) {
        ft_printf("Error: Corrupted zone list detected\n");
        unlock_malloc();
        return;
    }

    Zone *zone = base;
    while (zone) {
        show_alloc_zone(zone, true);
        zone = zone->next;
    }
    ft_printf("Total : %z bytes\n", get_alloc_blocks_size());
    unlock_malloc();
}

void *malloc(size_t size) {
    if (!size) return NULL;

    lock_malloc();

    size_t total_size = size + sizeof(Block);
    if (total_size < size) { // Overflow check
        unlock_malloc();
        errno = ENOMEM;
        return NULL;
    }

    ZoneType type = get_zone_type(total_size);
    Block *block = get_free_block_in_zone_type(type, total_size);

    if (block) {
        fragment_block(block, total_size);
        void *result = get_block_start(block);
        unlock_malloc();
        return result;
    }

    Zone *zone = get_zone(type, total_size);
    if (!zone) {
        errno = ENOMEM;
        unlock_malloc();
        return NULL;
    }

    block = zone->blocks;
    if (block->status == FREE && block->size >= total_size) {
        fragment_block(block, total_size);
        void *result = get_block_start(block);
        unlock_malloc();
        return result;
    }

    errno = ENOMEM;
    unlock_malloc();
    return NULL;
}

void free(void *ptr) {
    if (!ptr) return;

    lock_malloc();

    Block *block = get_block_from_ptr(ptr);
    if (!block) {
        unlock_malloc();
        if ((MALLOC_CHECK >> 2) & 1) {
            ft_printf("free(): Invalid pointer: %p\n", ptr);
        } else if (MALLOC_CHECK & 1) {
            ft_printf("free(): Invalid pointer\n");
        }
        if ((MALLOC_CHECK >> 1) & 1) abort();
        return;
    }

    if (block->status != ALLOCATED) {
        if ((MALLOC_CHECK >> 2) & 1) {
            ft_printf("free(): Double free: %p\n", ptr);
        } else if (MALLOC_CHECK & 1) {
            ft_printf("free(): Double free\n");
        }
        if ((MALLOC_CHECK >> 1) & 1) abort();
        unlock_malloc();
        return;
    }

    Zone *zone = get_zone_from_block(block);
    if (!zone) {
        unlock_malloc();
        return;
    }

    block->status = FREE;
    if (MALLOC_PERTURB) {
        ft_memset(get_block_start(block), 0xFF & MALLOC_PERTURB,
                  get_block_size(block));
    }

    add_to_free_list(zone, block);
    coalesce_free_blocks(zone, block);

    unlock_malloc();
}

void *realloc(void *ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (!size) {
        free(ptr);
        return NULL;
    }

    lock_malloc();

    Block *block = get_block_from_ptr(ptr);
    if (!block || block->status != ALLOCATED) {
        errno = EINVAL;
        unlock_malloc();
        return NULL;
    }

    size_t new_total_size = align(size + sizeof(Block), ALIGNMENT);
    size_t current_user_size = get_block_size(block);

    ZoneType new_type = get_zone_type(new_total_size);
    Zone *current_zone = get_zone_from_block(block);
    ZoneType current_type = current_zone ? current_zone->type : LARGE;

    if (new_type == current_type && block->size >= new_total_size) {
        if (block->size - new_total_size >= sizeof(Block) + ALIGNMENT) {
            fragment_block(block, new_total_size);
        }
        unlock_malloc();
        return ptr;
    }

    unlock_malloc();

    void *new_ptr = malloc(size);
    if (!new_ptr) {
        errno = ENOMEM;
        return NULL;
    }

    size_t copy_size = (current_user_size < size) ? current_user_size : size;
    ft_memcpy(new_ptr, ptr, copy_size);
    free(ptr);

    return new_ptr;
}

__attribute__((constructor))
static void init(void) {
    lock_malloc();
    get_zone(TINY, 0);
    get_zone(SMALL, 0);
    unlock_malloc();
}

__attribute__((destructor))
static void clean(void) {
    lock_malloc();
    if (!has_zone_cycle(base)) {
      Zone *next = NULL;
      while (base) {
            next = base->next;
            munmap(base, base->size);
            base = next;
        }
    }
    base = NULL;
    unlock_malloc();
    pthread_mutex_destroy(&malloc_mutex);
}
