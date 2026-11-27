# unsigned-varint — AI Agent Quick Ref

Zero-allocation 3-in-1 variable-length integer codec (ULEB128, SLEB128, Protobuf ZigZag, and QUIC RFC 9000 varint) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    U64Decode,
    I64Decode,
    i64ToBits,
    bitsToI64,
    uleb128Size,
    encodeUleb128,
    decodeUleb128,
    zigzagEncode,
} from "unsigned-varint";
```

## Core Types & Constants

```cp
class U64Decode { ok: bool, value: u64, bytes_read: usize } // methods: fail(...)
class I64Decode { ok: bool, value: i64, bytes_read: usize } // methods: fail(...)
```

## API Signatures

```cp
fn i64ToBits(v: i64): u64;
fn bitsToI64(u: u64): i64;
fn uleb128Size(v: u64): usize;
fn encodeUleb128(v: u64, out: []u8): usize?;
fn decodeUleb128(data: string): U64Decode;
fn zigzagEncode(v: i64): u64;
fn zigzagDecode(u: u64): i64;
fn encodeSleb128(v: i64, out: []u8): usize?;
fn decodeSleb128(data: string): I64Decode;
fn encodeZigzagVarint(v: i64, out: []u8): usize?;
fn decodeZigzagVarint(data: string): I64Decode;
fn encodeQuicVarint(v: u64, out: []u8): usize?;
fn decodeQuicVarint(data: string): U64Decode;
```

## Minimal Usage

```cp
import { decodeUleb128 } from "unsigned-varint";

pub fn main() {
    let result = decodeUleb128("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"unsigned-varint"`; install via `cap add unsigned-varint`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
