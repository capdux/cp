# parse_duration — AI Agent Quick Ref

Zero-heap human-readable duration parser and formatter supporting compound expressions, fractional units, Unicode microseconds, and overflow protection (clean-room port of Rust parse_duration / duration-str) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ParsedDuration,
    pdurCheckedAddU64,
    pdurCheckedMulU64,
    pdurMatchUnit,
    pdurAppendPart,
    parseDuration,
    formatDuration,
} from "parse_duration";
```

## Core Types & Constants

```cp
class ParsedDuration { secs: u64, nanos: u32 } // methods: isZero(...) +3
```

## API Signatures

```cp
fn pdurCheckedAddU64(a: u64, b: u64): u64?;
fn pdurCheckedMulU64(a: u64, b: u64): u64?;
fn pdurMatchUnit(u: string): u32;
fn pdurAppendPart(out: []u8, pos: usize, val: u64, suffix: string): usize;
fn parseDuration(input: string): ParsedDuration?;
fn formatDuration(d: ParsedDuration, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseDuration } from "parse_duration";

pub fn main() {
    let result = parseDuration("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"parse_duration"`; install via `cap add parse_duration`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
