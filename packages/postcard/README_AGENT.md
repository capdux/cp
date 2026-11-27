# postcard — AI Agent Quick Ref

Zero-heap compact binary wire protocol codecs (Varint, ZigZag, and COBS framing) (`v1.1.3`, `MIT`)

## Import

```cp
import {
    VarintU64Res,
    VarintI64Res,
    pcrdU64Max,
    encodeVarintU64,
    decodeVarintU64,
    zigzagEncode,
    zigzagDecode,
    encodeVarintI64,
} from "postcard";
```

## Core Types & Constants

```cp
class VarintU64Res { ok: bool, value: u64, consumed: usize }
class VarintI64Res { ok: bool, value: i64, consumed: usize }
```

## API Signatures

```cp
fn pcrdU64Max(): u64;
fn encodeVarintU64(val: u64, out: []u8): usize;
fn decodeVarintU64(data: &[]u8): VarintU64Res;
fn zigzagEncode(val: i64): u64;
fn zigzagDecode(val: u64): i64;
fn encodeVarintI64(val: i64, out: []u8): usize;
fn decodeVarintI64(data: &[]u8): VarintI64Res;
fn cobsEncode(src: &[]u8, out: []u8): usize;
fn cobsDecode(src: &[]u8, out: []u8): i64;
```

## Minimal Usage

```cp
import { encodeVarintU64 } from "postcard";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeVarintU64(0, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"postcard"`; install via `cap add postcard`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
