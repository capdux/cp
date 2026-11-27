# tonic — AI Agent Quick Ref

Zero-allocation gRPC over HTTP/2 length-prefixed frame codec, 17 status codes, and grpc-timeout parser/formatter in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    GrpcFrameHeader,
    statusName,
    isRetryableStatus,
    encodeFrame,
    decodeHeader,
    parseTimeoutMicros,
    formatTimeoutMillis,
} from "tonic";
```

## Core Types & Constants

```cp
class GrpcFrameHeader { compressed: bool, payload_len: u32, total_len: usize }
```

## API Signatures

```cp
fn statusName(code: u8): string;
fn isRetryableStatus(code: u8): bool;
fn encodeFrame(compressed: bool, payload: &[]u8, out: []u8): usize;
fn decodeHeader(data: &[]u8, out: &mut GrpcFrameHeader): i32;
fn parseTimeoutMicros(header: string): i64;
fn formatTimeoutMillis(ms: u64, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseTimeoutMicros } from "tonic";

pub fn main() {
    let result = parseTimeoutMicros("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"tonic"`; install via `cap add tonic`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
