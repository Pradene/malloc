// malloc_realloc_tests.c
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <stdalign.h>
#include <stddef.h>


// Colors for output
#define GREEN  "\033[0;32m"
#define RED    "\033[0;31m"
#define YELLOW "\033[0;33m"
#define BLUE   "\033[0;34m"
#define RESET  "\033[0m"

// Test statistics
static int tests_passed = 0;
static int tests_failed = 0;
static int total_tests  = 0;

// Thread test data
#define NUM_THREADS        4
#define ALLOCS_PER_THREAD  1000

typedef struct {
    int thread_id;
    int success_count;
    int fail_count;
} thread_data_t;

// ---------- Test result helpers ----------
static void test_result(const char* test_name, int passed) {
    total_tests++;
    if (passed) {
        printf("[%sPASS%s] %s\n", GREEN, RESET, test_name);
        tests_passed++;
    } else {
        printf("[%sFAIL%s] %s\n", RED, RESET, test_name);
        tests_failed++;
    }
}

static void* xmalloc(size_t n) {
    void* p = malloc(n);
    if (!p) {
        fprintf(stderr, "[WARN] malloc(%zu) returned NULL\n", n);
    }
    return p;
}

// ---------- Basic allocation tests ----------
static void test_basic_allocation(void) {
    printf("\n%s=== BASIC ALLOCATION TESTS ===%s\n", BLUE, RESET);

    void *ptr = malloc(100);
    test_result("Simple malloc(100)", ptr != NULL);
    if (ptr) {
        memset(ptr, 0xAA, 100);
        free(ptr);
    }

    ptr = malloc(0);
    // Both behaviors allowed: NULL or unique freeable pointer
    test_result("malloc(0) returns NULL or unique pointer", 1);
    if (ptr) free(ptr);

    ptr = malloc(1);
    test_result("malloc(1)", ptr != NULL);
    if (ptr) {
        *(char*)ptr = 42;
        test_result("Write to malloc(1)", *(char*)ptr == 42);
        free(ptr);
    }

    ptr = malloc(1024 * 1024); // 1MB
    test_result("malloc(1MB)", ptr != NULL);
    if (ptr) {
        *(char*)ptr = 1;
        *((char*)ptr + 1024*1024 - 1) = 2;
        test_result("Write to large allocation boundaries",
                    *(char*)ptr == 1 && *((char*)ptr + 1024*1024 - 1) == 2);
        free(ptr);
    }
}

// ---------- Edge cases ----------
static void test_edge_cases(void) {
    printf("\n%s=== EDGE CASE TESTS ===%s\n", BLUE, RESET);

    // free(NULL) is always safe
    free(NULL);
    test_result("free(NULL) is safe", 1);

    // Very large allocation should either succeed or fail with ENOMEM (don’t count as test failure)
    errno = 0;
    size_t max_reasonable = (size_t)1024 * 1024 * 1024; // 1GB
    void *ptr = malloc(max_reasonable);
    int ok = (ptr != NULL) || (ptr == NULL && errno == ENOMEM);
    test_result("malloc(1GB) succeeds or fails with ENOMEM", ok);
    if (ptr) free(ptr);
}

// ---------- Alignment ----------
static void test_alignment(void) {
    printf("\n%s=== ALIGNMENT TESTS ===%s\n", BLUE, RESET);

#if __STDC_VERSION__ >= 201112L
    const size_t min_align = _Alignof(max_align_t);
#else
    const size_t min_align = sizeof(void*); // fallback
#endif

    size_t sizes[] = {1,2,3,4,5,7,8,15,16,17,31,32,33,63,64,65,127,128,255,256};
    int num_sizes = (int)(sizeof(sizes) / sizeof(sizes[0]));

    for (int i = 0; i < num_sizes; i++) {
        void *ptr = malloc(sizes[i]);
        if (ptr) {
            uintptr_t addr = (uintptr_t)ptr;
            int aligned = (addr % min_align) == 0;
            char name[96];
            snprintf(name, sizeof(name), "Alignment >= alignof(max_align_t) for %zu", sizes[i]);
            test_result(name, aligned);
            free(ptr);
        }
    }
}

