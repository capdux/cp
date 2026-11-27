# ndarray — AI Agent Quick Ref

Zero-heap fixed-capacity 2D/N-D numerical array with stride indexing, matmul, transpose, dot, and reductions (crates.io ndarray equivalent) (`v1.0.0`, `MIT`)

## Import

```cp
import { Array2D, addArrays, matmul, dotProduct } from "ndarray";
```

## Core Types & Constants

```cp
class Array2D { rows: usize, cols: usize, data: [64]f64 } // methods: zeros(...) +13
```

## API Signatures

```cp
fn addArrays(a: Array2D, b: Array2D): Array2D?;
fn matmul(a: Array2D, b: Array2D): Array2D?;
fn dotProduct(a: []f64, b: []f64): f64?;
```

## Minimal Usage

```cp
import { addArrays } from "ndarray";

pub fn main() {
    let result = addArrays(/* a: Array2D */, /* b: Array2D */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ndarray"`; install via `cap add ndarray`.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
