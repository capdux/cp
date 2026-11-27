# resp — AI Agent Quick Ref

Zero-heap Redis RESP2 wire protocol streaming frame parser and command encoder. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RespFrame,
    IntResult,
    makeFrame,
    parseI64,
    findCrlf,
    parseFrame,
    appendByte,
    appendSlice,
} from "resp";
```

## Core Types & Constants

```cp
class RespFrame { kind: u8, data_start: usize, data_len: usize, int_val: i64, /* +2 fields */ }
class IntResult { value: i64, ok: bool }
```

## API Signatures

```cp
fn makeFrame(kind: u8): RespFrame;
fn parseI64(s: &[]u8): IntResult;
fn findCrlf(buf: &[]u8, start: usize): i64;
fn parseFrame(buf: &[]u8): RespFrame;
fn appendByte(dst: []u8, pos: usize, b: u8): i64;
fn appendSlice(dst: []u8, pos: usize, s: &[]u8): i64;
fn appendI64(dst: []u8, pos: usize, v: i64): i64;
fn encodeSimpleString(s: &[]u8, dst: []u8): i64;
fn encodeInteger(v: i64, dst: []u8): i64;
fn encodeNull(dst: []u8): i64;
fn encodeBulkString(s: &[]u8, dst: []u8): i64;
fn encodeCommand(args: &[]&[]u8, dst: []u8): i64;
```

## Minimal Usage

```cp
import { parseI64 } from "resp";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = parseI64(out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"resp"`; install via `cap add resp`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
