# http-content-range — AI Agent Quick Ref

Zero-allocation RFC 9110 HTTP Content-Range header parser and formatter (clean-room MIT implementation of crates.io http-content-range) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ContentRangeKind,
    ContentRange,
    hcrParseU64,
    hcrPush,
    hcrPushU64,
    parseContentRange,
    formatContentRangeHeader,
} from "http-content-range";
```

## Core Types & Constants

```cp
enum ContentRangeKind { Invalid, Bytes, BytesUnknownLength, Unsatisfied }
class ContentRange { kind: ContentRangeKind, first_byte: u64, last_byte: u64, complete_length: u64 } // methods: isValid(...) +1
```

## API Signatures

```cp
fn hcrParseU64(s: string): u64?;
fn hcrPush(out: []u8, pos: usize, s: string): usize;
fn hcrPushU64(out: []u8, pos: usize, val: u64): usize;
fn parseContentRange(header: string): ContentRange;
fn formatContentRangeHeader(cr: ContentRange, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseContentRange } from "http-content-range";

pub fn main() {
    let result = parseContentRange("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"http-content-range"`; install via `cap add http-content-range`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
