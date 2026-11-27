# h3o — AI Agent Quick Ref

Zero-heap Uber H3 64-bit hexagonal hierarchical spatial index cell parser, constructor, hierarchy traversal, and area table (clean-room port of Rust h3o) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    H3Cell,
    h3AllLow63,
    h3DigitShift,
    h3IsPentagonBaseCell,
    h3HexDigitVal,
    h3HexChar,
    h3ClearBits,
    parseH3Hex,
} from "h3o";
```

## Core Types & Constants

```cp
class H3Cell { bits: u64 } // methods: fromParts(...) +8
```

## API Signatures

```cp
fn h3AllLow63(): u64;
fn h3DigitShift(r: u8): u6;
fn h3IsPentagonBaseCell(bc: u8): bool;
fn h3HexDigitVal(c: u8): u8;
fn h3HexChar(nibble: u8): u8;
fn h3ClearBits(val: u64, mask: u64): u64;
fn parseH3Hex(hex: string): H3Cell?;
fn avgHexAreaKm2(res: u8): f64;
```

## Minimal Usage

```cp
import { parseH3Hex } from "h3o";

pub fn main() {
    let result = parseH3Hex("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"h3o"`; install via `cap add h3o`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
