#ifndef VEDALLOC_V2_H
#define VEDALLOC_V2_H

#include <stddef.h>

void *vedalloc_v2(size_t size);
void vedfree_v2(void *ptr);

#endif