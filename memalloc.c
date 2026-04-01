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

// gets allocator header
my_stats *get_malloc_header() {
  assert(heap_start != NULL);
  my_stats *malloc_header = (my_stats *)heap_start;
  assert(malloc_header->magical_bytes == MAGICAL_BYTES);
  return malloc_header;
}

// request 4kb mem n initialize first free block with headers
int *an_malloc(ssize_t size) {
    if (heap_start == NULL) {
      heap_start = sbrk(0); // get current end of heap 
      sbrk(4096);  // 4kb as most linux os page size of mem is 4096
    }

    char *heap_end = sbrk(0);
    long int length = heap_end - heap_start;

    // check if magic byte are at start of heap
    if ((*heap_start) != MAGICAL_BYTES) {
      *(heap_start) = MAGICAL_BYTES;
      my_stats *malloc_header = (my_stats *)heap_start;
      malloc_header->amount_of_blocks = 1;
      malloc_header->amount_of_pages = 1;

      // create first free block
      area *first_block = (area *)((char *)heap_start + sizeof(my_stats));

      first_block->marker = BLOCK_MARKER;
      first_block->in_use = false;
      first_block->length = length - sizeof(my_stats) - sizeof(area);
      first_block->next = NULL;
      first_block->prev = NULL;
    }

    return add_used_block(size);
}

// blocks are created here
int *add_used_block(ssize_t size) {
    my_stats *malloc_header = get_malloc_header();
    while (malloc_header->my_simple_lock) {
      sleep(1);  // primitive lock check
    };
    malloc_header->my_simple_lock = true;

    // skip header n point to first block
    area *block = (area *)((char *)heap_start + sizeof(my_stats));

    // best fit algo 
    area *smallest_block = NULL;
    area *last_block = block;
    while (block != NULL) {
      assert(block->marker == BLOCK_MARKER);
      if ((block->length + sizeof(area)) >= size && block->in_use == false) {
        if (smallest_block == NULL || smallest_block->length > block->length) {
          smallest_block = block;
        }
      }
      last_block = block;
      block = block->next;
    }

    // if no big enough blocks, extend heap
    if (smallest_block == NULL) {
      area *last_block = find_last_block();
      while (last_block->length < size) {
        sbrk(4096);
        last_block->length += 4096;
        malloc_header->amount_of_pages += 1;
      }
      smallest_block = last_block;
    }

    smallest_block->in_use = true;

    // one byte needed for new block's content, ensure it
    int must_have_size = smallest_block->length - size - sizeof(area) - 1;
    if (must_have_size <= 0) {
      sbrk(4096);  // grow heap again
      malloc_header->amount_of_pages += 1;
      last_block->length += 4096;
      must_have_size = smallest_block->length - size - sizeof(area) - 1;
    }

    // create new free block
    int remaining_size = must_have_size + 1;
    malloc_header->amount_of_blocks += 1;
    area *new_block = (area *)((char *)smallest_block + sizeof(area) + size);
    new_block->marker = BLOCK_MARKER;
    new_block->prev = smallest_block;
    new_block->next = smallest_block->next;

    // fix pointer
    if (new_block->next != NULL) {
      (new_block->next)->prev = new_block;
    }

    smallest_block->next = new_block;
    new_block->length = remaining_size;
    smallest_block->length = size;
    malloc_header->my_simple_lock = false;
    
    return (int *)((char *)smallest_block + sizeof(area));
}