// ---------- Sequential stress ----------
static void test_stress_sequential(void) {
    printf("\n%s=== SEQUENTIAL STRESS TESTS ===%s\n", BLUE, RESET);

    void **ptrs = xmalloc(10000 * sizeof(void*));
    test_result("Allocate pointer array for stress", ptrs != NULL);
    if (!ptrs) return;

    int success_count = 0;
    for (int i = 0; i < 10000; i++) {
        ptrs[i] = malloc(4);
        if (ptrs[i]) {
            success_count++;
            memset(ptrs[i], i & 0xFF, 4);
        }
    }
    test_result("10000 small allocations (>=95%)", success_count > 9500);

    for (int i = 0; i < 10000; i++) if (ptrs[i]) free(ptrs[i]);
    free(ptrs);

    success_count = 0;
    for (int i = 0; i < 1000; i++) {
        void *p1 = malloc(100);
        void *p2 = malloc(200);
        void *p3 = malloc(300);
        if (p1 && p2 && p3) success_count++;
        if (p1) free(p1);
        if (p2) free(p2);
        if (p3) free(p3);
    }
    test_result("1000 alternating alloc/free cycles (>=95%)", success_count > 950);
}

// ---------- Fragmentation ----------
static void test_fragmentation(void) {
    printf("\n%s=== FRAGMENTATION TESTS ===%s\n", BLUE, RESET);

    void **ptrs = xmalloc(1000 * sizeof(void*));
    test_result("Allocate pointer array for fragmentation", ptrs != NULL);
    if (!ptrs) return;

    for (int i = 0; i < 1000; i++) ptrs[i] = malloc(50);
    for (int i = 0; i < 1000; i += 2) { if (ptrs[i]) free(ptrs[i]); ptrs[i] = NULL; }

    int success_count = 0;
    for (int i = 0; i < 200; i++) {
        void *p = malloc(30);
        if (p) { success_count++; free(p); }
    }
    test_result("Allocation in fragmented memory (>=50%)", success_count >= 100);

    for (int i = 1; i < 1000; i += 2) if (ptrs[i]) free(ptrs[i]);
    free(ptrs);
}

// ---------- Concurrent malloc-only test ----------
static void* thread_malloc_test(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    void** ptrs = (void**)malloc(ALLOCS_PER_THREAD * sizeof(void*));
    if (!ptrs) { data->fail_count = ALLOCS_PER_THREAD; return NULL; }

    for (int i = 0; i < ALLOCS_PER_THREAD; i++) {
        size_t size = (size_t)(rand() % 1000 + 1);
        ptrs[i] = malloc(size);
        if (ptrs[i]) {
            data->success_count++;
            memset(ptrs[i], data->thread_id, size);
        } else {
            data->fail_count++;
        }
        if (i > 0 && (rand() % 10) == 0) {
            int idx = rand() % i;
            if (ptrs[idx]) { free(ptrs[idx]); ptrs[idx] = NULL; }
        }
    }

    for (int i = 0; i < ALLOCS_PER_THREAD; i++) if (ptrs[i]) free(ptrs[i]);
    free(ptrs);
    return NULL;
}

static void test_concurrent_malloc(void) {
    printf("\n%s=== CONCURRENT MALLOC TESTS ===%s\n", BLUE, RESET);

    pthread_t threads[NUM_THREADS];
    thread_data_t td[NUM_THREADS] = {0};

    for (int i = 0; i < NUM_THREADS; i++) {
        td[i].thread_id = i + 1;
        int ret = pthread_create(&threads[i], NULL, thread_malloc_test, &td[i]);
        if (ret != 0) {
            printf("Failed to create thread %d: %s\n", i, strerror(ret));
            td[i].fail_count = ALLOCS_PER_THREAD;
        }
    }
    for (int i = 0; i < NUM_THREADS; i++) pthread_join(threads[i], NULL);

    int total_success = 0, total_fail = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        total_success += td[i].success_count;
        total_fail    += td[i].fail_count;
        printf("Thread %d: %d success, %d failures\n",
               td[i].thread_id, td[i].success_count, td[i].fail_count);
    }
    double success_rate = (double)total_success / (total_success + total_fail + 1e-9);
    printf("Total: %d successes, %d failures (%.1f%%)\n",
           total_success, total_fail, success_rate * 100.0);
    test_result("Concurrent malloc success rate > 90%", success_rate > 0.9);
}

