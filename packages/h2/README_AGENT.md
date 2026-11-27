# h2 — AI Agent Quick Ref

Zero-allocation RFC 7541 HTTP/2 HPACK static table (61 entries), N-bit prefixed integer codec, and indexed header encoder for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    StaticEntry,
    IntDecodeResult,
    hpackEqBytes,
    hpackEntry,
    staticTableGet,
    staticTableFind,
    staticTableFindName,
    encodePrefixedInt,
} from "h2";
```

## Core Types & Constants

```cp
class StaticEntry { name: string, value: string, valid: bool }
class IntDecodeResult { value: u64, consumed: usize, valid: bool }
```

## API Signatures

```cp
fn hpackEqBytes(a: &[]u8, b: &[]u8): bool;
fn hpackEntry(n: string, v: string): StaticEntry;
fn staticTableGet(index: usize): StaticEntry;
fn staticTableFind(name: string, value: string): usize;
fn staticTableFindName(name: string): usize;
fn encodePrefixedInt(val: u64, prefix_bits: u8, first_byte_mask: u8, out: []u8): usize;
fn decodePrefixedInt(data: &[]u8, prefix_bits: u8): IntDecodeResult;
fn encodeIndexedHeader(index: usize, out: []u8): usize;
```

## Minimal Usage

```cp
import { hpackEntry } from "h2";

pub fn main() {
    let result = hpackEntry("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"h2"`; install via `cap add h2`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
