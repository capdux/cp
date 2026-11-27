[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# indexmap

> **Source Attribution**: Clean-room MIT CP port of **crates.io** [`indexmap`](https://github.com/indexmap-rs/indexmap) (`v2` / `MIT`), CP package `indexmap@1.0.0`.

Zero-allocation insertion-order-preserving hash map in pure CP. Combines a dense insertion-order entry array with an open-addressing hash index table powered by `@sindresorhus/fnv1a`.

## Why CP `indexmap`?

- **Zero Heap Allocation**: `IndexMapU64` (up to 64 entries, 128 hash buckets) and `IndexMapStr` (up to 32 entries, 64 hash buckets, max 64-byte keys) live completely inline on stack or in structs.
- **Zero-Overhead Stack Initialization**: Entry arrays and temporary key buffers (`[64]IndexMapEntry`, `[32]IndexMapStrEntry`, `[64]u8`) use `undefined` initialization to eliminate redundant stack zeroing.
- **O(1) Lookup + Deterministic Order**: Fast FNV-1a hashing for key lookup, O(1) index access (`getIndex`, `first`, `last`, `pop`), plus both O(1) `swapRemove` and order-preserving `shiftRemove`.

## Zero-Cost Migration (Rust → CP)

Keep the exact same package name (`indexmap`) — no renaming required.

**Rust (`Cargo.toml` / `main.rs`):**
```rust
use indexmap::IndexMap;
let mut m = IndexMap::new();
m.insert("a", 1);
```

**CP (`cap.json` / `main.cp`):**
```typescript
import { IndexMapU64, IndexMapStr } from "indexmap";
```

## Quick Start

### Insertion-Order String Map (`IndexMapStr`)

```typescript
import { IndexMapStr } from "indexmap";

export fn demoStrMap(): i64 {
    let mut m = IndexMapStr.new();
    _ = m.insert("alpha", 10);
    _ = m.insert("beta", 20);
    _ = m.insert("gamma", 30);

    // Update existing key in-place without changing order
    let upd = m.insert("beta", 200);
    // upd.replaced == true, upd.old_val == 20, upd.index == 1

    // Order-preserving removal vs O(1) swap-removal
    let removed = m.shiftRemove("alpha");
    if (removed.found) {
        let first = m.first(); // now "beta" -> 200
        return first.val;
    }
    return 0;
}
```

### Integer Key Map (`IndexMapU64`) & Index Iteration

```typescript
import { IndexMapU64 } from "indexmap";

export fn demoU64Map(): i64 {
    let mut m = IndexMapU64.new();
    _ = m.insert(1001, 5);
    _ = m.insert(1002, 15);

    let mut total: i64 = 0;
    let mut i: usize = 0;
    while (i < m.len()) {
        let e = m.getIndex(i);
        if (e.found) {
            total += e.val;
        }
        i += 1;
    }
    return total; // 20
}
```

## API Reference

Both `IndexMapU64` (capacity 64, key `u64`, val `i64`) and `IndexMapStr` (capacity 32, key `string` ≤ 64B, val `i64`) expose identical methods:

- `new()`, `len(): usize`, `capacity(): usize`, `isEmpty(): bool`, `clear()`
- `insert(key, val: i64): InsertResult` (`{ ok, replaced, old_val, index }`)
- `get(key): GetValResult` (`{ found, val }`)
- `getIndexOf(key): i64` (`>= 0` index or `-1` if missing)
- `containsKey(key): bool`
- `getIndex(idx: usize)` / `first()` / `last()` / `pop()` → `GetEntryResult` (for U64) or `GetStrEntryResult` (for Str, with `keyStr(): &[]u8`)
- `swapRemove(key): GetValResult` — O(1), moves last entry into vacated slot
- `shiftRemove(key): GetValResult` — O(n), preserves insertion order of all remaining entries

## License

MIT
