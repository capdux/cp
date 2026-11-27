# lru-cache — AI Agent Quick Ref

Zero-allocation O(1) LRU cache with intrusive doubly-linked list, open-addressing FNV-1a hash index, TTL expiration, and byte-weight eviction for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    LruConfig,
    LruStats,
    LruEntry,
    LruCache,
    fnv1a64,
    copyBytes,
} from "lru-cache";
```

## Core Types & Constants

```cp
class LruConfig { max_entries: usize, max_weight: u64, default_ttl_ns: u64 }
class LruStats { hits: u64, misses: u64, evictions: u64, expirations: u64, /* +2 fields */ } // methods: hitRate(...)
class LruEntry { key_buf: [MAX_KEY_LEN]u8, val_buf: [MAX_VAL_LEN]u8, key_len: u16, val_len: u16, /* +7 fields */ }
class LruCache { slots: [MAX_CAPACITY]LruEntry, buckets: [HASH_BUCKETS]u16, max_entries: usize, max_weight: u64, /* +12 fields */ } // methods: init(...) +27
```

## API Signatures

```cp
fn fnv1a64(data: &[]u8): u64;
fn copyBytes(dst: []u8, src: &[]u8);
```

## Minimal Usage

```cp
import { fnv1a64 } from "lru-cache";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = fnv1a64(out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"lru-cache"`; install via `cap add lru-cache`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
