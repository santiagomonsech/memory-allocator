#ifndef MALLOC_H
#define MALLOC_H

#include <stddef.h>

void *ma_malloc(size_t size);
void  ma_free(void *ptr);
void *ma_realloc(void *ptr, size_t size);
void *ma_calloc(size_t nmemb, size_t size);

#endif /* MALLOC_H */
