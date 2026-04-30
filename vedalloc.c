#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <string.h> 

#define HEAP_MAGIC 0x55
#define BLOCK_MAGIC 0xDD
#define PAGE_SIZE 4096

typedef struct block_header {
    uint8_t magic;
    bool in_use;
    size_t size;
    struct block_header *prev;
    struct block_header *next;
} block_header;
  
typedef struct heap_header {
    uint8_t magic;
    bool lock;
    size_t total_blocks;
    size_t total_pages;
} heap_header;

char *heap_start = NULL;

block_header *find_last_block();

heap_header *get_heap_header() {
  assert(heap_start != NULL);
  heap_header *hdr = (heap_header *)heap_start;
  assert(hdr->magic == HEAP_MAGIC);
  return hdr;
}

block_header *find_previous_used_block(block_header *ptr) {
  block_header *cur = ptr;
  while (cur->prev != NULL) {
    cur = cur->prev;
    if (cur->in_use) return cur;
  }
  return NULL;
}

// heap shrinking n ensure valid final free block
void reduce_heap_if_possible() {
  block_header *last_block = find_last_block();
  block_header *prev_used_block = find_previous_used_block(last_block);

  // shrink to min 1 page
  if (prev_used_block == NULL) {
    if (last_block->size > PAGE_SIZE) {
      last_block->size = PAGE_SIZE;
    }
    prev_used_block = last_block;
  }

  // calc new end of mem
  void *new_end = (char *)prev_used_block + sizeof(block_header) + prev_used_block->size;
  void *heap_end = sbrk(0);

  heap_header *hdr = get_heap_header();

  // shrink heap page by page
  while (new_end < heap_end - PAGE_SIZE) {
    sbrk(-PAGE_SIZE);
    heap_end = sbrk(0); 
    hdr->total_pages--;
  }

  // handle leftover gap, remove free bytes
  if ((char *)heap_end - (char *)new_end > sizeof(block_header) + 1) {
    block_header *free_block = (block_header *)new_end;

    free_block->magic = BLOCK_MAGIC;
    free_block->in_use = false;
    free_block->prev = prev_used_block;
    free_block->next = NULL;
    free_block->size = (char *)heap_end - (char *)new_end - sizeof(block_header);

    prev_used_block->next = free_block;
  }
}

// free block, clear its memory n attempts to merge other free blocks
bool ved_free(void *ptr) {
  if (!ptr) return false;

  heap_header *hdr = get_heap_header();
  while (hdr->lock) sleep(1);
  hdr->lock = true;

  // go to block header
  block_header *block = (block_header *)((char *)ptr - sizeof(block_header));

  // check if valid block
  if (block->magic != BLOCK_MAGIC) {
    hdr->lock = false;
    return false;
  } 

  block->in_use = false;  // mark block as free n free its mem
  memset(ptr, 0, block->size);

  // forward coalesce
  if (block->next && !block->next->in_use) {
    block_header *not_used_next = block->next;

    block->next = not_used_next->next;
    if (not_used_next->next) not_used_next->next->prev = block;

    block->size += sizeof(block_header) + not_used_next->size;
    memset(not_used_next, 0, sizeof(block_header) + not_used_next->size);

    hdr->total_blocks--;
  }

  // backward coalesce
  if (block->prev && !block->prev->in_use) {
    block_header *prev = block->prev;

    prev->size += sizeof(block_header) + block->size;
    prev->next = block->next;

    if (block->next) block->next->prev = prev;

    block = prev;
    hdr->total_blocks--;
  }

  reduce_heap_if_possible();

  hdr->lock = false;
  return true;
}

block_header *find_last_block() {
  heap_header *hdr = get_heap_header();
  block_header *block = (block_header *)((char *)hdr + sizeof(heap_header));
  while (block->next) {
    block = block->next;
  }
  return block;
}

// blocks are created here
void *add_used_block(size_t size) {
    heap_header *hdr = get_heap_header();

    while (hdr->lock) sleep(1);
    hdr->lock = true;

    // skip header n point to first block
    block_header *block = (block_header *)(heap_start + sizeof(heap_header));

    // best fit algo 
    block_header *best = NULL;
    block_header *last = block;

    while (block) {
      assert(block->magic == BLOCK_MAGIC);

      if (!block->in_use && block->size >= size) {
        if (!best || block->size < best->size) {
          best = block;
        }
      }
      last = block;
      block = block->next;
    }

    // if no big enough blocks, extend heap
    if (!best) {
      last = find_last_block();

      while (last->size < size) {
        sbrk(PAGE_SIZE);
        last->size += PAGE_SIZE;
        hdr->total_pages++;
      }
      best = last;
    }

    best->in_use = true;

    // create new free block
    size_t remaining = best->size - size;

    if (remaining > sizeof(block_header) + 1) {
      block_header *new_block =
          (block_header *)((char *)best + sizeof(block_header) + size);

      new_block->magic = BLOCK_MAGIC;
      new_block->in_use = false;
      new_block->size = remaining - sizeof(block_header);
      new_block->prev = best;
      new_block->next = best->next;

      if (new_block->next)
          new_block->next->prev = new_block;

      best->next = new_block;
      best->size = size;

      hdr->total_blocks++;
  }

  hdr->lock = false;

  return (char *)best + sizeof(block_header);
}

// request 4kb mem n initialize first free block with headers
void *ved_malloc(size_t size) {
    if (!heap_start) {
      heap_start = sbrk(0);
      sbrk(PAGE_SIZE);
    }

    char *heap_end = sbrk(0);
    size_t length = heap_end - heap_start;

    // check if magic byte are at start of heap
    if (*heap_start != HEAP_MAGIC) {
      heap_header *hdr = (heap_header *)heap_start;

      hdr->magic = HEAP_MAGIC;
      hdr->lock = false;
      hdr->total_blocks = 1;
      hdr->total_pages = 1;

      // create first free block
      block_header *first = (block_header *)(heap_start + sizeof(heap_header));

      first->magic = BLOCK_MAGIC;
      first->in_use = false;
      first->size = length - sizeof(heap_header) - sizeof(block_header);
      first->next = NULL;
      first->prev = NULL;
    }

    return add_used_block(size);
}
