# Changelog

---

## [Unreleased]
- M2 first-fit search, coalescing when freeing.

### In progress

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
