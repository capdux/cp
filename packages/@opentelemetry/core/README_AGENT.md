# @opentelemetry/core — AI Agent Quick Ref

Zero-allocation W3C Baggage (OpenTelemetry) HTTP header parser, percent-codec, and serializer in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    BaggageMember,
    isSpace,
    isTchar,
    isValidKey,
    trimSlice,
    strEq,
    parseSingleMember,
    parseBaggage,
} from "@opentelemetry/core";
```

## Core Types & Constants

```cp
class BaggageMember { key: string, value: string, properties: string }
```

## API Signatures

```cp
fn isSpace(b: u8): bool;
fn isTchar(b: u8): bool;
fn isValidKey(key: string): bool;
fn trimSlice(s: string): string;
fn strEq(a: string, b: string): bool;
fn parseSingleMember(seg: string, out: &mut BaggageMember): bool;
fn parseBaggage(header: string, out: []BaggageMember): usize;
fn countMembers(header: string): usize;
fn hexVal(b: u8): i32;
fn percentDecode(src: string, out: []u8, out_len: &mut usize): bool;
fn getDecodedValue(header: string, target_key: string, out: []u8, out_len: &mut usize): bool;
fn hexUpper(nibble: u8): u8;
fn needsPercentEncode(b: u8): bool;
fn formatBaggage(members: &[]BaggageMember, out: []u8): usize;
```

## Minimal Usage

```cp
import { isValidKey } from "@opentelemetry/core";

pub fn main() {
    let result = isValidKey("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@opentelemetry/core"`; install via `cap add @opentelemetry/core`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
