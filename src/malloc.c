#define _DEFAULT_SOURCE

#include "malloc.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

const size_t PADDING_ALIGNMENT = 7;
size_t *start_of_heap;

static inline size_t _get_block_size(size_t raw_size)
{
    size_t raw_block_size = raw_size + 2 * sizeof(size_t);
    size_t block_size = (raw_block_size + PADDING_ALIGNMENT) & ~PADDING_ALIGNMENT;
    // avoid heap buffer overflow.
    if (block_size < raw_size)
    {
        return 0;
    }
    return block_size;
}

static inline size_t _pack(size_t block_size, uint8_t alloc) { return block_size | alloc; }

static inline size_t _get_size(size_t header) { return header & ~PADDING_ALIGNMENT; }

static inline uint8_t _get_alloc(size_t header) { return header & 1u; }

static inline void _set(size_t *ptr, size_t header)
{
    *ptr = header; // set header
    size_t block_size = _get_size(header);
    *(ptr + (block_size / sizeof(size_t)) - 1) = header; // set footer
    return;
}

static inline size_t *_start_of_previous_block(size_t *ptr)
{
    if (ptr == start_of_heap)
    {
        return ptr;
    }
    size_t * previous_footer = (size_t *)ptr - 1;
    size_t block_size = _get_size(*previous_footer);
    return ptr - (block_size / sizeof(size_t));
}

static inline size_t *_start_of_next_block(size_t *ptr, size_t *bottom_heap)
{
    if (ptr == bottom_heap)
    {
        return ptr;
    }
    size_t block_size = _get_size(*ptr);
    return ptr + (block_size / sizeof(size_t));
}

// if newly requested block is smaller than original, then split both blocks, else use the orginal block.
static inline void _compact_block(size_t *header, size_t block_size, size_t *bottom_heap)
{
    size_t original_block = _get_size(*header);
    size_t remaining_size = original_block - block_size;
    if (remaining_size > 2 * sizeof(size_t))
    {
        _set(header, _pack(block_size, 1));
        size_t *next_header = _start_of_next_block(header, bottom_heap);
        _set(next_header, _pack(remaining_size, 0));
        return;
    }
    _set(header, _pack(original_block, 1));
}

static size_t *_first_fit(size_t block_size)
{
    size_t *bottom_heap = (size_t *)sbrk(0);
    if (start_of_heap == 0)
    {
        start_of_heap = bottom_heap;
    }
    for (size_t *ptr = start_of_heap; ptr < bottom_heap; ptr = _start_of_next_block(ptr, bottom_heap))
    {
        if (!_get_alloc(*ptr) && _get_size(*ptr) >= block_size)
        {
            _compact_block(ptr, block_size, bottom_heap);
            return ptr;
        }
    }
    return sbrk(block_size);
}

static void _coalesce_block(size_t *ptr)
{
    size_t *header = (size_t *)ptr - 1;
    size_t *bottom_of_heap = (size_t *)sbrk(0);
    size_t *previous_block = _start_of_previous_block((size_t *)header);
    size_t *next_block = _start_of_next_block((size_t *)header, bottom_of_heap);

    size_t * start_header = header;
    size_t  block_size = _get_size(*header);
    if(previous_block != header && !_get_alloc(*previous_block)){
        start_header = previous_block;
        block_size += _get_size(*previous_block);
    }
    if(next_block != bottom_of_heap && !_get_alloc(*next_block)){
        block_size += _get_size(*next_block);
    }
    _set(start_header, _pack(block_size, 0));
}

void *ma_malloc(size_t size)
{
    size_t block_size = _get_block_size(size);
    if (block_size == 0 || size == 0)
    {
        return NULL;
    }
    size_t *ptr = _first_fit(block_size);
    if (ptr == (void *)-1)
    {
        return NULL;
    }
    _set(ptr, _pack(block_size, 1));
    return (void *)(ptr + 1); // returning start of user space
}

void ma_free(void *ptr)
{
    if (ptr == NULL)
        return;
    _coalesce_block(ptr);
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
