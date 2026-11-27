# approx@1.0.0 (Agent Quickref)

> Zero-heap IEEE-754 floating-point approximate equality: abs_diff_eq, relative_eq, and ulps_eq for f64 and f32 (clean-room port of Rust approx) | License: MIT
> Import: `import { ... } from "approx";` | Full types: `src/index.d.cp`

## Functions
- `export fn apxAbsF64(x: f64): f64;`
- `export fn apxAbsF32(x: f32): f32;`
- `export fn apxIsNanF64(x: f64): bool;`
- `export fn apxIsNanF32(x: f32): bool;`
- `export fn apxIsInfF64(x: f64): bool;`
- `export fn apxIsInfF32(x: f32): bool;`
- `export fn absDiffEqF64(a: f64, b: f64, epsilon: f64): bool;`
- `export fn absDiffEqF32(a: f32, b: f32, epsilon: f32): bool;`
- `export fn relativeEqF64(a: f64, b: f64, epsilon: f64, max_relative: f64): bool;`
- `export fn relativeEqF32(a: f32, b: f32, epsilon: f32, max_relative: f32): bool;`
- `export fn ulpsDistanceF64(a: f64, b: f64): u64?;`
- `export fn ulpsEqF64(a: f64, b: f64, epsilon: f64, max_ulps: u64): bool;`
- `export fn ulpsDistanceF32(a: f32, b: f32): u32?;`
- `export fn ulpsEqF32(a: f32, b: f32, epsilon: f32, max_ulps: u32): bool;`
