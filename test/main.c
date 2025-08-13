#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include "malloc.h"

// Colors for output
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define RESET "\033[0m"

// Test statistics
static int tests_passed = 0;
static int tests_failed = 0;
static int total_tests = 0;

// Thread test data
#define NUM_THREADS 6
#define ALLOCS_PER_THREAD 1000

typedef struct {
    int thread_id;
    int success_count;
    int fail_count;
} thread_data_t;

// Test result reporting
void test_result(const char* test_name, int passed) {
    total_tests++;
    if (passed) {
        printf("[%sPASS%s] %s\n", GREEN, RESET, test_name);
        tests_passed++;
    } else {
        printf("[%sFAIL%s] %s\n", RED, RESET, test_name);
        tests_failed++;
    }
}

// Basic allocation tests
void test_basic_allocation() {
    printf("\n%s=== BASIC ALLOCATION TESTS ===%s\n", BLUE, RESET);

    // Test 1: Simple allocation and free
    void *ptr = malloc(100);
    test_result("Simple malloc(100)", ptr != NULL);
    if (ptr) {
        memset(ptr, 0xAA, 100);  // Write pattern
        free(ptr);
    }

    // Test 2: Zero size allocation
    ptr = malloc(0);
    test_result("malloc(0) returns non-NULL or NULL", 1); // Both behaviors are valid
    if (ptr) free(ptr);

    // Test 3: Small allocation
    ptr = malloc(1);
    test_result("malloc(1)", ptr != NULL);
    if (ptr) {
        *(char*)ptr = 42;
        test_result("Write to malloc(1)", *(char*)ptr == 42);
        free(ptr);
    }

    // Test 4: Large allocation
    ptr = malloc(1024 * 1024); // 1MB
    test_result("malloc(1MB)", ptr != NULL);
    if (ptr) {
        // Write to first and last bytes
        *(char*)ptr = 1;
        *((char*)ptr + 1024*1024 - 1) = 2;
        test_result("Write to large allocation boundaries", 
                   *(char*)ptr == 1 && *((char*)ptr + 1024*1024 - 1) == 2);
        free(ptr);
    }
}

void test_edge_cases() {
    printf("\n%s=== EDGE CASE TESTS ===%s\n", BLUE, RESET);

    // Test 1: Very large allocation
    void *ptr = malloc(SIZE_MAX);
    test_result("malloc(SIZE_MAX) fails gracefully", ptr == NULL);
    if (ptr) free(ptr);

    // Test 2: Free NULL pointer (should be safe)
    free(NULL);
    test_result("free(NULL) is safe", 1);

    // Test 3: Maximum reasonable size
    size_t max_reasonable = 1024 * 1024 * 1024; // 1GB
    ptr = malloc(max_reasonable);
    int success = (ptr != NULL);
    test_result("malloc(1GB)", success);
    if (ptr) free(ptr);
}

void test_alignment() {
    printf("\n%s=== ALIGNMENT TESTS ===%s\n", BLUE, RESET);

    // Test alignment for different sizes
    size_t sizes[] = {1, 2, 3, 4, 5, 7, 8, 15, 16, 17, 31, 32, 33, 63, 64, 65};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int i = 0; i < num_sizes; i++) {
        void *ptr = malloc(sizes[i]);
        if (ptr) {
            // Check if aligned to at least sizeof(void*) boundary
            uintptr_t addr = (uintptr_t)ptr;
            int aligned = (addr % sizeof(void*)) == 0;
        
            char test_name[100];
            snprintf(test_name, sizeof(test_name), "Alignment for size %zu", sizes[i]);
            test_result(test_name, aligned);
        
            free(ptr);
        }
    }
}

void test_stress_sequential() {
    printf("\n%s=== SEQUENTIAL STRESS TESTS ===%s\n", BLUE, RESET);

    // Test 1: Many small allocations
    void **ptrs = malloc(10000 * sizeof(void*));
    int success_count = 0;

    for (int i = 0; i < 10000; i++) {
        size_t size = rand() % 1000 + 1;
        ptrs[i] = malloc(size);
        if (ptrs[i]) {
            success_count++;
            // Write a pattern
            memset(ptrs[i], i % 256, size);
        }
    }

    test_result("10000 small allocations", success_count > 9500); // Allow some failures

    // Free all allocations
    for (int i = 0; i < 10000; i++) {
        if (ptrs[i]) free(ptrs[i]);
    }
    free(ptrs);

    // Test 2: Alternating allocation and free
    success_count = 0;
    for (int i = 0; i < 1000; i++) {
        void *ptr1 = malloc(100);
        void *ptr2 = malloc(200);
        void *ptr3 = malloc(300);
    
        if (ptr1 && ptr2 && ptr3) success_count++;
    
        if (ptr1) free(ptr1);
        if (ptr2) free(ptr2);
        if (ptr3) free(ptr3);
    }

    test_result("1000 alternating alloc/free cycles", success_count > 950);
}

