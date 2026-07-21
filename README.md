# memory-allocator

A malloc/free implementation in C built from scratch over sbrk, without using libc.

---

## Why I built this

I want to understand what actually happens when malloc is called, what interactions occur with the OS and how memory layout is managed when using the heap.

---

## How it works

The memory allocator uses the heap memory itself as the data structure to represent the memory layout and how it's used.
That "data structure" is called an `implicit free list` each element in that list contains the memory space allocated by the user, plus a header and a footer that allows the allocator to traverse memory to allocate, merge, split and free space in the heap space.
When requesting new memory, the allocator should check the implicit free list to check if there is already an available block in the list or if a new block has to be appended at the end, also, when freeing the requested memory, the allocator should flag the memory as usable by another allocation, so somehow it has to flag the block as free with its available size, so that when requesting a new block, then the allocator can check if the previously freed block is a good candidate. As the implicit free lists, does not contain an index that can be used to lookup before traversing the data structure, then a strategy has to be established like "first fit" or "best fit" which will imply different tradeoffs.

### Implicit free list

An implicit free lists is the preferred data structure for heap memory representation, the allocator has to traverse the `implicit free list` each time that an operation (free/alloc/re-alloc) is executed. But the metadata lives inside the list itself, embedded in the header and footer of each block.

### Boundary tags

The allocator won't keep a different structure to manage where each block starts, so to find free blocks or to allocate a new block, then it has to traverse the implicit free list, but the allocator doesn't know what size each block contains, so, to solve this each block has to include some metadata related to the block, that metadata is the size of the block + a flag that indicates if the block is free or not. This allows the allocator to traverse the heap block by block, by reading a fixed size header, which contains that information, so that when a new block is requested, then the allocator can traverse the free lists from the   begining and checking if the block is free and its size, this allows also to merge a recently freed block to its following free block and form a bigger free block, but the limitation is that the traversal is only forward, as it's impossible for a block to know where the header of the previous block is.
For that the footer is also included in the block, this footer is a duplicate of the block header, and as its at the end of the block, then the allocator can also check the previous block is also free and merge with the previous block to form a bigger block see [coalescing section](#Coalescing)

```
+------------------+
| header (8 bytes) |  ← block_size | alloc_bit
+------------------+
| payload          |
| ...              |
+------------------+
| padding          |  ← alignment to 8 bytes (last 3 bits always 0)
+------------------+
| footer (8 bytes) |  ← same as header — enables backward traversal
+------------------+
```

The padding is an additional space so that the entire block is aligned to 8 bytes.

### Coalescing

As mentioned in the previous section, when an user frees the memory requested, then there are 4 possible scenarios, in all four cases, the allocator reads the neighboring blocks' state and size before deciding how to merge. It reaches the next block by adding the current block's size to its start, and the previous block by reading the footer at (current_block_start - FTRP_SIZE).

- No Coalescing: both previous and following block are allocated, so that when block is freed, then its header and footer will set its alloc bit to 0 (free)
- Coalesce with previous: when freed and previous block is free, then both blocks can be merged, the new size has to be calculated, as there is a header and a footer that won't be required.
- Coalesce with following: same as before but with the following block.
- Coalesce with both blocks: this is when the recently freed block is surrounded by free blocks, and a new block can be built by merging the three blocks together.

---

## Design decisions

**Why boundary tags instead of a simpler header-only approach?**
Because when trying to get the start of the previous block, from the start of the current block, you need the size of the previous block, so that you can get to the start of the previous block, by calculating `start_of_current_block - size_of_previous_block`. Now The footer, placed at the end of each block, mirrors the header's content. This means the previous block's size is readable in O(1) from the current block's start: start_of_current_block - FTRP_SIZE..

**Why 8-byte alignment?**
This is to prevent CPU performance penalties and satisfy the hardware constraints of 64-bit systems. By ensuring every free memory chunk begins at an address divisible by 8, a custom malloc or free implementation guarantees that any primitive data type stored there can be accessed efficiently.

---

## Current status

| Milestone | Description | Status |
|-----------|-------------|--------|
| M1 | Boundary tags · `ma_malloc` via `sbrk` · `ma_free` (no coalescing) | ✅ Done |
| M2 | Coalescing (all 4 cases) · first-fit search · block splitting | ✅ Done |
| M3 | Explicit free list — prev/next pointers in payload | ⬜ Planned |
| M4 | Segregated free lists — multiple lists by size class | ⬜ Planned |
| M5 | Benchmark vs libc malloc — throughput + fragmentation | ⬜ Planned |

---

## Build and run

```bash
make           # build
make test      # build + run tests
make asan      # build with AddressSanitizer + run
make valgrind  # run under valgrind
make clean     # remove build artifacts
```

**Requirements:** gcc, valgrind (optional)

---

## Reference

- Bryant & O'Hallaron — *Computer Systems: A Programmer's Perspective*, 3e · Chapter 9.9
