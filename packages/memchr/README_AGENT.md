# memchr — AI Agent Quick Reference

- **Package**: `memchr` (`v1.0.0`) | **Source**: crates.io `memchr` v2 | **Import**: `import { ... } from "memchr";`
- **Role**: Zero-heap 16-lane hardware SIMD (`@Vector(16, u8)`) byte & substring search engine.

## Core Exports (`src/index.d.cp`)

```cp
fn memchr(needle: u8, haystack: &[]u8): i64;
fn memchr2(n1: u8, n2: u8, haystack: &[]u8): i64;
fn memchr3(n1: u8, n2: u8, n3: u8, haystack: &[]u8): i64;
fn memrchr(needle: u8, haystack: &[]u8): i64;
fn memrchr2(n1: u8, n2: u8, haystack: &[]u8): i64;
fn memrchr3(n1: u8, n2: u8, n3: u8, haystack: &[]u8): i64;
fn memchrCount(needle: u8, haystack: &[]u8): usize;
fn memchrFindAll(needle: u8, haystack: &[]u8, out_offsets: []usize): usize;
fn memmemFind(haystack: &[]u8, needle: &[]u8): i64;
fn memmemRfind(haystack: &[]u8, needle: &[]u8): i64;
fn countByte(needle: u8, haystack: &[]u8): usize;
```

## Minimal Usage

```cp
import { memchr, memchr2, memrchr, memmemFind, memchrFindAll } from "memchr";

const buf: []const u8 = "key=val&foo=bar";
const eq = memchr('=' as u8, &buf);                 // 3 (i64, -1 if not found)
const sep = memchr2('&' as u8, '=' as u8, &buf);    // 3
const lastEq = memrchr('=' as u8, &buf);            // 11
const sub = memmemFind(&buf, &"foo");               // 8

let out: [8]usize = undefined;
const n = memchrFindAll('=' as u8, &buf, out[0..8]); // n == 2, out = [3, 11]
```

## Pitfalls & Constraints

1. **Sentinel return `-1`**: All `memchr*`, `memrchr*`, `memmem*` return `i64` (`-1` when not found), NOT `usize?`. Check `>= 0` before casting to `usize`.
2. **Argument order differs**: `memchr(needle, haystack)` takes `needle` first, whereas `memmemFind(haystack, needle)` and `memmemRfind(haystack, needle)` take `haystack` first (matching Rust `memchr` v2).
3. **Output buffer cap**: `memchrFindAll` stops cleanly when `out_offsets` is full; return value is written count `<= out_offsets.len`.
4. **Empty needle in `memmem`**: Returns `0` for `memmemFind` and `haystack.len as i64` for `memmemRfind`.
