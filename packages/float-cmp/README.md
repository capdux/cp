[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# float-cmp

Zero-heap IEEE-754 floating-point approximate equality, ratio comparison, approximate ordering (`approxOrd`), signed ULPs difference, and adjacent representable float stepping (`nextUp` / `nextDown`). Clean-room port of Rust `float-cmp`.

## Provenance & Zero-Friction Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | crates.io |
| **Upstream Package** | `float-cmp` (`mikedilger/float-cmp`) |
| **CP Package** | `float-cmp` (`v1.0.0`) |
| **License** | MIT |

Keep the exact package name in CP — no `@cap/*` prefix or renaming required:

```rust
// Rust (crates.io: float-cmp)
use float_cmp::{approx_eq, F64Margin};
let ok = approx_eq!(f64, 0.1 + 0.2, 0.3, F64Margin { epsilon: 1e-12, ulps: 2 });
```

```typescript
// CP (same package name: "float-cmp")
import { approxEqF64, F64Margin } from "float-cmp";
let margin = F64Margin.fromTuple(1e-12, 2);
let ok = approxEqF64(0.1 + 0.2, 0.3, margin);
```

## Installation

```bash
cap add float-cmp
```

## Quick Start & Practical Examples

### 1. Dual Epsilon + ULPs Margin (`F64Margin` / `F32Margin`)

```typescript
import {
    F64Margin,
    F32Margin,
    approxEqF64,
    approxNeF64,
    approxEqF32,
    approxNeF32,
} from "float-cmp";

fn marginChecks(): void {
    // Construct margins via presets or builder helpers
    let m_def = F64Margin.defaultMargin();
    let m_zero = F64Margin.zero();
    let m_custom = F64Margin.fromTuple(1e-12, 4);
    let m_eps = F64Margin.withEpsilon(1e-9);
    let m_ulps = F64Margin.withUlps(2);

    let eq = approxEqF64(0.1 + 0.2, 0.3, m_custom);
    let ne = approxNeF64(1.0, 2.0, m_def);

    let m32 = F32Margin.fromTuple(1e-5 as f32, 4);
    let eq32 = approxEqF32(1.0 as f32, 1.000001 as f32, m32);
    let ne32 = approxNeF32(1.0 as f32, 2.0 as f32, m32);
}
```

### 2. Ratio Comparison & Approximate Ordering

```typescript
import {
    F64Margin,
    approxEqRatioF64,
    approxEqRatioF32,
    approxOrdF64,
    approxOrdF32,
} from "float-cmp";

fn ratioAndOrder(): void {
    // Relative ratio check: |a - b| <= ratio * max(|a|, |b|)
    let close = approxEqRatioF64(1000.0, 1000.5, 0.001);
    let close32 = approxEqRatioF32(100.0 as f32, 100.05 as f32, 0.001 as f32);

    // Approximate ordering: -1 (a < b), 0 (approx equal), +1 (a > b)
    let m = F64Margin.fromTuple(1e-9, 4);
    let ord = approxOrdF64(1.0, 1.0 + 1e-10, m); // 0
}
```

### 3. Signed ULPs Difference & Next Representable Float (`nextUp` / `nextDown`)

```typescript
import {
    ulpsDiffF64,
    ulpsDiffF32,
    nextUp,
    nextDown,
    nextUpF64,
    nextDownF64,
    nextUpF32,
    nextDownF32,
    f64ToOrderedI64,
    f32ToOrderedI32,
    fcmpIsNanF64,
    fcmpIsInfF64,
} from "float-cmp";

fn ulpsStepping(): void {
    let up = nextUp(1.0);       // alias for nextUpF64
    let down = nextDown(1.0);   // alias for nextDownF64
    if (let diff = ulpsDiffF64(1.0, up)) {
        // diff == -1 (1.0 is 1 ULP below up)
    }
    let up32 = nextUpF32(1.0 as f32);
    let down32 = nextDownF32(1.0 as f32);
    let diff32 = ulpsDiffF32(up32, 1.0 as f32); // +1

    // Lexicographically ordered integer keys for radix sort / indexing
    let key64 = f64ToOrderedI64(1.0);
    let key32 = f32ToOrderedI32(1.0 as f32);
}
```

## API Reference

### Margin Types
- `F64Margin { epsilon: f64, ulps: u64 }`: `defaultMargin()`, `zero()`, `fromTuple(epsilon, ulps)`, `withEpsilon(eps)`, `withUlps(u)`
- `F32Margin { epsilon: f32, ulps: u32 }`: `defaultMargin()`, `zero()`, `fromTuple(epsilon, ulps)`, `withEpsilon(eps)`, `withUlps(u)`

### Comparisons & Ordering
- `approxEqF64(a, b, margin): bool`, `approxNeF64(a, b, margin): bool`
- `approxEqF32(a, b, margin): bool`, `approxNeF32(a, b, margin): bool`
- `approxEqRatioF64(a, b, ratio): bool`, `approxEqRatioF32(a, b, ratio): bool`
- `approxOrdF64(a, b, margin): i32`, `approxOrdF32(a, b, margin): i32` (`-1`, `0`, `+1`)

### ULPs, Stepping & Classification
- `ulpsDiffF64(a: f64, b: f64): i64?`, `ulpsDiffF32(a: f32, b: f32): i32?`
- `nextUp` / `nextUpF64`, `nextDown` / `nextDownF64`, `nextUpF32`, `nextDownF32`
- `f64ToOrderedI64(x: f64): i64`, `f32ToOrderedI32(x: f32): i32`
- `fcmpIsNanF64` / `fcmpIsNanF32`, `fcmpIsInfF64` / `fcmpIsInfF32`
