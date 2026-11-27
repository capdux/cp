# indexmap (AI Agent Quick Ref)

- **Import**: `import { IndexMapU64, IndexMapStr, InsertResult, GetValResult, GetEntryResult, GetStrEntryResult } from "indexmap";`
- **Source**: `crates.io:indexmap` v2 (MIT, zero-alloc, FNV-1a via `@sindresorhus/fnv1a`, `undefined` entry arrays).

## Core Signatures

```cp
export class InsertResult { ok: bool, replaced: bool, old_val: i64, index: usize }
export class GetValResult { found: bool, val: i64 }
export class GetEntryResult { found: bool, key: u64, val: i64 }
export class GetStrEntryResult {
    found: bool, key_buf: [64]u8, key_len: usize, val: i64,
    fn keyStr(): &[]u8;
}

export class IndexMapU64 { // cap = 64
    fn new(): IndexMapU64;
    fn len(): usize; fn capacity(): usize; fn isEmpty(): bool; fn clear(_: auto);
    fn getIndexOf(key: u64): i64; fn containsKey(key: u64): bool;
    fn get(key: u64): GetValResult;
    fn getIndex(idx: usize): GetEntryResult;
    fn first(): GetEntryResult; fn last(): GetEntryResult;
    fn insert(_: auto, key: u64, val: i64): InsertResult;
    fn swapRemove(_: auto, key: u64): GetValResult;
    fn shiftRemove(_: auto, key: u64): GetValResult;
    fn pop(_: auto): GetEntryResult;
}

export class IndexMapStr { // cap = 32, max key len = 64 bytes
    fn new(): IndexMapStr;
    fn len(): usize; fn capacity(): usize; fn isEmpty(): bool; fn clear(_: auto);
    fn getIndexOf(key: string): i64; fn containsKey(key: string): bool;
    fn get(key: string): GetValResult;
    fn getIndex(idx: usize): GetStrEntryResult;
    fn first(): GetStrEntryResult; fn last(): GetStrEntryResult;
    fn insert(_: auto, key: string, val: i64): InsertResult;
    fn swapRemove(_: auto, key: string): GetValResult;
    fn shiftRemove(_: auto, key: string): GetValResult;
    fn pop(_: auto): GetStrEntryResult;
}
```

## Minimal Pattern

```cp
let mut m = IndexMapStr.new();
let r = m.insert("k1", 100);
if (r.ok) {
    let v = m.get("k1"); // v.found == true, v.val == 100
}
```

## Pitfalls

1. **Fixed capacities**: `IndexMapU64` max 64 entries; `IndexMapStr` max 32 entries and key length `<= 64` bytes. Exceeding either returns `InsertResult { ok: false, ... }`.
2. **`swapRemove` vs `shiftRemove`**: `swapRemove` is O(1) but moves the last element into the removed slot (reordering); use `shiftRemove` when insertion order must stay intact.
3. **`getIndexOf` returns `i64`**: Returns `-1` when absent; check `>= 0` before casting to `usize`.
4. **Mutable map**: Use `let mut m = ...` for `insert`, `swapRemove`, `shiftRemove`, `pop`, `clear`.
