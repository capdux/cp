# properties-reader — AI Agent Quick Ref

Zero-heap Java / Spring Boot / i18n .properties parser and formatter with =/:/space separators, backslash line continuations, comments, and \uXXXX Unicode escapes. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PropEntry,
    EscDecodeResult,
    Properties,
    isLineWs,
    decodeEscapeInto,
    pushByte,
    pushEscaped,
    formatEntry,
} from "properties-reader";
```

## Core Types & Constants

```cp
class PropEntry { key_start: usize, key_len: usize, val_start: usize, val_len: usize }
class EscDecodeResult { next_pos: usize, written: usize }
class Properties { entries: [128]PropEntry, count: usize, val_buf: [2048]u8, buf_used: usize } // methods: parse(...) +4
```

## API Signatures

```cp
fn isLineWs(c: u8): bool;
fn decodeEscapeInto(src: &[]u8, esc_pos: usize, dst: []u8, dst_pos: usize): EscDecodeResult;
fn pushByte(dst: []u8, pos: usize, b: u8): i64;
fn pushEscaped(dst: []u8, pos: usize, ch: u8): i64;
fn formatEntry(key: &[]u8, value: &[]u8, dst: []u8): i64;
```

## Minimal Usage

```cp
import { isLineWs } from "properties-reader";

pub fn main() {
    let result = isLineWs(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"properties-reader"`; install via `cap add properties-reader`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