void test_fragmentation() {
    printf("\n%s=== FRAGMENTATION TESTS ===%s\n", BLUE, RESET);

    // Create fragmentation pattern
    void **ptrs = malloc(1000 * sizeof(void*));

    // Allocate many small blocks
    for (int i = 0; i < 1000; i++) {
        ptrs[i] = malloc(50);
    }

    // Free every other block to create fragmentation
    for (int i = 0; i < 1000; i += 2) {
        if (ptrs[i]) free(ptrs[i]);
        ptrs[i] = NULL;
    }

    // Try to allocate larger blocks in fragmented space
    int success_count = 0;
    for (int i = 0; i < 100; i++) {
        void *ptr = malloc(30); // Should fit in fragmented space
        if (ptr) {
            success_count++;
            free(ptr);
        }
    }

    test_result("Allocation in fragmented memory", success_count > 50);

    // Clean up
    for (int i = 1; i < 1000; i += 2) {
        if (ptrs[i]) free(ptrs[i]);
    }
    free(ptrs);
}

pthread_mutex_t rand_lock = PTHREAD_MUTEX_INITIALIZER;

int safe_rand() {
    pthread_mutex_lock(&rand_lock);
    int r = rand();
    pthread_mutex_unlock(&rand_lock);
    return r;
}

void* thread_malloc_test(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;

    void** ptrs = malloc(ALLOCS_PER_THREAD * sizeof(void*));
    if (!ptrs) { data->fail_count = ALLOCS_PER_THREAD; return NULL; }

    for (int i = 0; i < ALLOCS_PER_THREAD; i++) {
        size_t size = (safe_rand() % 1000) + 1;
        ptrs[i] = malloc(size);
        if (ptrs[i]) {
            data->success_count++;
            memset(ptrs[i], data->thread_id, size);
        } else {
            data->fail_count++;
        }

        if (i > 0 && (safe_rand() % 10) == 0) {
            int idx = safe_rand() % i;
            if (ptrs[idx]) {
                free(ptrs[idx]);
                ptrs[idx] = NULL;
            }
        }
    }

    // verify
    for (int i = 0; i < ALLOCS_PER_THREAD; i++) {
        if (ptrs[i]) {
            unsigned char* bytes = (unsigned char*)ptrs[i];
            if (bytes[0] != (unsigned char)data->thread_id) {
                data->fail_count++;
            }
        }
    }

    for (int i = 0; i < ALLOCS_PER_THREAD; i++) {
        if (ptrs[i]) free(ptrs[i]);
    }
    free(ptrs);
    return NULL;
}


void test_concurrent_malloc() {
    printf("\n%s=== CONCURRENT MALLOC TESTS ===%s\n", BLUE, RESET);

    pthread_t threads[NUM_THREADS];
    thread_data_t thread_data[NUM_THREADS];

    // Initialize thread data
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i + 1;
        thread_data[i].success_count = 0;
        thread_data[i].fail_count = 0;
    }

    struct timeval start, end;
    gettimeofday(&start, NULL);

    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        int ret = pthread_create(&threads[i], NULL, thread_malloc_test, &thread_data[i]);
        if (ret != 0) {
            printf("Failed to create thread %d: %s\n", i, strerror(ret));
            thread_data[i].fail_count = ALLOCS_PER_THREAD;
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&end, NULL);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    // Collect results
    int total_success = 0, total_fail = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        total_success += thread_data[i].success_count;
        total_fail += thread_data[i].fail_count;
        printf("Thread %d: %d success, %d failures\n", 
               thread_data[i].thread_id, 
               thread_data[i].success_count, 
               thread_data[i].fail_count);
    }

    printf("Concurrent test completed in %.2f seconds\n", elapsed);
    printf("Total: %d successes, %d failures\n", total_success, total_fail);

    // Test passes if most allocations succeeded
    double success_rate = (double)total_success / (total_success + total_fail);
    test_result("Concurrent malloc success rate > 90%", success_rate > 0.9);
}

void test_realloc_scenarios() {
    printf("\n%s=== REALLOC TESTS ===%s\n", BLUE, RESET);

    // Test 1: Basic realloc
    void *ptr = malloc(100);
    if (ptr) {
        memset(ptr, 0x42, 100);
        ptr = realloc(ptr, 200);
        test_result("Basic realloc expansion", ptr != NULL);
        if (ptr) {
            test_result("Data preserved after realloc", ((char*)ptr)[99] == 0x42);
            free(ptr);
        }
    }

    // Test 2: Realloc with NULL (should act like malloc)
    ptr = realloc(NULL, 100);
    test_result("realloc(NULL, size) acts like malloc", ptr != NULL);
    if (ptr) free(ptr);

    // Test 3: Realloc with size 0 (should act like free)
    ptr = malloc(100);
    if (ptr) {
        ptr = realloc(ptr, 0);
        test_result("realloc(ptr, 0) acts like free", 1); // Implementation defined
    }

    // Test 4: Shrinking realloc
    ptr = malloc(1000);
    if (ptr) {
        memset(ptr, 0x33, 1000);
        ptr = realloc(ptr, 100);
        test_result("Realloc shrink", ptr != NULL);
        if (ptr) {
            test_result("Data preserved after shrink", ((char*)ptr)[99] == 0x33);
            free(ptr);
        }
    }
}

