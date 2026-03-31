#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

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