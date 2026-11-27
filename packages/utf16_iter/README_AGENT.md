# utf16_iter — AI Agent Quick Ref

Zero-allocation bidirectional UTF-16 code point iterator, Latin-1 to UTF-16 expander, and UTF-16 to UTF-8 transcoder in CP (`v1.0.0`, `Apache-2.0 OR MIT`)

## Import

```cp
import {
    Utf16Chars,
    Utf16Latin1,
    u16iIsHighSurrogate,
    u16iIsLowSurrogate,
    u16iCombineSurrogates,
    u16iEncodeUtf8,
    countUtf16Chars,
    utf16ToUtf8,
} from "utf16_iter";
```

## Core Types & Constants

```cp
class Utf16Chars { slice: &[]u16, pos: usize, end: usize } // methods: new(...) +4
class Utf16Latin1 { bytes: &[]u8, pos: usize, end: usize } // methods: new(...) +6
```

## API Signatures

```cp
fn u16iIsHighSurrogate(unit: u16): bool;
fn u16iIsLowSurrogate(unit: u16): bool;
fn u16iCombineSurrogates(high: u16, low: u16): u32;
fn u16iEncodeUtf8(cp: u32, out: []u8, offset: usize): usize;
fn countUtf16Chars(slice: &[]u16): usize;
fn utf16ToUtf8(slice: &[]u16, out: []u8): usize;
```

## Minimal Usage

```cp
import { u16iIsHighSurrogate } from "utf16_iter";

pub fn main() {
    let result = u16iIsHighSurrogate(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"utf16_iter"`; install via `cap add utf16_iter`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
