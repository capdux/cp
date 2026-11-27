# ksuid — AI Agent Quick Ref

Zero-heap Segment KSUID (20-byte K-Sortable Unique IDentifier) 27-character Base62 encoder, decoder, validator, and comparator. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Ksuid,
    b62Char,
    b62Val,
    fromParts,
    timestampSecs,
    encode,
    decode,
    isValidKsuid,
} from "ksuid";
```

## Core Types & Constants

```cp
class Ksuid { bytes: [20]u8 }
```

## API Signatures

```cp
fn b62Char(v: u8): u8;
fn b62Val(c: u8): i32;
fn fromParts(unix_secs: u64, payload: [16]u8): Ksuid;
fn timestampSecs(k: Ksuid): u64;
fn encode(k: Ksuid, out: []u8): usize;
fn decode(s: string, out: &mut Ksuid): bool;
fn isValidKsuid(s: string): bool;
fn compare(a: Ksuid, b: Ksuid): i32;
```

## Minimal Usage

```cp
import { decode } from "ksuid";

pub fn main() {
    let result = decode("example", /* out: &mut Ksuid */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ksuid"`; install via `cap add ksuid`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
