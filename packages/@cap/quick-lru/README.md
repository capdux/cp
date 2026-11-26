# @cap/quick-lru

Clean-room MIT, zero-heap two-generation QuickLRU cache (`QuickLruI64`) for CP — modeled on Sindre Sorhus's `quick-lru`.

## Dependencies
- `@cap/fnv-hash` (`1.0.0`) — 64-bit FNV-1a key hashing.

## API
- `QuickLruI64.new(max_entries)` (up to `QLRU_MAX_GEN = 32` per generation)
- `set`, `get`, `peek`, `has`, `delete`, `clear`, `size`, `maxSize`
