# num-iter@1.0.0 (Agent Quickref)

> Zero-heap stepped numeric range iterators for signed and unsigned 64-bit integers with overflow-safe inclusive/exclusive bounds (clean-room port of Rust num-iter) | License: MIT
> Import: `import { ... } from "num-iter";` | Full types: `src/index.d.cp`
> Deps: `num-integer`

## Functions
- `export fn nitrAbsI64(v: i64): u64;`
- `export fn nitrCheckedAddI64(a: i64, b: i64): i64?;`
- `export fn nitrDiffU64(hi: i64, lo: i64): u64;`
- `export fn nitrAdvanceSigned(state: i64, step: i64, n: usize): i64;`
- `export fn rangeStep(start: i64, stop: i64, step: i64): RangeStep;`
- `export fn rangeStepInclusive(start: i64, stop: i64, step: i64): RangeStepInclusive;`
- `export fn rangeStepU64(start: u64, stop: u64, step: u64): RangeStepU64;`
- `export fn collectRangeStep(r: &mut RangeStep, out: []i64): usize;`
