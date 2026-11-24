# Changelog

---

## [Unreleased]
- `ma_realloc` and `ma_calloc` not implemented yet (return `NULL`).

---

  ## [0.4.0] - 2026-11-24
  ## Added
  - User configuration for fit strategy (first fit vs best fit at this time).
  - Best-fit strategy for allocating a new block.

  ## [0.3.0] - 2026-10-14
  ### Added
  - (explicit free list: sentinels, prev/next in the payload, minimum block size)
  ### Changed
  - (first-fit now walks the free list instead of the whole heap; address-ordered insertion and its O(n) free cost)

  ## [0.2.0] - 2026-07-21
  ### Added
  - (first-fit, splitting, coalescing of the 4 cases)
  ### Fixed
  - (only if a 0.1.0 bug was fixed here)

  ## [0.1.0] - 2026-07-08
  ### Added
  - (boundary tags, ma_malloc via sbrk, ma_free without coalescing)