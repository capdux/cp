# jsonrpc-core — AI Agent Quick Ref

Zero-heap JSON-RPC 2.0 request/notification/result/error encoder, envelope validator, batch detector, and standard error codes. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    isServerError,
    errorMessage,
    isWs,
    appendByte,
    appendSlice,
    appendI32,
    appendJsonString,
    encodeRequest,
} from "jsonrpc-core";
```

## API Signatures

```cp
fn isServerError(code: i32): bool;
fn errorMessage(code: i32): string;
fn isWs(c: u8): bool;
fn appendByte(out: []u8, pos: usize, b: u8): i64;
fn appendSlice(out: []u8, pos: usize, s: string): i64;
fn appendI32(out: []u8, pos: usize, v: i32): i64;
fn appendJsonString(out: []u8, pos: usize, s: string): i64;
fn encodeRequest(id_json: string, method: string, params_json: string, out: []u8): usize;
fn encodeResult(id_json: string, result_json: string, out: []u8): usize;
fn encodeError(id_json: string, code: i32, message: string, data_json: string, out: []u8): usize;
fn isBatch(json: string): bool;
fn isValidEnvelope(json: string): bool;
```

## Minimal Usage

```cp
import { encodeResult } from "jsonrpc-core";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeResult("example", "example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"jsonrpc-core"`; install via `cap add jsonrpc-core`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
