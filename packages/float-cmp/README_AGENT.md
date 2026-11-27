# float-cmp@1.0.0 (Agent Quickref)

> Zero-heap IEEE-754 floating-point approximate equality and ordering with dual epsilon/ULPs margins and ratio comparisons (clean-room port of Rust float-cmp) | License: MIT
> Import: `import { ... } from "float-cmp";` | Full types: `src/index.d.cp`
> Deps: `approx`

## Functions
- `export fn fcmpIsNanF64(x: f64): bool;`
- `export fn fcmpIsNanF32(x: f32): bool;`
- `export fn approxEqF64(a: f64, b: f64, margin: F64Margin): bool;`
- `export fn approxEqF32(a: f32, b: f32, margin: F32Margin): bool;`
- `export fn approxEqRatioF64(a: f64, b: f64, ratio: f64): bool;`
- `export fn approxOrdF64(a: f64, b: f64, margin: F64Margin): i32;`
