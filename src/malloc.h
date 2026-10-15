#ifndef MALLOC_H
#define MALLOC_H

#include <stddef.h>

void *ma_malloc(size_t size);
void  ma_free(void *ptr);
void *ma_realloc(void *ptr, size_t size);
void *ma_calloc(size_t nmemb, size_t size);

void ma_print_free_list(void);   // debug
long ma_free_list_len(void);     // debug: node count, -1 if the list is inconsistent

#endif /* MALLOC_H */