// ---------- Your existing realloc scenarios ----------
static void test_realloc_scenarios(void) {
    printf("\n%s=== REALLOC TESTS ===%s\n", BLUE, RESET);

    void *ptr = malloc(100);
    if (ptr) {
        memset(ptr, 0x42, 100);
        void *q = realloc(ptr, 200);
        test_result("Basic realloc expansion", q != NULL);
        if (q) {
            test_result("Data preserved after realloc (prefix)", ((unsigned char*)q)[99] == 0x42);
            free(q);
        }
    }

    ptr = realloc(NULL, 100);
    test_result("realloc(NULL, size) acts like malloc", ptr != NULL);
    if (ptr) free(ptr);

    ptr = malloc(100);
    if (ptr) {
        void *q = realloc(ptr, 0);
        test_result("realloc(ptr, 0) returns NULL or freeable pointer", 1);
        if (q) free(q);
    }

    ptr = malloc(1000);
    if (ptr) {
        memset(ptr, 0x33, 1000);
        void *q = realloc(ptr, 100);
        test_result("Realloc shrink", q != NULL);
        if (q) {
            test_result("Shrink preserves first 100 bytes", ((unsigned char*)q)[99] == 0x33);
            free(q);
        }
    }
}

// ---------- Comprehensive realloc suite ----------
static void test_realloc_basic(void) {
    printf("\n%s=== REALLOC: BASIC ===%s\n", BLUE, RESET);

    void *p = realloc(NULL, 128);
    test_result("realloc(NULL, 128) returns non-NULL", p != NULL);
    if (p) { memset(p, 0xAB, 128); test_result("… usable", ((unsigned char*)p)[0] == 0xAB); free(p); }

    p = malloc(64);
    test_result("malloc(64) before realloc(...,0)", p != NULL);
    if (p) {
        memset(p, 0xCD, 64);
        void *q = realloc(p, 0);
        test_result("realloc(p,0) returns NULL or freeable pointer", 1);
        if (q) free(q);
    }
}

static void test_realloc_shrink_preserves_prefix(void) {
    printf("\n%s=== REALLOC: SHRINK ===%s\n", BLUE, RESET);

    size_t oldN = 1000, newN = 200;
    unsigned char *p = (unsigned char*)malloc(oldN);
    test_result("malloc(oldN)", p != NULL);
    if (!p) return;

    for (size_t i = 0; i < oldN; i++) p[i] = (unsigned char)(i & 0xFF);

    unsigned char *q = (unsigned char*)realloc(p, newN);
    test_result("realloc shrink returns non-NULL", q != NULL);
    if (!q) return;

    int ok = 1;
    for (size_t i = 0; i < newN; i++) if (q[i] != (unsigned char)(i & 0xFF)) { ok = 0; break; }
    test_result("shrink preserves first newN bytes", ok);

    q[newN - 1] ^= 0xFF;
    test_result("write at new end ok", 1);

    free(q);
}

static void test_realloc_grow_preserves_prefix(void) {
    printf("\n%s=== REALLOC: GROW ===%s\n", BLUE, RESET);

    size_t oldN = 256, newN = 4096;
    unsigned char *p = (unsigned char*)malloc(oldN);
    test_result("malloc(oldN)", p != NULL);
    if (!p) return;

    memset(p, 0x5A, oldN);
    unsigned char *q = (unsigned char*)realloc(p, newN);
    test_result("realloc grow returns non-NULL", q != NULL);
    if (!q) return;

    int ok = 1;
    for (size_t i = 0; i < oldN; i++) if (q[i] != 0x5A) { ok = 0; break; }
    test_result("grow preserves old prefix", ok);

    memset(q + oldN, 0xC3, newN - oldN);
    test_result("grow new tail writable", 1);

    free(q);
}

// Bounded grow/shrink cycles so it never balloons
static void test_realloc_cycle(void) {
    printf("\n%s=== REALLOC: GROW/SHRINK CYCLES (CAPPED) ===%s\n", BLUE, RESET);

    const size_t MAX_N = 1 << 20; // 1 MiB cap
    size_t n = 64;
    unsigned char *p = (unsigned char*)malloc(n);
    test_result("malloc(64)", p != NULL);
    if (!p) return;

    unsigned pat = 0;
    for (size_t i = 0; i < n; i++) p[i] = (unsigned char)((i + pat) & 0xFF);

    int ok = 1;
    for (int it = 0; it < 200 && ok; it++) {
        size_t newN;
        if ((it & 1) == 0) {
            size_t grow = n + (n >> 1) + 128; // ~1.5x + 128
            newN = (grow > MAX_N) ? MAX_N : grow;
            if (newN == n) newN = n + 1;
        } else {
            newN = (n > 2) ? (n / 2 + 1) : 1;
        }

        unsigned char *q = (unsigned char*)realloc(p, newN);
        if (!q) { ok = 0; break; }

        size_t keep = (n < newN) ? n : newN;
        for (size_t i = 0; i < keep; i++) {
            if (q[i] != (unsigned char)((i + pat) & 0xFF)) { ok = 0; break; }
        }
        if (!ok) { free(q); break; }

        pat = (pat + 37) & 0xFF;
        for (size_t i = 0; i < newN; i++) q[i] = (unsigned char)((i + pat) & 0xFF);

        p = q;
        n = newN;
    }

    test_result("realloc cycles preserve prefix", ok);
    if (p) free(p);
}

