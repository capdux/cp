# uuidv7 — AI Agent Quick Ref

Zero-allocation RFC 9562 UUIDv7 constructor, 36-byte formatter, parser, timestamp extractor, and comparator in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    UuidV7,
    fromParts,
    timestampMs,
    version,
    isRfcVariant,
    formatHyphenated,
    parse,
    isValidUuidV7,
} from "uuidv7";
```

## Core Types & Constants

```cp
class UuidV7 { bytes: [16]u8 }
```

## API Signatures

```cp
fn fromParts(timestamp_ms: u64, rand_bytes: [10]u8): UuidV7;
fn timestampMs(u: UuidV7): u64;
fn version(u: UuidV7): u8;
fn isRfcVariant(u: UuidV7): bool;
fn formatHyphenated(u: UuidV7, out: []u8): usize;
fn parse(s: string, out: &mut UuidV7): bool;
fn isValidUuidV7(s: string): bool;
fn compare(a: UuidV7, b: UuidV7): i32;
```

## Minimal Usage

```cp
import { parse } from "uuidv7";

pub fn main() {
    let result = parse("example", /* out: &mut UuidV7 */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"uuidv7"`; install via `cap add uuidv7`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
