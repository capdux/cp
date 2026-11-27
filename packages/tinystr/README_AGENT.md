# tinystr — AI Agent Quick Ref

Zero-allocation stack-inline fixed-capacity ASCII strings (TinyAsciiStr4, TinyAsciiStr8, TinyAsciiStr16) with u32/u64 packing and BCP-47 case transforms (ICU4X tinystr parity) in pure CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    TinyAsciiStr4,
    TinyAsciiStr4Result,
    TinyAsciiStr8,
    TinyAsciiStr8Result,
    TinyAsciiStr16,
    TinyAsciiStr16Result,
    tsIsLower,
    tsIsUpper,
} from "tinystr";
```

## Core Types & Constants

```cp
class TinyAsciiStr4 { bytes: [4]u8 } // methods: empty(...) +19
class TinyAsciiStr4Result { ok: bool, value: TinyAsciiStr4, err: i64 }
class TinyAsciiStr8 { bytes: [8]u8 } // methods: empty(...) +19
class TinyAsciiStr8Result { ok: bool, value: TinyAsciiStr8, err: i64 }
class TinyAsciiStr16 { bytes: [16]u8 } // methods: empty(...) +17
class TinyAsciiStr16Result { ok: bool, value: TinyAsciiStr16, err: i64 }
```

## API Signatures

```cp
fn tsIsLower(b: u8): bool;
fn tsIsUpper(b: u8): bool;
fn tsValidateBytes(src: &[]u8, max_len: usize): i64;
fn tsValidatePacked(buf: &[]u8): i64;
fn tsSliceLen(buf: &[]u8): usize;
fn tsCopyOut(src: &[]u8, n: usize, out: []u8): usize;
fn tsSliceCmp(a: &[]u8, b: &[]u8): i32;
fn tsAllAlpha(buf: &[]u8, n: usize): bool;
fn tsAllAlnum(buf: &[]u8, n: usize): bool;
fn tsAllDigit(buf: &[]u8, n: usize): bool;
fn tsAllLower(buf: &[]u8, n: usize): bool;
fn tsAllUpper(buf: &[]u8, n: usize): bool;
```

## Minimal Usage

```cp
import { tsIsLower } from "tinystr";

pub fn main() {
    let result = tsIsLower(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"tinystr"`; install via `cap add tinystr`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
