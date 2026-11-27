# bytesize — AI Agent Quick Ref

Zero-heap integer byte size constants (SI & IEC), ByteSize builder, parser, and human-readable formatter in pure CP (`v1.0.0`, `Apache-2.0`)

## Import

```cp
import {
    ByteSize,
    bszWriteByte,
    bszUnitMultiplier,
    bszFormatScaled,
    formatSi,
    formatIec,
    parseByteSize,
} from "bytesize";
```

## Core Types & Constants

```cp
class ByteSize { val: u64 } // methods: b(...) +15
```

## API Signatures

```cp
fn bszWriteByte(out: []u8, pos: usize, b: u8): usize;
fn bszUnitMultiplier(s: string): u64?;
fn bszFormatScaled(bytes: u64, unit: u64, suffix: string, out: []u8): usize;
fn formatSi(bytes: u64, out: []u8): usize;
fn formatIec(bytes: u64, out: []u8): usize;
fn parseByteSize(input: string): u64?;
```

## Minimal Usage

```cp
import { parseByteSize } from "bytesize";

pub fn main() {
    let result = parseByteSize("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bytesize"`; install via `cap add bytesize`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
