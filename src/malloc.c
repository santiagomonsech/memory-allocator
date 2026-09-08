#define _DEFAULT_SOURCE

#include "malloc.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

const size_t PADDING_ALIGNMENT = 7;
const size_t PREV_OFFSET = 1;
const size_t NEXT_OFFSET = 2;
const size_t MIN_BLOCK_SIZE = 4 * sizeof(size_t);

size_t *start_of_heap;

static size_t HEAD[3];
static size_t TAIL[3];

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
    size_t *previous_footer = (size_t *)ptr - 1;
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

static inline size_t *_next(size_t *ptr)
{
    return (size_t *)*(ptr + NEXT_OFFSET);
}

static inline size_t *_prev(size_t *ptr)
{
    return (size_t *)*(ptr + PREV_OFFSET);
}

static inline void _set_next(size_t *ptr, size_t *next)
{
    *(ptr + NEXT_OFFSET) = (size_t)next;
}

static inline void _set_prev(size_t *ptr, size_t *prev)
{
    *(ptr + PREV_OFFSET) = (size_t)prev;
}

static inline void _compact_block(size_t *header, size_t block_size)
{
    size_t original_block = _get_size(*header);
    size_t remaining_size = original_block - block_size;

    size_t *previous_free_block = _prev(header);
    size_t *next_free_block = _next(header);
    if (remaining_size >= MIN_BLOCK_SIZE)
    {
        _set(header, _pack(block_size, 1));
        size_t *new_free_block = header + (block_size / sizeof(size_t));
        _set(new_free_block, _pack(remaining_size, 0));
        _set_next(new_free_block, _next(header));
        _set_prev(new_free_block, _prev(header));
        _set_next(previous_free_block, new_free_block);
        _set_prev(next_free_block, new_free_block);
    }
    else
    {
        _set(header, _pack(original_block, 1));
        _set_next(previous_free_block, next_free_block);
        _set_prev(next_free_block, previous_free_block);
    }
}

static void _init_heap()
{
    start_of_heap = sbrk(0);
    HEAD[1] = (size_t)NULL;
    HEAD[2] = (size_t)TAIL;
    TAIL[1] = (size_t)HEAD;
    TAIL[2] = (size_t)NULL;
}

static size_t *_first_fit(size_t block_size)
{
    if (*(HEAD + NEXT_OFFSET) == 0 && *(TAIL + PREV_OFFSET) == 0)
    {
        _init_heap();
        size_t *new_block = (size_t *)sbrk(block_size);
        if (new_block == (void *)-1)
        {
            return NULL;
        }
        _set(new_block, _pack(block_size, 1));
        return new_block;
    }
    for (size_t *ptr = _next(HEAD); ptr != TAIL; ptr = _next(ptr))
    {
        if (_get_size(*ptr) >= block_size)
        {
            _compact_block(ptr, block_size);
            return ptr;
        }
    }
    size_t *new_block = (size_t *)sbrk(block_size);
    if (new_block == (void *)-1)
    {
        return NULL;
    }
    _set(new_block, _pack(block_size, 1));
    return new_block;
}

static void _update_explicit_list(size_t *free_block)
{
    if (_next(HEAD) == TAIL)
    {
        _set_next(HEAD, free_block);
        _set_prev(free_block, HEAD);
        _set_next(free_block, TAIL);
        _set_prev(TAIL, free_block);
        return;
    }
    size_t *free_ptr = HEAD;
    while (free_ptr != TAIL)
    {
        size_t *next_ptr = _next((size_t *)free_ptr);
        if (next_ptr > free_block)
        {
            _set_next(free_ptr, free_block);
            _set_prev(free_block, free_ptr);
            _set_next(free_block, next_ptr);
            _set_prev(next_ptr, free_block);
            return;
        }
        else if (next_ptr == free_block)
        {
            _set_next(free_ptr, next_ptr);
            _set_prev(next_ptr, free_ptr);
            return;
        }
        else if (next_ptr == TAIL)
        {
            _set_next(free_ptr, free_block);
            _set_prev(TAIL, free_block);
            _set_next(free_block, TAIL);
            _set_prev(free_block, free_ptr);
            return;
        }
        free_ptr = next_ptr;
    }
    return;
}

static void _coalesce_block(size_t *ptr)
{
    size_t *header = (size_t *)ptr - 1;
    size_t *bottom_of_heap = (size_t *)sbrk(0);
    size_t *previous_block = _start_of_previous_block((size_t *)header);
    size_t *next_block = _start_of_next_block((size_t *)header, bottom_of_heap);

    size_t *start_header = header;
    size_t block_size = _get_size(*header);
    if (previous_block != header && !_get_alloc(*previous_block))
    {
        start_header = previous_block;
        block_size += _get_size(*previous_block);
    }
    if (next_block != bottom_of_heap && !_get_alloc(*next_block))
    {
        block_size += _get_size(*next_block);
    }
    _set(start_header, _pack(block_size, 0));
    _update_explicit_list(start_header);
}

void *ma_malloc(size_t size)
{
    size_t block_size = _get_block_size(size);
    if (block_size == 0 || size == 0)
    {
        return NULL;
    }

    size_t *ptr = _first_fit(block_size);
    if (ptr == NULL)
    {
        return NULL;
    }
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
