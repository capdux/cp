[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# float-cmp

Zero-heap IEEE-754 floating-point approximate equality, ULPs distance, next representable float stepping, ratio comparisons, and approximate ordering in pure CP.

- **Package**: `float-cmp` (`v1.0.0`)
- **Upstream Alignment**: Clean-room CP port of `crates.io/float-cmp` (`MIT`), composing `approx` for IEEE-754 epsilon and ULP primitives.

## Installation & Import

```bash
cap add float-cmp
```

```typescript
import {
    F64Margin, F32Margin,
    approxEqF64, approxNeF64, approxOrdF64, approxEqRatioF64,
    ulpsDiffF64, nextUpF64, nextDownF64,
} from "float-cmp";
```

## Core API

### Margin Builders

| Class | Fields & Methods | Description |
| --- | --- | --- |
| `F64Margin` | `epsilon: f64`, `ulps: u64`; `defaultMargin()`, `zero()`, `fromTuple(epsilon, ulps)`, `withEpsilon(eps)`, `withUlps(u)` | Dual absolute-epsilon and ULPs tolerance for `f64` comparisons. |
| `F32Margin` | `epsilon: f32`, `ulps: u32`; `defaultMargin()`, `zero()`, `fromTuple(epsilon, ulps)`, `withEpsilon(eps)`, `withUlps(u)` | Dual absolute-epsilon and ULPs tolerance for `f32` comparisons. |

### Approximate Equality, Ratio & Ordering

| Function | Signature | Description |
| --- | --- | --- |
| `approxEqF64` / `approxNeF64` | `(a: f64, b: f64, margin: F64Margin): bool` | Tests `f64` approximate equality / inequality using both `epsilon` and `ulps`. |
| `approxEqF32` / `approxNeF32` | `(a: f32, b: f32, margin: F32Margin): bool` | Tests `f32` approximate equality / inequality using both `epsilon` and `ulps`. |
| `approxEqRatioF64` | `(a: f64, b: f64, ratio: f64): bool` | Checks `|a - b| <= max(|a|, |b|) * ratio` for `f64`. |
| `approxEqRatioF32` | `(a: f32, b: f32, ratio: f32): bool` | Checks `|a - b| <= max(|a|, |b|) * ratio` for `f32`. |
| `approxOrdF64` | `(a: f64, b: f64, margin: F64Margin): i32` | Returns `0` if approx equal, `-1` if `a < b`, `1` if `a > b`. |
| `approxOrdF32` | `(a: f32, b: f32, margin: F32Margin): i32` | Returns `0` if approx equal, `-1` if `a < b`, `1` if `a > b`. |

### IEEE-754 Classification, ULPs & Adjacent Floats

| Function | Signature | Description |
| --- | --- | --- |
| `fcmpIsNanF64` / `fcmpIsNanF32` | `(x: f64/f32): bool` | Returns `true` if `x` is NaN. |
| `fcmpIsInfF64` / `fcmpIsInfF32` | `(x: f64/f32): bool` | Returns `true` if `x` is `+inf` or `-inf`. |
| `f64ToOrderedI64` / `f32ToOrderedI32` | `(x: f64): i64` / `(x: f32): i32` | Maps finite IEEE-754 bit patterns to order-preserving signed integers. |
| `ulpsDiffF64` / `ulpsDiffF32` | `(a, b): i64?` / `i32?` | Signed ULP distance (`a - b`); returns `null` if either operand is NaN or infinite. |
| `nextUpF64` / `nextDownF64` | `(x: f64): f64` | Next representable `f64` towards `+inf` / `-inf` (aliases: `nextUp`, `nextDown`). |
| `nextUpF32` / `nextDownF32` | `(x: f32): f32` | Next representable `f32` towards `+inf` / `-inf`. |

## Examples

### 1. Dual Epsilon & ULPs Comparison

```typescript
import { F64Margin, approxEqF64, approxOrdF64 } from "float-cmp";

fn main() {
    let m = F64Margin.defaultMargin().withUlps(4);
    let ok = approxEqF64(0.1 + 0.2, 0.3, m); // true
    let ord = approxOrdF64(1.0, 2.0, m);     // -1
}
```

### 2. ULPs Distance & Adjacent Stepping

```typescript
import { nextUpF64, ulpsDiffF64 } from "float-cmp";

fn main() {
    let a: f64 = 1.0;
    let b = nextUpF64(nextUpF64(a));
    if (let diff = ulpsDiffF64(b, a)) {
        // diff == 2
    }
}
```

### 3. Ratio Comparison

```typescript
import { approxEqRatioF64, approxEqRatioF32 } from "float-cmp";

fn main() {
    let close64 = approxEqRatioF64(1000.0, 1000.5, 0.001); // true
    let close32 = approxEqRatioF32(10.0, 10.01, 0.005);    // true
}
```

## Differences from Upstream & Highlights

- **Explicit Type Suffixes**: Replaces Rust `ApproxEq` / `Ulps` trait macros (`approx_eq!`) with direct `approxEqF64`/`approxEqF32`, `F64Margin`/`F32Margin` value types.
- **Null-Safe ULPs Diff**: `ulpsDiffF64` / `ulpsDiffF32` return CP optionals (`i64?` / `i32?`), returning `null` for NaN/Inf instead of overflowing.
- **Zero Heap & Composable**: Pure stack value structs with fluent `.withEpsilon()` / `.withUlps()` chaining.
