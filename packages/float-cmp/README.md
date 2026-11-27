[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/float-cmp

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:float-cmp` ([https://github.com/mikedilger/float-cmp](https://github.com/mikedilger/float-cmp))

Clean-room MIT, zero-heap IEEE-754 floating-point approximate equality and ordering library for CP — modeled on crates.io `float-cmp` v0.10 and built directly on `@cap/approx`.

## Features

- **Dual-threshold margins**: `F64Margin` & `F32Margin` combine absolute `epsilon` and IEEE-754 `ulps` tolerances with builder helpers (`defaultMargin`, `zero`, `withEpsilon`, `withUlps`).
- **Strict IEEE-754 semantics**: exact equality short-circuiting (`-0.0 == +0.0`, `+Inf == +Inf`, `-Inf == -Inf`), strict `NaN` rejection in equality checks, and cross-sign ULP protection.
- **Relative ratio comparison**: `approxEqRatioF64` delegates to `@cap/approx`'s `relativeEqF64`.
- **Approximate total ordering**: `approxOrdF64` returns `-1`, `0`, or `1` with margin-aware equality collapsing.

## API

### Margin Types
- `F64Margin { epsilon: f64, ulps: u64 }`
  - `F64Margin.defaultMargin(): F64Margin` (`epsilon = F64_EPSILON`, `ulps = 4`)
  - `F64Margin.zero(): F64Margin` (`epsilon = 0.0`, `ulps = 0`)
  - `margin.withEpsilon(eps: f64): F64Margin`
  - `margin.withUlps(u: u64): F64Margin`
- `F32Margin { epsilon: f32, ulps: u32 }`
  - `F32Margin.defaultMargin(): F32Margin` (`epsilon = F32_EPSILON`, `ulps = 4`)
  - `F32Margin.zero(): F32Margin` (`epsilon = 0.0`, `ulps = 0`)
  - `margin.withEpsilon(eps: f32): F32Margin`
  - `margin.withUlps(u: u32): F32Margin`

### Functions
- `approxEqF64(a: f64, b: f64, margin: F64Margin): bool`
- `approxEqF32(a: f32, b: f32, margin: F32Margin): bool`
- `approxEqRatioF64(a: f64, b: f64, ratio: f64): bool`
- `approxOrdF64(a: f64, b: f64, margin: F64Margin): i32`

## Usage

```typescript
import {
    F64Margin,
    approxEqF64,
    approxEqRatioF64,
    approxOrdF64,
} from "@cap/float-cmp";

let m = F64Margin.defaultMargin().withEpsilon(1.0e-12);
let eq = approxEqF64(0.1 + 0.2, 0.3, m);        // true
let rel = approxEqRatioF64(1000.0, 1000.5, 0.001); // true
let ord = approxOrdF64(1.0, 1.0000000000001, m);   // 0
```

## License

MIT
