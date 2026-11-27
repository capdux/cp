# ciborium-ll — AI Agent Quick Ref

Zero-heap RFC 8949 low-level streaming CBOR encoder and decoder primitives in pure CP (`v1.0.0`, `Apache-2.0`)

## Import

```cp
import {
    CborHeaderKind,
    CborHeader,
    CborDecoder,
    CborEncoder,
    cblMaxI64AsU64,
    cblReadBe16,
    cblReadBe32,
    cblReadBe64,
} from "ciborium-ll";
```

## Core Types & Constants

```cp
enum CborHeaderKind { Positive, Negative, Bytes, Text, Array, Map, ... }
class CborHeader { kind: CborHeaderKind, u64_val: u64, i64_val: i64, f64_val: f64, /* +2 fields */ } // methods: invalid(...)
class CborDecoder { data: &[]u8, pos: usize } // methods: new(...) +4
class CborEncoder { buf: []u8, pos: usize } // methods: new(...) +18
```

## API Signatures

```cp
fn cblMaxI64AsU64(): u64;
fn cblReadBe16(data: &[]u8, pos: usize): u16;
fn cblReadBe32(data: &[]u8, pos: usize): u32;
fn cblReadBe64(data: &[]u8, pos: usize): u64;
fn cblDecodeHalf(bits: u16): f64;
fn cblArgSize(val: u64): usize;
fn cblWriteHead(buf: []u8, pos: usize, major: u8, val: u64): usize;
```

## Minimal Usage

```cp
import { cblMaxI64AsU64 } from "ciborium-ll";

pub fn main() {
    let result = cblMaxI64AsU64();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ciborium-ll"`; install via `cap add ciborium-ll`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
