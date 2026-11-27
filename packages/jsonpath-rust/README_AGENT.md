# jsonpath-rust — AI Agent Quick Ref

Zero-heap RFC 9535 JSONPath expression parser and canonical serializer in pure CP (`v0.7.1`, `MIT`)

## Import

```cp
import {
    JpSegKind,
    JpSegment,
    JprIntRes,
    jprIsWs,
    jprIsDigit,
    jprIsIdentStart,
    jprIsIdentCont,
    jprIsSimpleIdent,
} from "jsonpath-rust";
```

## Core Types & Constants

```cp
enum JpSegKind { Root, Property, Index, Wildcard, RecursiveProp, RecursiveWildcard, ... }
class JpSegment { kind: JpSegKind, name: string, index: i64, slice_start: i64, /* +3 fields */ }
class JprIntRes { val: i64, next_pos: usize, ok: bool }
```

## API Signatures

```cp
fn jprIsWs(c: u8): bool;
fn jprIsDigit(c: u8): bool;
fn jprIsIdentStart(c: u8): bool;
fn jprIsIdentCont(c: u8): bool;
fn jprIsSimpleIdent(s: string): bool;
fn jprMakeSeg(kind: JpSegKind, name: string): JpSegment;
fn jprParseInt(s: string, start_pos: usize): JprIntRes;
fn parseJsonPath(expr: string, out: []JpSegment): i64;
fn jprPushByte(out: []u8, pos: usize, b: u8): usize;
fn jprPushSlice(out: []u8, pos: usize, s: string): usize;
fn jprPushI64(out: []u8, pos: usize, val: i64): usize;
fn formatJsonPath(segs: &[]JpSegment, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseJsonPath } from "jsonpath-rust";

pub fn main() {
    let result = parseJsonPath("example", /* out: []JpSegment */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"jsonpath-rust"`; install via `cap add jsonpath-rust`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
