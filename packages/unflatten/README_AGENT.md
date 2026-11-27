# unflatten — AI Agent Quick Ref

Zero-allocation nested JSON flatten & unflatten codec (npm `flat`/`unflatten` parity, fixed-capacity stack tables/pools).

## Import

```cp
import {
    FlatOptions,
    FlatTable,
    flattenJson,
    flattenToJson,
    unflattenJson,
} from "unflatten";
```

## Core API Signatures

```cp
// Options & Table
class FlatOptions { delimiter: u8, max_depth: usize, safe: bool }
FlatOptions.defaults(): FlatOptions // delimiter=46 ('.'), max_depth=0, safe=false

class FlatEntry { key: [96]u8, key_len: usize, val: [128]u8, val_len: usize }
class FlatTable { entries: [48]FlatEntry, len: usize }
FlatTable.new(): FlatTable
table.set(key: string, raw_json_val: string): bool
table.get(key: string, out: []u8): i64
table.count(): usize

// Main Operations (return >= 0 count/bytes, or < 0 on parse/capacity error)
flattenJson(json_in: string, opts: FlatOptions, out_table: &mut FlatTable): i64
flattenToJson(json_in: string, opts: FlatOptions, out: []u8): i64
unflattenJson(flat_json: string, opts: FlatOptions, out: []u8): i64

// Tree Pool (internal/advanced)
class UfPool { nodes: [96]UfNode, count: usize }
ufInsertPath(pool: &mut UfPool, path: string, delim: u8, raw_val: string): bool
ufIsSequentialArray(pool: &mut UfPool, node_idx: i32): bool
ufEmitNode(pool: &mut UfPool, node_idx: i32, out: []u8, pos: usize, is_root: bool): i64
```

## Minimal Example

```cp
import { FlatOptions, flattenToJson, unflattenJson } from "unflatten";

fn demo() {
    let opts = FlatOptions.defaults();
    let flat_buf: [256]u8 = undefined;
    let flen = flattenToJson("{\"a\":{\"b\":[1,2]}}", opts, flat_buf[0..256]);
    if (flen >= 0) {
        let out_buf: [256]u8 = undefined;
        let ulen = unflattenJson(flat_buf[0..(flen as usize)], opts, out_buf[0..256]);
        // ulen >= 0 -> "{\"a\":{\"b\":[1,2]}}"
    }
}
```

## Gotchas & Borrow Rules

- **Fixed limits**: `FlatTable` max 48 entries (key ≤ 96B, val ≤ 128B); `UfPool` max 96 nodes (segment ≤ 48B). Exceeding limits returns `< 0` or `false`.
- **Mutable borrow**: `flattenJson` requires `out_table: &mut FlatTable`; declare `let mut table = FlatTable.new();` and pass `&mut table`.
- **Writable slice**: `out: []u8` in `flattenToJson` / `unflattenJson` / `table.get` must be a writable slice (e.g. `buf[0..N]`), never `&[]u8`.
- **Delimiter fallback**: `opts.delimiter == 0` defaults to `46` (`'.'`).
- **Safe mode**: `opts.safe = true` keeps nested arrays as raw JSON values instead of flattening to `.0`, `.1`.
- **Negative returns**: Always check `res >= 0` before casting `res as usize` to slice `out[0..(res as usize)]`.
