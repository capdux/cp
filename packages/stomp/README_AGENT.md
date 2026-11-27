# stomp — AI Agent Quick Ref

Zero-allocation STOMP 1.2 messaging protocol frame encoder, decoder, and header escaper in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    StompHeader,
    StompFrame,
    escapeHeader,
    unescapeHeader,
    encodeFrame,
    parseContentLength,
    decodeFrame,
    getHeader,
} from "stomp";
```

## Core Types & Constants

```cp
class StompHeader { key: string, value: string }
class StompFrame { command: string, headers: [16]StompHeader, header_count: usize, body: string, /* +1 fields */ } // methods: empty(...)
```

## API Signatures

```cp
fn escapeHeader(input: string, out: []u8): usize;
fn unescapeHeader(input: string, out: []u8): usize;
fn encodeFrame(command: string, headers: &[]StompHeader, body: string, out: []u8): usize;
fn parseContentLength(val: string): i64;
fn decodeFrame(data: string, out_frame: &mut StompFrame): i32;
fn getHeader(frame: StompFrame, key: string): string;
```

## Minimal Usage

```cp
import { parseContentLength } from "stomp";

pub fn main() {
    let result = parseContentLength("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"stomp"`; install via `cap add stomp`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
