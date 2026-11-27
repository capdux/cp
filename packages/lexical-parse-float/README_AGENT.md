# lexical-parse-float — AI Agent Quick Ref

Zero-allocation pure CP strict floating-point and radix integer string parser (rust-lexical lexical-parse-float / lexical-parse-integer / lexical-util parity) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ParseFloatResult,
    ParseIntResult,
    lexpToLower,
    lexpEqIgnoreCase,
    lexpDecodeDigit,
    lexpScalePow10,
    parseFloat,
    parseIntRadix,
} from "lexical-parse-float";
```

## Core Types & Constants

```cp
class ParseFloatResult { ok: bool, value: f64, consumed: usize }
class ParseIntResult { ok: bool, value: i64, consumed: usize }
```

## API Signatures

```cp
fn lexpToLower(c: u8): u8;
fn lexpEqIgnoreCase(a: string, b: string): bool;
fn lexpDecodeDigit(c: u8): i32;
fn lexpScalePow10(val_in: f64, exp10: i32): f64;
fn parseFloat(s: string): ParseFloatResult;
fn parseIntRadix(s: string, radix: u32): ParseIntResult;
```

## Minimal Usage

```cp
import { parseFloat } from "lexical-parse-float";

pub fn main() {
    let result = parseFloat("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"lexical-parse-float"`; install via `cap add lexical-parse-float`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
