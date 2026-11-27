# plist — AI Agent Quick Ref

Zero-allocation Apple XML Property List (.plist) dictionary parser and serializer in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PlistValKind,
    PlistValue,
    plstInvalid,
    plstEq,
    plstStartsAt,
    plstFindSub,
    plstSkipWs,
    plstParseI64,
} from "plist";
```

## Core Types & Constants

```cp
enum PlistValKind { String, Integer, Real, Bool, Data, Date, ... }
class PlistValue { kind: PlistValKind, str_val: string, int_val: i64, bool_val: bool }
```

## API Signatures

```cp
fn plstInvalid(): PlistValue;
fn plstEq(a: string, b: string): bool;
fn plstStartsAt(hay: string, pos: usize, needle: string): bool;
fn plstFindSub(hay: string, start: usize, needle: string): usize?;
fn plstSkipWs(s: string, start: usize): usize;
fn plstParseI64(s: string): i64;
fn plstSkipBalanced(xml: string, pos: usize, open_tag: string, close_tag: string): usize;
fn plstSkipElement(xml: string, pos: usize): usize;
fn plstParseValueAt(xml: string, pos: usize): PlistValue;
fn plistGetDictValue(xml: string, key_name: string): PlistValue;
fn plstWriteBytes(dst: []u8, pos: usize, src: string): usize;
fn plstFormatI64(val: i64, buf: []u8): usize;
fn plistWriteDictEntry(key: string, val: PlistValue, out: []u8): usize;
```

## Minimal Usage

```cp
import { plstEq } from "plist";

pub fn main() {
    let result = plstEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"plist"`; install via `cap add plist`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
