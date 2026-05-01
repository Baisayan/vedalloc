#include <stdio.h>
#include <assert.h>
#include "vedalloc.h"

void validate_heap() {
    heap_header *hdr = (heap_header *)heap_start;
    assert(hdr->magic == HEAP_MAGIC);
    block_header *block = (block_header *)(heap_start + sizeof(heap_header));
    size_t count = 0;

    while (block) {
        assert(block->magic == BLOCK_MAGIC);
        if (block->next) {
            assert(block->next->prev == block);
        }

        if (!block->in_use && block->next) {
            assert(block->next->in_use == true);
        }
        assert(block->size > 0);

        count++;
        block = block->next;
    }

    assert(count == hdr->total_blocks);
}

void test_basic_alloc_free() {
    void *p = vedalloc(32);
    assert(p);
    validate_heap();

    assert(vedfree(p));
    validate_heap();
}

void test_split() {
    void *p1 = vedalloc(200);
    void *p2 = vedalloc(100);

    assert(p1 && p2);
    validate_heap();

    vedfree(p1);
    validate_heap();
}

void test_forward_coalesce() {
    void *a = vedalloc(100);
    void *b = vedalloc(100);

    vedfree(a);
    vedfree(b);

    validate_heap();
}

void test_backward_coalesce() {
    void *a = vedalloc(100);
    void *b = vedalloc(100);

    vedfree(b);
    vedfree(a);

    validate_heap();
}

void test_reuse() {
    void *a = vedalloc(128);
    vedfree(a);
    void *b = vedalloc(64);
    assert(b == a);
    validate_heap();
}

void test_heap_growth() {
    void *a = vedalloc(5000);
    assert(a);
    validate_heap();
}

void test_multiple_allocs() {
    void *arr[50];

    for (int i = 0; i < 50; i++) {
        arr[i] = vedalloc(32 + i);
        assert(arr[i] != NULL);
    }

    validate_heap();

    for (int i = 0; i < 50; i++) {
        vedfree(arr[i]);
    }

    validate_heap();
}

void test_invalid_free() {
    int x;
    assert(vedfree(&x) == false);
}

void test_double_free() {
    void *p = vedalloc(64);
    vedfree(p);
    assert(vedfree(p) == false);
}

void test_fragment_reuse() {
    void *a = vedalloc(100);
    void *b = vedalloc(200);
    void *c = vedalloc(300);

    vedfree(b);

    void *d = vedalloc(150);
    assert(d != NULL);
    assert(a != NULL);
    assert(c != NULL);

    validate_heap();
}

void test_zero_alloc() {
    void *p = vedalloc(0);
    assert(p != NULL || p == NULL); // observe behavior
}

void test_small_alloc() {
    void *p = vedalloc(1);
    assert(p);
    validate_heap();
}

void test_shrink() {
    void *p = vedalloc(8000);
    vedfree(p);
    validate_heap();
}

int main() {
    printf("Running tests...\n");

    test_basic_alloc_free();
    test_split();
    test_forward_coalesce();
    test_backward_coalesce();
    test_reuse();
    test_heap_growth();
    test_multiple_allocs();
    test_invalid_free();
    test_double_free();
    test_fragment_reuse();
    test_zero_alloc();
    test_small_alloc();
    test_shrink();

    printf("All tests passed.\n");
    return 0;
}