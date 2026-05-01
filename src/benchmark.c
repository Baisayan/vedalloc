#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include "vedalloc.h"

#define ITER 1000000
#define BULK 1000
#define STRESS_OPS 200000

long ns() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e9 + t.tv_nsec;
}

void print_result(const char *label, long my_ns, long std_ns) {
    printf("\n%s\n", label);
    printf("vedalloc: %.3f ms\n", my_ns / 1e6);
    printf("malloc  : %.3f ms\n", std_ns / 1e6);
}

void bench_single() {
    vedalloc_reset();

    long start = ns();
    for (int i = 0; i < ITER; i++) {
        void *p = vedalloc(32);
        vedfree(p);
    }
    long my_ns = ns() - start;

    start = ns();
    for (int i = 0; i < ITER; i++) {
        void *p = malloc(32);
        free(p);
    }
    long std_ns = ns() - start;

    print_result("single size (32 bytes)", my_ns, std_ns);
}

void bench_mixed() {
    vedalloc_reset();
    size_t sizes[] = {8, 16, 32, 64, 128, 256, 512};

    long start = ns();
    for (int i = 0; i < ITER; i++) {
        void *p = vedalloc(sizes[i % 7]);
        vedfree(p);
    }
    long my_ns = ns() - start;

    start = ns();
    for (int i = 0; i < ITER; i++) {
        void *p = malloc(sizes[i % 7]);
        free(p);
    }
    long std_ns = ns() - start;

    print_result("mixed sizes", my_ns, std_ns);
}

void bench_bulk() {
    vedalloc_reset();
    void *ptrs[BULK];

    long start = ns();
    for (int i = 0; i < BULK; i++) ptrs[i] = vedalloc(64);
    for (int i = 0; i < BULK; i++) vedfree(ptrs[i]);
    long my_ns = ns() - start;

    start = ns();
    for (int i = 0; i < BULK; i++) ptrs[i] = malloc(64);
    for (int i = 0; i < BULK; i++) free(ptrs[i]);
    long std_ns = ns() - start;

    print_result("bulk alloc 1000 + free", my_ns, std_ns);
}

void bench_reverse() {
    vedalloc_reset();
    void *ptrs[BULK];

    long start = ns();
    for (int i = 0; i < BULK; i++) ptrs[i] = vedalloc(64);
    for (int i = BULK - 1; i >= 0; i--) vedfree(ptrs[i]);
    long my_ns = ns() - start;

    start = ns();
    for (int i = 0; i < BULK; i++) ptrs[i] = malloc(64);
    for (int i = BULK - 1; i >= 0; i--) free(ptrs[i]);
    long std_ns = ns() - start;

    print_result("reverse free", my_ns, std_ns);
}

void bench_fragment_reuse() {
    vedalloc_reset();
    long start = ns();
    for (int i = 0; i < ITER; i++) {
        void *a = vedalloc(100);
        void *b = vedalloc(200);
        void *c = vedalloc(300);

        vedfree(b);
        void *d = vedalloc(150);

        vedfree(a);
        vedfree(c);
        vedfree(d);
    }
    long my_ns = ns() - start;

    start = ns();
    for (int i = 0; i < ITER; i++) {
        void *a = malloc(100);
        void *b = malloc(200);
        void *c = malloc(300);

        free(b);
        void *d = malloc(150);

        free(a);
        free(c);
        free(d);
    }
    long std_ns = ns() - start;

    print_result("fragment reuse", my_ns, std_ns);
}

void fragmentation_stats() {
    if (!heap_start) return;

    block_header *b = (block_header *)(heap_start + sizeof(heap_header));
    size_t total_free = 0;
    size_t largest = 0;

    while (b) {
        if (!b->in_use) {
            total_free += b->size;
            if (b->size > largest) largest = b->size;
        }
        b = b->next;
    }

    printf("\nFragmentation stats:\n");
    printf("total free memory: %zu bytes\n", total_free);
    printf("largest free block: %zu bytes\n", largest);

    if (total_free > 0)
        printf("external fragmentation: %.2f%%\n",
               100.0 * (1.0 - (double)largest / total_free));
}

void bench_stress() {
    vedalloc_reset();
    void *my_arr[1000] = {0};
    void *std_arr[1000] = {0};

    long start = ns();
    for (int i = 0; i < STRESS_OPS; i++) {
        int idx = rand() % 1000;

        if (my_arr[idx]) {
            vedfree(my_arr[idx]);
            my_arr[idx] = NULL;
        } else {
            my_arr[idx] = vedalloc(rand() % 256 + 1);
        }
    }
    long my_ns = ns() - start;

    start = ns();
    for (int i = 0; i < STRESS_OPS; i++) {
        int idx = rand() % 1000;

        if (std_arr[idx]) {
            free(std_arr[idx]);
            std_arr[idx] = NULL;
        } else {
            std_arr[idx] = malloc(rand() % 256 + 1);
        }
    }
    long std_ns = ns() - start;

    print_result("stress random workload", my_ns, std_ns);
}

int main() {
    bench_single();
    bench_mixed();
    bench_bulk();
    bench_reverse();
    bench_fragment_reuse();
    bench_stress();
    fragmentation_stats();
    return 0;
}