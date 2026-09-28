# Changelog

---

## [Unreleased]
- `ma_realloc` and `ma_calloc` not implemented yet (return `NULL`).

---

## [2026-09-08] — M3: Explicit free list completed

### Fixed
- Heap-init check in `_first_fit` was always true, resetting the free list on every call.
- Pointer/int comparison bug in the free-list traversal loop.
- `_compact_block` had no `else` branch — a reused block that didn't need splitting was never unlinked from the free list nor marked allocated, causing two live allocations to alias the same address.
- `_compact_block`'s split branch computed the remainder block's address from an unset global (`start_of_heap`) instead of `header`.
- The first block ever allocated never had its header/footer written (missing `_set()` on the initial `sbrk` branch of `_first_fit`), leaving it with a zeroed header.
- `start_of_heap` was declared but never initialized in the explicit-free-list code path — `_start_of_previous_block` read one word before the actual heap start on the very first `free()`. Undefined behavior, confirmed via a non-deterministic SEGV under AddressSanitizer (passed or failed depending on binary memory layout). Fixed by capturing `sbrk(0)` once, inside `_init_heap()`, before the first block is requested — verified with gdb (`true` for the first block, `false` for any later one).
- `_update_explicit_list` didn't stop traversing after inserting a node — a spurious extra iteration could re-run an insertion branch, orphaning a node from the forward chain and creating an infinite cycle in the backward (`prev`) chain. Fixed with `return` after each of the three insertion branches.
- `sbrk` failure wasn't checked on the heap-initialization branch of `_first_fit` (only on the fallback branch) — could write through an invalid `(void*)-1` pointer instead of returning `NULL`.
- `ma_malloc` didn't check for a `NULL` return from `_first_fit` before computing `ptr + 1`, would silently turn a legitimate allocation failure into a garbage non-NULL pointer instead of `NULL`.

### Changed
  - Free-list insertion ended up **address-ordered** instead of the LIFO planned on 09-01: `free` is O(n) in the number of free blocks instead of O(1), in exchange for better memory utilization (first-fit on an address-ordered list approaches best-fit, CS:APP 9.9.13). LIFO vs. address-ordered comparison deferred to M5's benchmark.
- Removed dead code: the old implicit-free-list `_first_fit`/`_compact_block` (superseded by the explicit-list versions) and leftover debug `printf`s.
- `valgrind` Makefile target now depends on `clean`, matching `asan` — running `make valgrind` right after `make asan` was reusing the ASan-instrumented binary, causing a runtime conflict.

Verified with `make test` + `make asan` + `make valgrind`, all green (0 errors, 0 leaks), plus targeted gdb verification of the `start_of_heap` invariant and a hand-traced scenario for the list-corruption fix.

## [2026-09-03] — M3: Explicit free list (in progress)
- `HEAD`/`TAIL` sentinels implemented as `static size_t[3]` in `.bss`.
- `_first_fit` now traverses the explicit free list instead of scanning the whole heap.
- Fixed a pointer-arithmetic offset bug (`PREV_OFFSET`/`NEXT_OFFSET` defined in bytes but used as `size_t*` element offsets, double scaling caused a segfault) and undersized sentinel arrays (infinite loop).
- Known bug at end of session: freed blocks weren't being correctly reused (address-reuse assert failing) — carried over and fixed in the 09-08 entry above.

## [2026-09-01] — M3: Design (Explicit Free Lists)
- Static sentinel nodes `HEAD`/`TAIL`, kept outside the `sbrk`-managed heap — they never participate in physical coalescing, so they don't need a header/footer.
- Minimum block size raised to `4 * sizeof(size_t)` (header + footer + prev + next pointers).
- Free-list insertion is **address-ordered**,comparison against LIFO-ordered insertion deferred to M5's benchmark.
---

## [2026-06-12] — M1: First approach

### Added
- `ma_malloc(size)`: computes aligned block size, calls `sbrk`, does not set the header nor return the allocated memory starting pointer.
- `ma_free`: still empty
- Inline helpers defined (not yet integrated): `_get_block_size`, `_pack`, `_get_size`, `_get_alloc`, `set`

### Design decisions
- Header and footer store `block_size | alloc_bit` packed into a single `size_t`
- Footer duplicates header to enable O(1) backward traversal during coalescing
- `sbrk` used instead of `mmap` — simpler for learning, grows heap linearly

## [2026-07-08] — M1

- Included boundary tags in memory (header-footer), no reuse of freed blocks yet (always grows via sbrk)
- malloc and free are functional
- basic test cases passing

## [2026-07-13] - M2 Starting

- Working on first-fit approach, missing coalesing the block if a first fit match is found but its size is smaller then the original block (producing seg fault)
- Added function to traverse heap blocks.

## [2026-07-14] - M2 (Partial)
- Fixed case when first-fit matches with a smaller block size and compacting the bigger one.
- Included check for heap buffer overflow.

## [2026-07-21] - M2 Completed
- Coalescing with neighbouring blocks on free — all 4 cases (no free neighbors, only previous, only next, both), using boundary tags to find the previous block in O(1).
- Explicit tests per coalescing case, asserting the merged block's address is reused (not just absence of crash/leak).
