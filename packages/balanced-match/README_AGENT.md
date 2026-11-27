# balanced-match — AI Agent Quick Ref

Zero-allocation nested balanced delimiter matcher (npm balanced-match v3 parity) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    BalancedResult,
    bmMakeRange,
    bmEmptyResult,
    balancedRange,
    balanced,
    balancedAll,
} from "balanced-match";
```

## Core Types & Constants

```cp
class BalancedResult { found: bool, start: usize, end: usize, pre: string, /* +2 fields */ }
```

## API Signatures

```cp
fn bmMakeRange(s: usize, e: usize): [usize, usize];
fn bmEmptyResult(): BalancedResult;
fn balancedRange(a: string, b: string, input: string): [usize, usize]?;
fn balanced(a: string, b: string, input: string): BalancedResult;
fn balancedAll(a: string, b: string, input: string, out: []BalancedResult): usize;
```

## Minimal Usage

```cp
import { balancedRange } from "balanced-match";

pub fn main() {
    let result = balancedRange("example", "example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"balanced-match"`; install via `cap add balanced-match`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
