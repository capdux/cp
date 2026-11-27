# utf-8 — AI Agent Quick Ref

Zero-copy incremental streaming UTF-8 decoder and RFC 3629 validator with Unicode Maximal Subpart lossy recovery in pure CP (`v1.0.0`, `MIT OR Apache-2.0`)

## Import

```cp
import {
    Utf8DecodeStatus,
    Utf8ChunkResult,
    LossyUtf8Decoder,
    u8dExpectedLen,
    u8dIsValidSecond,
    u8dIsValidCont,
    u8dWriteSlice,
    u8dWriteReplacement,
} from "utf-8";
```

## Core Types & Constants

```cp
enum Utf8DecodeStatus { Ok, InvalidSequence, Incomplete }
class Utf8ChunkResult { status: Utf8DecodeStatus, valid_prefix: string, consumed: usize, error_len: usize }
class LossyUtf8Decoder { pending: [4]u8, pending_len: usize } // methods: new(...) +5
```

## API Signatures

```cp
fn u8dExpectedLen(b0: u8): usize;
fn u8dIsValidSecond(b0: u8, b1: u8): bool;
fn u8dIsValidCont(b0: u8, idx: usize, b: u8): bool;
fn u8dWriteSlice(out: []u8, pos: usize, src: &[]u8): usize;
fn u8dWriteReplacement(out: []u8, pos: usize): usize;
fn decodeUtf8Chunk(input: &[]u8): Utf8ChunkResult;
fn isValidUtf8(input: &[]u8): bool;
```

## Minimal Usage

```cp
import { decodeUtf8Chunk } from "utf-8";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decodeUtf8Chunk(out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"utf-8"`; install via `cap add utf-8`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
