#define _DEFAULT_SOURCE

#include "malloc.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

const size_t PADDING_ALIGNMENT = 7;

static inline size_t _get_block_size(size_t raw_size) { 
    size_t raw_block_size = raw_size + 2*sizeof(size_t);
    return (raw_block_size + PADDING_ALIGNMENT) & ~ PADDING_ALIGNMENT;
}

static inline size_t _pack(size_t block_size, uint8_t alloc) { return block_size | alloc;}

static inline size_t _get_size(size_t header) { return header & ~PADDING_ALIGNMENT;}

static inline uint8_t _get_alloc(size_t header) { return header & 1u;}

static inline void _set(size_t * ptr, size_t header) { 
    *ptr = header; //set header
    size_t block_size =  _get_size(header);
    *(ptr + (block_size/sizeof(size_t)) - 1) = header; //set footer
    return;
}

void *ma_malloc(size_t size)
{
    if(size == 0){
        return NULL;
    }
    size_t block_size = _get_block_size(size);
    size_t * ptr = sbrk(block_size);
    if(ptr == (void *)-1){
        return NULL;
    }
    _set(ptr, _pack(block_size, 1));
    return (void *)(ptr + 1); //returning start of user space
}

void ma_free(void *ptr)
{
    if(ptr == NULL)
        return;
    size_t * header = (size_t *)ptr - 1;
    size_t block_size = _get_size(*header);
    _set(header, _pack(block_size, 0));
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
