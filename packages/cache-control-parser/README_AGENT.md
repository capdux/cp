# cache-control-parser — AI Agent Quick Ref

Zero-allocation RFC 9111 HTTP Cache-Control parser, formatter, and shared/private TTL calculator (cache-control-parser / http-cache-semantics parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CacheControl,
    unquote,
    parseNonNegI64,
    writeStrChecked,
    writeU64Checked,
    appendDirectiveFlag,
    appendDirectiveKv,
} from "cache-control-parser";
```

## Core Types & Constants

```cp
class CacheControl { max_age: i64, s_maxage: i64, stale_while_revalidate: i64, stale_if_error: i64, /* +7 fields */ } // methods: parse(...) +3
```

## API Signatures

```cp
fn unquote(s_in: &[]u8): &[]u8;
fn parseNonNegI64(s_in: &[]u8): i64;
fn writeStrChecked(dst: []u8, pos: usize, s: string): i64;
fn writeU64Checked(dst: []u8, pos: usize, v_in: u64): i64;
fn appendDirectiveFlag(dst: []u8, pos_in: usize, written: &mut usize, name: string): i64;
fn appendDirectiveKv(dst: []u8, pos_in: usize, written: &mut usize, name: string, val: i64): i64;
```

## Minimal Usage

```cp
import { writeStrChecked } from "cache-control-parser";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = writeStrChecked(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"cache-control-parser"`; install via `cap add cache-control-parser`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