static void test_realloc_stress_various_sizes(void) {
    printf("\n%s=== REALLOC: STRESS (VARIOUS SIZES) ===%s\n", BLUE, RESET);

    const int R = 2000;
    unsigned char *p = NULL;
    size_t n = 0;
    int ok = 1;

    for (int i = 0; i < R; i++) {
        size_t newN = (size_t)(rand() % 8192) + 1;
        for (size_t j = 0; j < n; j++) p[j] = (unsigned char)((j + i) & 0xFF);

        unsigned char *q = (unsigned char*)realloc(p, newN);
        if (!q) { ok = 0; free(p); break; }

        size_t keep = (n < newN) ? n : newN;
        for (size_t j = 0; j < keep; j++) {
            if (q[j] != (unsigned char)((j + i) & 0xFF)) { ok = 0; break; }
        }
        if (!ok) { free(q); break; }

        for (size_t j = 0; j < newN; j++) q[j] = (unsigned char)((j ^ i) & 0xFF);

        p = q;
        n = newN;
    }

    if (p) free(p);
    test_result("Random-size realloc stress with data checks", ok);
}

// ---------- Concurrent realloc churn ----------
typedef struct {
    int tid;
    unsigned char *buf;
    size_t size;
    int iterations;
    int ok;
} realloc_thread_t;

static void* realloc_churn_thread(void *arg) {
    realloc_thread_t *T = (realloc_thread_t*)arg;
    T->ok = 1;
    T->size = 64;
    T->buf  = (unsigned char*)malloc(T->size);
    if (!T->buf) { T->ok = 0; return NULL; }
    memset(T->buf, T->tid, T->size);

    for (int i = 0; i < T->iterations && T->ok; i++) {
        for (size_t j = 0; j < T->size; j++) T->buf[j] = (unsigned char)(T->tid + (j % 13));

        size_t delta = (size_t)(rand() % 2048);
        size_t newN  = (i % 3 == 0) ? (T->size + delta + 1)
                                    : (T->size > delta + 1 ? T->size - (delta + 1) : 1);

        unsigned char *q = (unsigned char*)realloc(T->buf, newN);
        if (!q) { T->ok = 0; break; }

        size_t keep = T->size < newN ? T->size : newN;
        for (size_t j = 0; j < keep; j++) {
            if (q[j] != (unsigned char)(T->tid + (j % 13))) { T->ok = 0; break; }
        }
        if (!T->ok) { free(q); break; }

        for (size_t j = 0; j < newN; j++) q[j] = (unsigned char)((T->tid ^ (j % 251)) & 0xFF);

        T->buf  = q;
        T->size = newN;
    }
    if (T->buf) free(T->buf);
    return NULL;
}

static void test_realloc_concurrent(void) {
    printf("\n%s=== REALLOC: CONCURRENT CHURN ===%s\n", BLUE, RESET);

    const int N = 4;
    pthread_t th[N];
    realloc_thread_t td[N];
    int ok = 1;

    for (int i = 0; i < N; i++) {
        td[i].tid = 0x10 + i;
        td[i].buf = NULL;
        td[i].size = 0;
        td[i].iterations = 1500;
        td[i].ok = 0;
        int r = pthread_create(&th[i], NULL, realloc_churn_thread, &td[i]);
        if (r != 0) ok = 0;
    }
    for (int i = 0; i < N; i++) pthread_join(th[i], NULL);
    for (int i = 0; i < N; i++) if (!td[i].ok) ok = 0;

    test_result("Concurrent realloc churn preserved prefixes & stability", ok);
}

// Wrapper to call realloc tests
static void test_realloc_suite(void) {
    test_realloc_basic();
    test_realloc_shrink_preserves_prefix();
    test_realloc_grow_preserves_prefix();
    test_realloc_cycle();
    test_realloc_stress_various_sizes();
    test_realloc_concurrent();
}

// ---------- Performance benchmark (kept as-is) ----------
static double sec_since(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) / 1e9;
}

static double median(double *v, int n) {
    for (int i = 1; i < n; i++) {
        double x = v[i]; int j = i - 1;
        while (j >= 0 && v[j] > x) { v[j+1] = v[j]; j--; }
        v[j+1] = x;
    }
    return (n % 2) ? v[n/2] : 0.5*(v[n/2 - 1] + v[n/2]);
}

