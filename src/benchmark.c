#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#include "vedalloc.h"
#include "vedalloc_v2.h"

#define ITER 1000000
#define BULK 1000
#define STRESS_OPS 200000

long ns() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e9 + t.tv_nsec;
}

void print_result(const char *label, long v1, long v2, long std) {
    printf("\n%s\n", label);
    printf("vedalloc: %.3f ms\n", v1 / 1e6);
    printf("vedalloc_v2: %.3f ms\n", v2 / 1e6);
    printf("malloc  : %.3f ms\n", std / 1e6);
}

void bench_single() {

    vedalloc_reset();
    long start = ns();
    for (int i = 0; i < ITER; i++) {
        void *p = vedalloc(32);
        vedfree(p);
    }
    long v1 = ns() - start;

    start = ns();
    for (int i = 0; i < ITER; i++) {
        void *p = vedalloc_v2(32);
        vedfree_v2(p);
    }
    long v2 = ns() - start;

    start = ns();
    for (int i = 0; i < ITER; i++) {
        void *p = malloc(32);
        free(p);
    }
    long std = ns() - start;

    print_result("single size (32 bytes)", v1, v2, std);
}

void bench_mixed() {
    size_t sizes[] = {8, 16, 32, 64, 128, 256, 512};

    vedalloc_reset();
    long start = ns();
    for (int i = 0; i < ITER; i++) {
        void *p = vedalloc(sizes[i % 7]);
        vedfree(p);
    }
    long v1 = ns() - start;

    start = ns();
    for (int i = 0; i < ITER; i++) {
        void *p = vedalloc_v2(sizes[i % 7]);
        vedfree_v2(p);
    }
    long v2 = ns() - start;

    start = ns();
    for (int i = 0; i < ITER; i++) {
        void *p = malloc(sizes[i % 7]);
        free(p);
    }
    long std = ns() - start;

    print_result("mixed sizes", v1, v2, std);
}

void bench_bulk() {
    vedalloc_reset();
    void *ptrs[BULK];

    long start = ns();
    for (int i = 0; i < BULK; i++) ptrs[i] = vedalloc(64);
    for (int i = 0; i < BULK; i++) vedfree(ptrs[i]);
    long v1 = ns() - start;

    start = ns();
    for (int i = 0; i < BULK; i++) ptrs[i] = vedalloc_v2(64);
    for (int i = 0; i < BULK; i++) vedfree_v2(ptrs[i]);
    long v2 = ns() - start;

    start = ns();
    for (int i = 0; i < BULK; i++) ptrs[i] = malloc(64);
    for (int i = 0; i < BULK; i++) free(ptrs[i]);
    long std = ns() - start;

    print_result("bulk alloc 1000 + free", v1, v2, std);
}

void bench_reverse() {
    void *ptrs[BULK];

    vedalloc_reset();
    long start = ns();
    for (int i = 0; i < BULK; i++) ptrs[i] = vedalloc(64);
    for (int i = BULK - 1; i >= 0; i--) vedfree(ptrs[i]);
    long v1 = ns() - start;

    start = ns();
    for (int i = 0; i < BULK; i++) ptrs[i] = vedalloc_v2(64);
    for (int i = BULK - 1; i >= 0; i--) vedfree_v2(ptrs[i]);
    long v2 = ns() - start;

    start = ns();
    for (int i = 0; i < BULK; i++) ptrs[i] = malloc(64);
    for (int i = BULK - 1; i >= 0; i--) free(ptrs[i]);
    long std = ns() - start;

    print_result("reverse free", v1, v2, std);
}

void bench_stress() {
    void *arr1[1000] = {0};
    void *arr2[1000] = {0};
    void *arr3[1000] = {0};

    vedalloc_reset();
    long start = ns();
    for (int i = 0; i < STRESS_OPS; i++) {
        int idx = rand() % 1000;
        if (arr1[idx]) {
            vedfree(arr1[idx]);
            arr1[idx] = NULL;
        } else {
            arr1[idx] = vedalloc(rand() % 256 + 1);
        }
    }
    long v1 = ns() - start;

    start = ns();
    for (int i = 0; i < STRESS_OPS; i++) {
        int idx = rand() % 1000;
        if (arr2[idx]) {
            vedfree_v2(arr2[idx]);
            arr2[idx] = NULL;
        } else {
            arr2[idx] = vedalloc_v2(rand() % 256 + 1);
        }
    }
    long v2 = ns() - start;

    start = ns();
    for (int i = 0; i < STRESS_OPS; i++) {
        int idx = rand() % 1000;
        if (arr3[idx]) {
            free(arr3[idx]);
            arr3[idx] = NULL;
        } else {
            arr3[idx] = malloc(rand() % 256 + 1);
        }
    }
    long std = ns() - start;

    print_result("stress random workload", v1, v2, std);
}

// void bench_fragment_reuse() {
//     vedalloc_reset();
//     long start = ns();
//     for (int i = 0; i < ITER; i++) {
//         void *a = vedalloc(100);
//         void *b = vedalloc(200);
//         void *c = vedalloc(300);

//         vedfree(b);
//         void *d = vedalloc(150);

//         vedfree(a);
//         vedfree(c);
//         vedfree(d);
//     }
//     long my_ns = ns() - start;

//     start = ns();
//     for (int i = 0; i < ITER; i++) {
//         void *a = malloc(100);
//         void *b = malloc(200);
//         void *c = malloc(300);

//         free(b);
//         void *d = malloc(150);

//         free(a);
//         free(c);
//         free(d);
//     }
//     long std_ns = ns() - start;

//     print_result("fragment reuse", my_ns, std_ns);
// }

// void fragmentation_stats() {
//     if (!heap_start) return;

//     block_header *b = (block_header *)(heap_start + sizeof(heap_header));
//     size_t total_free = 0;
//     size_t largest = 0;

//     while (b) {
//         if (!b->in_use) {
//             total_free += b->size;
//             if (b->size > largest) largest = b->size;
//         }
//         b = b->next;
//     }

//     printf("\nFragmentation stats:\n");
//     printf("total free memory: %zu bytes\n", total_free);
//     printf("largest free block: %zu bytes\n", largest);

//     if (total_free > 0)
//         printf("external fragmentation: %.2f%%\n",
//                100.0 * (1.0 - (double)largest / total_free));
// }

int main() {
    bench_single();
    bench_mixed();
    bench_bulk();
    bench_reverse();
    bench_stress();
    // bench_fragment_reuse();
    // fragmentation_stats();
    return 0;
}