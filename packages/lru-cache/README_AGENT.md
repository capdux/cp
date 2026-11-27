# lru-cache@1.0.0 — AI Agent Quick Ref

- **Source**: `npm:lru-cache` (`isaacs/node-lru-cache`, ISC/MIT) | **Deps**: none
- **Import**: `import { LruCache, LruConfig, LruStats, MAX_CAPACITY, MAX_KEY_LEN, MAX_VAL_LEN } from "lru-cache";`
- **Full Types**: `packages/lru-cache/src/index.d.cp`

## Constants & Limits
```cp
MAX_CAPACITY: usize = 256
HASH_BUCKETS: usize = 512
MAX_KEY_LEN:  usize = 64
MAX_VAL_LEN:  usize = 256
```

## Core Types & Methods
```cp
class LruConfig { max_entries: usize, max_weight: u64, default_ttl_ns: u64 }
class LruStats  { hits: u64, misses: u64, evictions: u64, expirations: u64, len: usize, total_weight: u64; fn hitRate(): f64 }

class LruCache {
    fn init(cfg: LruConfig): LruCache
    fn put(_: auto, key: &[]u8, val: &[]u8, now_ns: u64): bool
    fn putWithOptions(_: auto, key: &[]u8, val: &[]u8, ttl_ns: u64, weight: u64, now_ns: u64): bool
    fn get(_: auto, key: &[]u8, now_ns: u64): (&[]u8)?
    fn peek(key: &[]u8, now_ns: u64): (&[]u8)?
    fn contains(key: &[]u8, now_ns: u64): bool
    fn remove(_: auto, key: &[]u8): bool
    fn popLru(_: auto): bool
    fn purgeExpired(_: auto, now_ns: u64): usize
    fn clear(_: auto)
    fn resetStats(_: auto)
    fn len(): usize; fn isEmpty(): bool; fn capacity(): usize; fn totalWeight(): u64
    fn stats(): LruStats
    fn mostRecentKey(): (&[]u8)?; fn leastRecentKey(): (&[]u8)?
}
```

## Minimal Pattern
```cp
import { LruCache, LruConfig } from "lru-cache";

let cache = LruCache.init(LruConfig{ max_entries: 64, max_weight: 0, default_ttl_ns: 0 });
* = cache.put("user:1", "alice", 0);
const val = cache.get("user:1", 0); // (&[]u8)?
```

## Pitfalls & Notes
1. **Length Bounds**: `put`/`putWithOptions` returns `false` if `key.len == 0`, `key.len > 64`, `val.len > 256`, `max_entries == 0`, or `weight > max_weight` (when `max_weight > 0`).
2. **Pass `now_ns` Explicitly**: No hidden syscall clock; pass `0` if TTL is disabled (`default_ttl_ns == 0`).
3. **Borrowed Slice Lifetime**: Slices returned by `get`/`peek`/`mostRecentKey`/`leastRecentKey` borrow from `cache.slots`; copy out before mutating/evicting the same slot.
4. **Mutable Receiver**: Declare `let cache = LruCache.init(...)` (mutable) for `put`, `get`, `remove`, `popLru`, `purgeExpired`, `clear`, `resetStats`.
5. **Stack Size**: `LruCache` holds `[256]LruEntry` (~90 KB) inline (`undefined`-initialized); avoid copying by value in tight loops.
