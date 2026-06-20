#define _DEFAULT_SOURCE

#include "malloc.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>

const size_t PADDING_ALIGNMENT = 7;

static inline size_t _get_block_size(size_t raw_size) { return raw_size + 2*sizeof(size_t) + ((raw_size + PADDING_ALIGNMENT) & ~ PADDING_ALIGNMENT);;

static inline size_t _pack(size_t block_size, uint8_t alloc) { return (block_size - 2*(sizeof(size_t))) || alloc;};

static inline size_t _get_size(size_t header) { return header & ~PADDING_ALIGNMENT;};

static inline uint8_t _get_alloc(size_t header) { return header & PADDING_ALIGNMENT;};

static inline void set(size_t * ptr, size_t header) { 
    *ptr = header; //set header
    size_t block_size =  _get_size(header);
    *(ptr + sizeof(size_t) + block_size) = header; //set footer
    return;
}

void *ma_malloc(size_t size)
{
    if(size == 0){
        return NULL;
    }
    size_t block_size = _get_block_size(size);
    size_t * ptr = sbrk(block_size);
    if(ptr == -1){
        return NULL;
    }
    (void)size;
    return NULL;
}

void ma_free(void *ptr)
{
    (void)ptr;
}

void *ma_realloc(void *ptr, size_t size)
{
    (void)ptr;
    (void)size;
    return NULL;
}

void *ma_calloc(size_t nmemb, size_t size)
{
    (void)nmemb;
    (void)size;
    return NULL;
}
