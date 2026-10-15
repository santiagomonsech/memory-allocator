# memory-allocator

A malloc/free implementation in C built from scratch over sbrk, without using libc.

---

## Why I built this

I want to understand what actually happens when malloc is called, what interactions occur with the OS and how memory layout is managed when using the heap.

---

## How it works

The memory allocator uses the heap memory itself as the data structure to represent the memory layout and how it's used.
The allocator keeps an explicit free lists, a doubly linked list of only the free blocks, with prev and next pointers stored inside each free block's unused payload. 
The allocator walks this list using first-fit approach to decide next free block to use, skipping allocated blocks entirely.
This list is kept sorted by address, bounded by the HEAD and TAIL sentinels to match the start and end of the list.
When requesting new memory, the allocator should check walks the explicit free list to check if there is already an available block in the list or if a new block has to be appended at the end, also, when freeing the requested memory, the allocator should flag the memory as usable by another allocation and link this newly freed block with the previous and next free blocks. The allocator can check if the previously freed block is a good candidate.

### Implicit free list

An implicit free lists is a structure for heap memory representation, the allocator has to traverse the `implicit free list` each time that an operation (free/alloc/re-alloc) is executed. But the metadata lives inside the list itself, embedded in the header and footer of each block.
Updated: This is no longer the default technique to decide the next free block to allocate, as it was replaced by the use of explicit free lists (M3).

### Boundary tags

The allocator won't keep a different structure to manage where each block starts, so to find free blocks or to allocate a new block, then it has to traverse the implicit free list, but the allocator doesn't know what size each block contains, so, to solve this each block has to include some metadata related to the block, that metadata is the size of the block + a flag that indicates if the block is free or not. This allows the allocator to traverse the heap block by block, by reading a fixed size header, which contains that information, so that when a new block is requested, then the allocator can traverse the free lists from the   begining and checking if the block is free and its size, this allows also to merge a recently freed block to its following free block and form a bigger free block, but the limitation is that the traversal is only forward, as it's impossible for a block to know where the header of the previous block is.
For that the footer is also included in the block, this footer is a duplicate of the block header, and as its at the end of the block, then the allocator can also check the previous block is also free and merge with the previous block to form a bigger block see [coalescing section](#Coalescing)

When a block is allocated
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

When a block is free
```
+------------------+
| header (8 bytes) |  ← block_size | alloc_bit
+------------------+
| prev free block  |  ← payload space as is unused when block is free, it will be used for pointers in the  
| next free block  |    explicit free list
| (payload unused  |
| space)           |
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

### Explicit lists

The implicit list has to walk through every block in the heap to find a free one, in the worst case, that means touching every block just to serve a single `malloc`. Explicit lists fix this by linking only the *free* blocks together, so allocation only ever has to traverse the free blocks, never the whole heap.

This doesn't require a new data structure: the `prev`/`next` pointers live inside the payload of the free block itself. Since that space isn't in use while the block is free, no extra memory is reserved for it but it does raise the minimum block size, from `2*sizeof(size_t)` (header + footer only) to `4*sizeof(size_t)` (header + footer + prev + next), and it adds the logic needed to keep those references consistent whenever a block is freed or reused. A small cost for a real performance win.

The new layout of a free block:

```
+------------------+
| header (8 bytes) |  ← block_size | alloc_bit
+------------------+
| prev             |  ← previous free block pointer
+------------------+
| next             |  ← next free block pointer
+------------------+
| footer (8 bytes) |  ← same as header — enables backward traversal
+------------------+
```


---

## Design decisions

**Why boundary tags instead of a simpler header-only approach?**
Because when trying to get the start of the previous block, from the start of the current block, you need the size of the previous block, so that you can get to the start of the previous block, by calculating `start_of_current_block - size_of_previous_block`. Now The footer, placed at the end of each block, mirrors the header's content. This means the previous block's size is readable in O(1) from the current block's start: start_of_current_block - FTRP_SIZE..

**Why 8-byte alignment?**
This is to prevent CPU performance penalties and satisfy the hardware constraints of 64-bit systems. By ensuring every free memory chunk begins at an address divisible by 8, a custom malloc or free implementation guarantees that any primitive data type stored there can be accessed efficiently.

**Why first-fit instead of best-fit?**
Mainly simplicity of implementation, first-fit stops searching as soon as it finds a candidate block, while best-fit needs to traverse the entire free list to guarantee the tightest fit. Per CS:APP 9.9.6, first-fit tends to have throughput comparable to best-fit in practice, despite best-fit giving better worst-case external fragmentation guarantees. I haven't measured that trade-off for this allocator yet, that's exactly what M5 (benchmark vs libc malloc, throughput + fragmentation) is for, instead of assuming it.

**Why static sentinels (`HEAD`/`TAIL`) instead of `NULL`-terminated pointers?**
`HEAD` and `TAIL` live outside the `sbrk`-managed heap (static globals), so they never take part in physical coalescing and don't need a real header/footer, but every real free block still expects to read a `prev`/`next` at the same fixed offsets. Giving the sentinels the same shape as a real block (padding out the header slot they don't use) means the insertion/removal code never has to special case "is this a sentinel or a real block?"one code path handles both.

**Why address ordered free list instead of LIFO**
An arbitrary choice, but i'm aware that LIFO is O(1) vs address-ordered which is O(N). The benefit, per CS:APP 9.9.13, is better memory utilization: first-fit on an address-ordered list approaches best-fit. M5 will implement LIFO and measure both.

---

## Current status

| Milestone | Description | Status |
|-----------|-------------|--------|
| M1 | Boundary tags · `ma_malloc` via `sbrk` · `ma_free` (no coalescing) | ✅ Done |
| M2 | Coalescing (all 4 cases) · first-fit search · block splitting | ✅ Done |
| M3 | Explicit free list — prev/next pointers in payload | ✅ Done |
| M4 | Segregated free lists — multiple lists by size class | ⬜ Planned |
| M5 | Benchmark vs libc malloc — throughput + fragmentation | ⬜ Planned |

---

## Testing

Beyond "does it crash" — the coalescing tests assert **address reuse**, not just absence of a crash:

- **Merge with next**: after freeing a block and coalescing with the following one, `ma_malloc` for a block that fits returns the *same* address the freed block had — the merge extends forward, the start of the block never moves.
- **Merge with previous / both neighbors**: the resulting address is *earlier* than the just-freed block's — the merge absorbs the earlier neighbor, so the block that "wins" starts where the previous block used to.

Tests use allocated "wall" blocks that are never freed to isolate each coalescing scenario from leftover free blocks contaminating the next test.

The whole suite runs clean under `make asan` (AddressSanitizer) and `make valgrind`.

A code review on the M2 implementation caught 3 real bugs before merge:

- **Integer overflow in `_get_block_size`** — fixed with a sentinel that returns 0 when `block_size < raw_size` (overflow detected), consumed by `ma_malloc`.
- **Redundant `sbrk(0)` per loop iteration** in `_first_fit` / `_start_of_next_block` — fixed by caching `bottom_heap` once and passing it as a parameter instead of querying the OS on every iteration.
- **Test coverage gap** — no test asserted that a freed block actually got reused; only that nothing crashed.

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
