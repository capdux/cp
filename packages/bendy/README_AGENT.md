# bendy — AI Agent Quick Ref

BitTorrent BEP-3 deterministic binary Bencode streaming encoder, zero-copy decoder, validator, and dict lookup. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    BencodeIntResult,
    BencodeBytesResult,
    BencodeWriter,
    peekKind,
    decodeInt,
    decodeBytes,
    compareBytes,
    parseValueEnd,
} from "bendy";
```

## Core Types & Constants

```cp
class BencodeIntResult { value: i64, next_pos: usize, ok: bool }
class BencodeBytesResult { start: usize, len: usize, next_pos: usize, ok: bool }
class BencodeWriter { buf: []u8, pos: usize, overflow: bool } // methods: init(...) +9
```

## API Signatures

```cp
fn peekKind(src: &[]u8, pos: usize): u8;
fn decodeInt(src: &[]u8, pos: usize): BencodeIntResult;
fn decodeBytes(src: &[]u8, pos: usize): BencodeBytesResult;
fn compareBytes(a: &[]u8, b: &[]u8): i32;
fn parseValueEnd(src: &[]u8, pos: usize, depth: usize): i64;
fn validate(src: &[]u8): bool;
fn dictLookup(src: &[]u8, key: &[]u8): BencodeBytesResult;
```

## Minimal Usage

```cp
import { decodeInt } from "bendy";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decodeInt(out_buf[0..256], 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bendy"`; install via `cap add bendy`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
