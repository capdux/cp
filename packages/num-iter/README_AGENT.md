# num-iter — AI Agent Quick Ref

Zero-heap stepped numeric range iterators for signed and unsigned 64-bit integers with overflow-safe inclusive/exclusive bounds (clean-room port of Rust num-iter) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RangeStep,
    RangeStepInclusive,
    RangeStepU64,
    nitrAbsI64,
    nitrCheckedAddI64,
    nitrDiffU64,
    nitrAdvanceSigned,
    rangeStep,
} from "num-iter";
```

## Core Types & Constants

```cp
class RangeStep { state: i64, stop: i64, step: i64, done: bool } // methods: isDone(...) +3
class RangeStepInclusive { state: i64, stop: i64, step: i64, done: bool } // methods: isDone(...) +2
class RangeStepU64 { state: u64, stop: u64, step: u64, done: bool } // methods: isDone(...) +3
```

## API Signatures

```cp
fn nitrAbsI64(v: i64): u64;
fn nitrCheckedAddI64(a: i64, b: i64): i64?;
fn nitrDiffU64(hi: i64, lo: i64): u64;
fn nitrAdvanceSigned(state: i64, step: i64, n: usize): i64;
fn rangeStep(start: i64, stop: i64, step: i64): RangeStep;
fn rangeStepInclusive(start: i64, stop: i64, step: i64): RangeStepInclusive;
fn rangeStepU64(start: u64, stop: u64, step: u64): RangeStepU64;
fn collectRangeStep(r: &mut RangeStep, out: []i64): usize;
```

## Minimal Usage

```cp
import { nitrAbsI64 } from "num-iter";

pub fn main() {
    let result = nitrAbsI64(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"num-iter"`; install via `cap add num-iter`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
