# flat — AI Agent Quick Ref

Zero-allocation JSON flattening (a.b.0.c), dot-prop path lookup & flat-to-JSON reconstruction engine (flat / dot-prop parity) for CP (`v1.0.1`, `MIT`)

## Import

```cp
import {
    FlatResult,
    strEq,
    isWs,
    skipWs,
    parseJsonStringEnd,
    formatUsize,
    parseValue,
    flattenJsonWith,
} from "flat";
```

## Core Types & Constants

```cp
class FlatResult { pool: [MAX_FLAT_POOL]u8, pool_len: usize, key_starts: [MAX_FLAT_ENTRIES]usize, key_lens: [MAX_FLAT_ENTRIES]usize, /* +4 fields */ } // methods: empty(...) +5
```

## API Signatures

```cp
fn strEq(a: string, b: string): bool;
fn isWs(c: u8): bool;
fn skipWs(s: string, pos: usize): usize;
fn parseJsonStringEnd(s: string, open_quote: usize): usize;
fn formatUsize(v: usize, buf: []u8): string;
fn parseValue(s: string, start_pos: usize, prefix: []u8, prefix_len: usize, sep: u8, depth: usize, res: &mut FlatResult): usize;
fn flattenJsonWith(json_str: string, sep: u8): FlatResult;
fn flattenJson(json_str: string): FlatResult;
fn getByPath(json_str: string, dot_path: string, out: []u8): string?;
fn hasPath(json_str: string, dot_path: string): bool;
fn appendStr(out: []u8, pos: usize, s: string): usize;
fn formatFlatJsonObject(flat: FlatResult, out: []u8): string;
```

## Minimal Usage

```cp
import { parseJsonStringEnd } from "flat";

pub fn main() {
    let result = parseJsonStringEnd("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"flat"`; install via `cap add flat`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
