# ordered-float@1.0.0 (Agent Quickref)

> Zero-heap IEEE-754 total ordering, canonical hashing, and NaN-safe min/max/clamp for f64 and f32 (clean-room port of Rust ordered-float) | License: MIT
> Import: `import { ... } from "ordered-float";` | Full types: `src/index.d.cp`
> Deps: `@sindresorhus/fnv1a`

## Functions
- `export fn isNanF64(x: f64): bool;`
- `export fn isInfiniteF64(x: f64): bool;`
- `export fn isFiniteF64(x: f64): bool;`
- `export fn cmpOrderedF64(a: f64, b: f64): i32;`
- `export fn eqOrderedF64(a: f64, b: f64): bool;`
- `export fn hashOrderedF64(x: f64): u64;`
- `export fn sortOrderedF64(arr: []f64);`