void performance_benchmark(void) {
    printf("\n%s=== PERFORMANCE BENCHMARK ===%s\n", BLUE, RESET);

    const int pairs      = 100000;
    const int trials     = 5;
    double times_seq[trials], times_rand[trials], times_inter[trials];

    srand(12345);

    for (int t = 0; t < trials; t++) {
        struct timespec a,b;

        void **ptrs = (void **)malloc((size_t)pairs * sizeof(void *));
        clock_gettime(CLOCK_MONOTONIC, &a);
        for (int i = 0; i < pairs; i++) ptrs[i] = malloc(64);
        for (int i = 0; i < pairs; i++) free(ptrs[i]);
        clock_gettime(CLOCK_MONOTONIC, &b);
        times_seq[t] = sec_since(a,b);
        free(ptrs);

        ptrs = (void **)malloc((size_t)pairs * sizeof(void *));
        clock_gettime(CLOCK_MONOTONIC, &a);
        for (int i = 0; i < pairs; i++) {
            size_t sz = (size_t)(rand() % 1024 + 1);
            ptrs[i] = malloc(sz);
        }
        for (int i = 0; i < pairs; i++) free(ptrs[i]);
        clock_gettime(CLOCK_MONOTONIC, &b);
        times_rand[t] = sec_since(a,b);
        free(ptrs);

        clock_gettime(CLOCK_MONOTONIC, &a);
        void *last = NULL;
        for (int i = 0; i < pairs; i++) {
            size_t sz = (size_t)(rand() % 2048 + 1);
            void *p = malloc(sz);
            if (last) free(last);
            last = p;
        }
        if (last) free(last);
        clock_gettime(CLOCK_MONOTONIC, &b);
        times_inter[t] = sec_since(a,b);
    }

    double s_seq   = median(times_seq,  trials);
    double s_rand  = median(times_rand, trials);
    double s_inter = median(times_inter,trials);

    double pairs_per_sec_seq   = pairs / s_seq;
    double ops_per_sec_seq     = (pairs * 2.0) / s_seq;

    double pairs_per_sec_rand  = pairs / s_rand;
    double ops_per_sec_rand    = (pairs * 2.0) / s_rand;

    double pairs_per_sec_inter = pairs / s_inter;
    double ops_per_sec_inter   = (pairs * 2.0) / s_inter;

    printf("Sequential %d alloc/free pairs: %.3f s  (%.0f pairs/s, %.0f ops/s)\n",
           pairs, s_seq, pairs_per_sec_seq, ops_per_sec_seq);

    printf("Random size %d alloc/free pairs: %.3f s  (%.0f pairs/s, %.0f ops/s)\n",
           pairs, s_rand, pairs_per_sec_rand, ops_per_sec_rand);

    printf("Interleaved %d alloc/free pairs: %.3f s  (%.0f pairs/s, %.0f ops/s)\n",
           pairs, s_inter, pairs_per_sec_inter, ops_per_sec_inter);

    test_result("Performance benchmark completed", 1);
}

// ---------- Summary ----------
static void print_summary(void) {
    printf("\n%s=== TEST SUMMARY ===%s\n", BLUE, RESET);
    printf("Total tests: %d\n", total_tests);
    printf("Passed: %s%d%s\n", GREEN, tests_passed, RESET);
    printf("Failed: %s%d%s\n", RED, tests_failed, RESET);

    if (tests_failed == 0) {
        printf("\n%s🎉 ALL TESTS PASSED! 🎉%s\n", GREEN, RESET);
    } else {
        printf("\n%s⚠️  Some tests failed. Review the output above. ⚠️%s\n", YELLOW, RESET);
    }

    double success_rate = (double)tests_passed / (double)(total_tests ? total_tests : 1) * 100.0;
    printf("Success rate: %.1f%%\n", success_rate);
}

// ---------- Main ----------
int main(void) {
    printf("%s=== COMPREHENSIVE MALLOC/REALLOC TEST SUITE ===%s\n", BLUE, RESET);
    printf("Testing implementation compiled on %s at %s\n", __DATE__, __TIME__);

    srand((unsigned)time(NULL));

    test_basic_allocation();
    test_edge_cases();
    test_alignment();
    test_stress_sequential();
    test_fragmentation();

    test_realloc_scenarios(); // your original quick tests

    // Keep benchmark as requested
    performance_benchmark();

    test_concurrent_malloc();

    // New comprehensive realloc tests
    test_realloc_suite();

    print_summary();
    return (tests_failed > 0) ? 1 : 0;
}
