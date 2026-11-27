# segment-tree — AI Agent Quick Ref

Zero-allocation iterative 2*N Segment Tree with O(log N) rangeSum, rangeMin, rangeMax, and point updates (Rust segment-tree parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { RangeStats, SegmentTree, minI64, maxI64 } from "segment-tree";
```

## Core Types & Constants

```cp
class RangeStats { sum: i64, min_val: i64, max_val: i64, valid: bool }
class SegmentTree { size: usize, sums: [TREE_CAP]i64, mins: [TREE_CAP]i64, maxs: [TREE_CAP]i64 } // methods: init(...) +9
```

## API Signatures

```cp
fn minI64(a: i64, b: i64): i64;
fn maxI64(a: i64, b: i64): i64;
```

## Minimal Usage

```cp
import { minI64 } from "segment-tree";

pub fn main() {
    let result = minI64(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"segment-tree"`; install via `cap add segment-tree`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
