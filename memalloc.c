#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

typedef struct free_area {
    uint8_t marker;
    struct free_area *prev;
    bool in_use;
    uint32_t length;
    struct free_area *next;
} area;
  
typedef struct stats {
    int magical_bytes;
    bool my_simple_lock;
    uint32_t amount_of_blocks;
    uint16_t amount_of_pages;
} my_stats;

const int MAGICAL_BYTES = 0x55; // header marker 
const int BLOCK_MARKER = 0xDD;  // block marker

char *heap_start = NULL;

int *an_malloc(ssize_t size) {
    if (heap_start == NULL) {
      heap_start = sbrk(0);
      sbrk(4096);
    }

    char *heap_end = sbrk(0);
    long int length = heap_end - heap_start;

    // check if magic byte are at start of heap
    if ((*heap_start) != MAGICAL_BYTES) {
      // first execution of malloc
      *(heap_start) = MAGICAL_BYTES;
      my_stats *malloc_header = (my_stats *)heap_start;
      malloc_header->amount_of_blocks = 1;
      malloc_header->amount_of_pages = 1;
      area *first_block = (area *)((char *)heap_start + sizeof(my_stats));
      first_block->marker = BLOCK_MARKER;
      first_block->in_use = false;
      first_block->length = length - sizeof(my_stats) - sizeof(area);
      first_block->next = NULL;
      first_block->prev = NULL;
    }

    return add_used_block(size);
  }
