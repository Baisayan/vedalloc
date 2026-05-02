#ifndef VEDALLOC_H
#define VEDALLOC_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

void *vedalloc(size_t size);
bool vedfree(void *ptr);

extern char *heap_start;

void vedalloc_reset();

#endif