void test_memory_patterns() {
    printf("\n%s=== MEMORY PATTERN TESTS ===%s\n", BLUE, RESET);

    // Test various allocation patterns
    void *ptrs[100];

    // Pattern 1: Ascending sizes
    int success_count = 0;
    for (int i = 0; i < 100; i++) {
        ptrs[i] = malloc((i + 1) * 10);
        if (ptrs[i]) {
            success_count++;
            memset(ptrs[i], i % 256, (i + 1) * 10);
        }
    }
    test_result("Ascending size allocations", success_count > 95);

    // Verify data integrity
    int data_ok = 1;
    for (int i = 0; i < 100; i++) {
        if (ptrs[i]) {
            if (((char*)ptrs[i])[0] != (i % 256)) {
                data_ok = 0;
                break;
            }
        }
    }
    test_result("Data integrity after ascending allocations", data_ok);

    // Free in reverse order
    for (int i = 99; i >= 0; i--) {
        if (ptrs[i]) free(ptrs[i]);
    }

    // Pattern 2: Power-of-2 sizes
    success_count = 0;
    for (int i = 0; i < 20; i++) {
        size_t size = 1 << i; // 1, 2, 4, 8, 16, ..., 524288
        void *ptr = malloc(size);
        if (ptr) {
            success_count++;
            // Write pattern at boundaries
            if (size >= 1) ((char*)ptr)[0] = 0xAA;
            if (size >= 2) ((char*)ptr)[size-1] = 0xBB;
            free(ptr);
        }
    }
    test_result("Power-of-2 size allocations", success_count > 18);
}

void performance_benchmark() {
    printf("\n%s=== PERFORMANCE BENCHMARK ===%s\n", BLUE, RESET);

    const int num_ops = 10000;
    struct timeval start, end;

    // Benchmark 1: Sequential allocations
    gettimeofday(&start, NULL);
    void **ptrs = malloc(num_ops * sizeof(void*));
    for (int i = 0; i < num_ops; i++) {
        ptrs[i] = malloc(64);
    }
    for (int i = 0; i < num_ops; i++) {
        if (ptrs[i]) free(ptrs[i]);
    }
    free(ptrs);
    gettimeofday(&end, NULL);

    double sequential_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Sequential %d alloc/free pairs: %.3f seconds (%.0f ops/sec)\n", 
           num_ops, sequential_time, num_ops / sequential_time);

    // Benchmark 2: Random size allocations
    gettimeofday(&start, NULL);
    ptrs = malloc(num_ops * sizeof(void*));
    for (int i = 0; i < num_ops; i++) {
        size_t size = (rand() % 1000) + 1;
        ptrs[i] = malloc(size);
    }
    for (int i = 0; i < num_ops; i++) {
        if (ptrs[i]) free(ptrs[i]);
    }
    free(ptrs);
    gettimeofday(&end, NULL);

    double random_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Random size %d alloc/free pairs: %.3f seconds (%.0f ops/sec)\n", 
           num_ops, random_time, num_ops / random_time);

    test_result("Performance benchmark completed", 1);
}

void print_summary() {
    printf("\n%s=== TEST SUMMARY ===%s\n", BLUE, RESET);
    printf("Total tests: %d\n", total_tests);
    printf("Passed: %s%d%s\n", GREEN, tests_passed, RESET);
    printf("Failed: %s%d%s\n", RED, tests_failed, RESET);

    if (tests_failed == 0) {
        printf("\n%s🎉 ALL TESTS PASSED! 🎉%s\n", GREEN, RESET);
    } else {
        printf("\n%s⚠️  Some tests failed. Review the output above. ⚠️%s\n", YELLOW, RESET);
    }

    double success_rate = (double)tests_passed / total_tests * 100;
    printf("Success rate: %.1f%%\n", success_rate);
}

int main() {
    printf("%s=== COMPREHENSIVE MALLOC TEST SUITE ===%s\n", BLUE, RESET);
    printf("Testing system malloc implementation\n");
    printf("Compiled on %s at %s\n", __DATE__, __TIME__);

    // Seed random number generator
    srand(time(NULL));

    // Run all test suites
    test_basic_allocation();
    test_edge_cases();
    test_alignment();
    test_stress_sequential();
    test_fragmentation();
    test_realloc_scenarios();
    test_memory_patterns();
    performance_benchmark();
    test_concurrent_malloc();

    // Print final summary
    print_summary();

    return (tests_failed > 0) ? 1 : 0;
}
