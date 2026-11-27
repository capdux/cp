[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# lru-cache

> **Source Attribution**: Clean-room MIT CP implementation of [`npm:lru-cache`](https://github.com/isaacs/node-lru-cache) (ISC/MIT) | CP Package Version: `1.0.0` | Zero external dependencies

Zero-allocation O(1) industrial-grade LRU cache for CP with an intrusive doubly-linked list, open-addressing FNV-1a 64-bit hash index, nanosecond TTL expiration, byte/custom weight eviction, and runtime hit-rate metrics.

## Zero-Cost Migration (TypeScript / JS → CP)

Keep the exact same package name (`"lru-cache"`) — no renaming required.

| TypeScript / Node.js (`npm:lru-cache`) | CP (`lru-cache`) |
| :--- | :--- |
| `import { LRUCache } from "lru-cache";` | `import { LruCache, LruConfig } from "lru-cache";` |
| `new LRUCache({ max: 128, ttl: 5000 })` | `LruCache.init(LruConfig{ max_entries: 128, default_ttl_ns: 5_000_000_000 })` |
| `cache.set(k, v)` | `cache.put(k, v, now_ns)` |
| `cache.get(k)` / `cache.peek(k)` | `cache.get(k, now_ns)` / `cache.peek(k, now_ns)` |
| `cache.delete(k)` / `cache.purgeStale()` | `cache.remove(k)` / `cache.purgeExpired(now_ns)` |

## API Reference

### Constants

- `MAX_CAPACITY: usize = 256` (maximum inline slot pool entries)
- `HASH_BUCKETS: usize = 512` (open-addressing hash table size, ≤50% load factor)
- `MAX_KEY_LEN: usize = 64` (maximum key length in bytes)
- `MAX_VAL_LEN: usize = 256` (maximum value length in bytes)

### Core Types & Methods

| Symbol / Signature | Description |
| :--- | :--- |
| `LruConfig { max_entries, max_weight, default_ttl_ns }` | Cache config (`max_weight = 0` unlimited, `default_ttl_ns = 0` no TTL). |
| `LruStats { hits, misses, evictions, expirations, len, total_weight }` | Metrics snapshot with `hitRate(): f64`. |
| `LruCache.init(cfg: LruConfig): LruCache` | Initialize cache (`max_entries` clamped to `MAX_CAPACITY`). |
| `put(*_, key: &[]u8, val: &[]u8, now_ns: u64): bool` | Insert/update with default TTL and automatic weight (`key.len + val.len`). |
| `putWithOptions(*_, key, val, ttl_ns: u64, weight: u64, now_ns: u64): bool` | Insert/update with custom TTL and weight (`0` = auto byte weight). |
| `get(*_, key: &[]u8, now_ns: u64): (&[]u8)?` | Fetch & promote to MRU; lazily evicts expired entries and updates stats. |
| `peek(key: &[]u8, now_ns: u64): (&[]u8)?` | Read-only lookup (respects TTL) without changing LRU order or stats. |
| `contains(key: &[]u8, now_ns: u64): bool` | Check existence & non-expiration without mutating order. |
| `remove(*_, key: &[]u8): bool` / `popLru(*_): bool` | Remove specific key / evict least-recently-used tail entry. |
| `purgeExpired(*_, now_ns: u64): usize` | Sweep all expired entries at `now_ns`; returns purged count. |
| `clear(*_)` / `resetStats(*_)` | Remove all entries / zero hit-miss-eviction counters. |
| `len()` / `isEmpty()` / `capacity()` / `totalWeight()` / `stats()` | Query runtime size, capacity, weight, and `LruStats`. |
| `mostRecentKey(): (&[]u8)?` / `leastRecentKey(): (&[]u8)?` | Inspect MRU head key or LRU tail key. |

## Usage Examples

### Basic Capacity LRU & `peek` vs `get`

```cp
import { LruCache, LruConfig } from "lru-cache";

fn demoBasicLru() {
    let cache = LruCache.init(LruConfig{ max_entries: 3 });

    * = cache.put("a", "alpha", 0);
    * = cache.put("b", "beta", 0);
    * = cache.put("c", "gamma", 0);

    // peek does NOT promote "a"
    const p = cache.peek("a", 0);
    * = p;

    // get promotes "a" to MRU; "b" becomes LRU tail
    const v = cache.get("a", 0);
    * = v;

    // Inserting "d" evicts "b"
    * = cache.put("d", "delta", 0);
    // cache.contains("b", 0) == false
    // cache.mostRecentKey().? == "d"
    // cache.leastRecentKey().? == "c"
}
```

### TTL Expiration & Weight-Based Eviction

```cp
import { LruCache, LruConfig } from "lru-cache";

fn demoTtlAndWeight() {
    const SEC: u64 = 1_000_000_000;
    let cache = LruCache.init(LruConfig{
        max_entries: 64,
        max_weight: 100,
        default_ttl_ns: 5 * SEC,
    });

    const t0: u64 = 100 * SEC;
    * = cache.putWithOptions("k1", "payload_1", 2 * SEC, 40, t0);
    * = cache.putWithOptions("k2", "payload_2", 5 * SEC, 40, t0);
    // Exceeds max_weight (100) -> evicts "k1"
    * = cache.putWithOptions("k3", "payload_3", 5 * SEC, 50, t0);

    const purged = cache.purgeExpired(t0 + 6 * SEC);
    const st = cache.stats();
    const rate = st.hitRate();
    * = purged;
    * = rate;
}
```

## Performance & Safety Highlights

- **`undefined` Stack Slot Pool**: `slots: [256]LruEntry = undefined` paired with a bump watermark (`allocated`) and intrusive free-list (`free_head`) avoids zeroing ~90 KB on `init()`.
- **Strict O(1) Intrusive List + Back-Pointer Bucket**: Every `LruEntry` stores `prev`/`next` (`u16`) and its hash `bucket`, enabling O(1) promote, remove, and tail eviction with zero heap allocation.
- **Tombstone-Aware FNV-1a Open Addressing**: 512 buckets with automatic lightweight `rehash()` at 75% occupancy (`REHASH_THRESHOLD = 384`) prevents probe degradation under heavy churn.
- **Inline Ownership & Saturating TTL**: Keys (`≤64 B`) and values (`≤256 B`) are copied into inline buffers (zero dangling slices); expiration uses saturating addition (`now_ns +| ttl_ns`) to prevent `u64` wraparound.

## License

MIT
