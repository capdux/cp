# bstr — AI Agent Quick Ref

Zero-allocation byte string utilities (Rust bstr v1 parity) powered by memchr SWAR search, line splitting, lossy UTF-8 decoding, and ASCII transforms (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SliceRange,
    bsIsContByte,
    bsUtf8ValidLen,
    bsUtf8InvalidSkip,
    bstrFind,
    bstrRfind,
    bstrFindByte,
    bstrRfindByte,
} from "bstr";
```

## Core Types & Constants

```cp
class SliceRange { start: usize, end: usize } // methods: len(...) +1
```

## API Signatures

```cp
fn bsIsContByte(b: u8): bool;
fn bsUtf8ValidLen(src: &[]u8, i: usize): usize;
fn bsUtf8InvalidSkip(src: &[]u8, i: usize): usize;
fn bstrFind(haystack: &[]u8, needle: &[]u8): i64;
fn bstrRfind(haystack: &[]u8, needle: &[]u8): i64;
fn bstrFindByte(haystack: &[]u8, b: u8): i64;
fn bstrRfindByte(haystack: &[]u8, b: u8): i64;
fn bstrContains(haystack: &[]u8, needle: &[]u8): bool;
fn bstrStartsWith(s: &[]u8, prefix: &[]u8): bool;
fn bstrEndsWith(s: &[]u8, suffix: &[]u8): bool;
fn bstrTrimStart(s: &[]u8): SliceRange;
fn bstrTrimEnd(s: &[]u8): SliceRange;
fn bstrTrim(s: &[]u8): SliceRange;
fn bstrLineCount(s: &[]u8): usize;
fn bstrLineAt(s: &[]u8, idx: usize): SliceRange;
fn bstrReplaceAll(src: &[]u8, pattern: &[]u8, replacement: &[]u8, out: []u8): usize;
fn bstrIsUtf8(src: &[]u8): bool;
fn bstrToUtf8Lossy(src: &[]u8, out: []u8): usize;
fn bstrIsAscii(s: &[]u8): bool;
fn bstrToLowerAscii(src: &[]u8, out: []u8): usize;
fn bstrToUpperAscii(src: &[]u8, out: []u8): usize;
fn bstrEqIgnoreAsciiCase(a: &[]u8, b: &[]u8): bool;
```

## Minimal Usage

```cp
import { bsIsContByte } from "bstr";

pub fn main() {
    let result = bsIsContByte(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bstr"`; install via `cap add bstr`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
