# @cap/lru-crate

Clean-room MIT, zero-heap O(1) LRU cache (`LruCacheI64`) for CP — modeled on Rust's `lru` crate.

## Dependencies
- `@cap/fnv-hash` (`1.0.0`) — 64-bit FNV-1a key hashing.

## API
- `LruCacheI64.withCapacity(cap)` (up to `LRU_MAX_CAP = 64`)
- `put`, `get`, `peek`, `pop`, `popLru`, `contains`, `peekLruKey`, `peekMruKey`, `len`, `capacity`, `isEmpty`, `clear`
