#ifndef VEDALLOC_H
#define VEDALLOC_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

void *vedalloc(size_t size);
bool vedfree(void *ptr);

extern char *heap_start;

typedef struct block_header {
    uint8_t magic;
    bool in_use;
    size_t size;
    struct block_header *prev;
    struct block_header *next;
} block_header;
  
typedef struct heap_header {
    uint8_t magic;
    size_t total_blocks;
    size_t total_pages;
} heap_header;

#define HEAP_MAGIC 0x55
#define BLOCK_MAGIC 0xDD
#define PAGE_SIZE 4096

void vedalloc_reset();

#endif