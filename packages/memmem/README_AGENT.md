# memmem — AI Agent Quick Ref

Zero-allocation byte-substring search engine with precompiled BMH + KMP Finder, rfind, and non-overlapping count (Rust memchr::memmem parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Finder,
    findLongNeedle,
    find,
    rfind,
    contains,
    count,
} from "memmem";
```

## Core Types & Constants

```cp
class Finder { needle_buf: [MAX_NEEDLE]u8, needle_len: usize, bad_char: [256]usize, lps: [MAX_NEEDLE]u16, /* +1 fields */ } // methods: init(...) +5
```

## API Signatures

```cp
fn findLongNeedle(haystack: &[]u8, needle: &[]u8): i64;
fn find(haystack: &[]u8, needle: &[]u8): i64;
fn rfind(haystack: &[]u8, needle: &[]u8): i64;
fn contains(haystack: &[]u8, needle: &[]u8): bool;
fn count(haystack: &[]u8, needle: &[]u8): usize;
```

## Minimal Usage

```cp
import { findLongNeedle } from "memmem";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = findLongNeedle(out_buf[0..256], buf_1[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"memmem"`; install via `cap add memmem`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
