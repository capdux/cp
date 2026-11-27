[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# deranged

Zero-heap ranged bounded integer library for CP, modeled on Rust's [`deranged`](https://github.com/jhpratt/deranged) crate (MIT, CP v1.0.0).

Provides value-type bounded integer wrappers across all 8 fixed-width integer types (`RangedI8`, `RangedI16`, `RangedI32`, `RangedI64`, `RangedU8`, `RangedU16`, `RangedU32`, `RangedU64`) with checked, saturating, wrapping (for `RangedI32`/`RangedI64`), and range-narrowing/expanding operations—ideal for dates, times, ports, percentages, and protocol fields.

## Installation & Import

```bash
cap add deranged
```

```cp
import { RangedU8, RangedI32, RangedI64, RangedU16 } from "deranged";
```

## Core API Reference

### Available Types

All 8 fixed-width integer types share a uniform value layout `{ val: T, min_v: T, max_v: T, ok: bool }`:

- Signed: `RangedI8` (`i8`), `RangedI16` (`i16`), `RangedI32` (`i32`), `RangedI64` (`i64`)
- Unsigned: `RangedU8` (`u8`), `RangedU16` (`u16`), `RangedU32` (`u32`), `RangedU64` (`u64`)

### Common Methods (All 8 Types)

| Method | Signature (`T` = underlying int) | Description |
| :--- | :--- | :--- |
| `new(val, min_v, max_v)` | `(T, T, T): RangedT` | Constructs a ranged value; sets `ok = false` if `min_v > max_v` or `val` is out of `[min_v, max_v]`. |
| `newSaturating(val, min_v, max_v)` | `(T, T, T): RangedT` | Clamps `val` into `[min_v, max_v]` (sets `ok = false` only if `min_v > max_v`). |
| `get()` | `(): T` | Returns the stored integer value. |
| `min()` / `max()` | `(): T` | Returns the inclusive lower / upper bound. |
| `isValid()` | `(): bool` | Returns `true` if `ok` is set and the value is within bounds. |
| `inBounds(v)` | `(T): bool` | Checks whether `v` lies in `[min_v, max_v]`. |
| `checkedAdd(delta)` | `(T): RangedT` | Adds `delta` with overflow & range checks; sets `ok = false` if out of bounds. |
| `checkedSub(delta)` | `(T): RangedT` | Subtracts `delta` with underflow & range checks; sets `ok = false` if out of bounds. |
| `saturatingAdd(delta)` | `(T): RangedT` | Adds `delta`, clamping to `[min_v, max_v]`. |
| `saturatingSub(delta)` | `(T): RangedT` | Subtracts `delta`, clamping to `[min_v, max_v]`. |
| `expand(new_min, new_max)` | `(T, T): RangedT` | Widens bounds if `new_min <= min_v && new_max >= max_v`; otherwise sets `ok = false`. |
| `narrow(new_min, new_max)` | `(T, T): RangedT` | Tightens bounds if `new_min <= val <= new_max`; otherwise sets `ok = false`. |

### Additional Modular Wrapping Methods (`RangedI32` & `RangedI64` Only)

| Method | Signature | Description |
| :--- | :--- | :--- |
| `wrappingAdd(delta)` | `(T): RangedT` | Wraps `val + delta` modularly within `[min_v, max_v]` (e.g. clock hours/minutes). |
| `wrappingSub(delta)` | `(T): RangedT` | Wraps `val - delta` modularly within `[min_v, max_v]`. |

## Usage Examples

### 1. Bounded Month & Day Validation (`RangedU8`)

```cp
import { RangedU8 } from "deranged";

fn validateMonth() {
    let month = RangedU8.new(12, 1, 12);
    if (month.isValid()) {
        let overflow = month.checkedAdd(1);   // overflow.isValid() == false
        let clamped = month.saturatingAdd(5); // clamped.get() == 12
    }

    let pct = RangedU8.newSaturating(150, 0, 100);
    // pct.isValid() == true, pct.get() == 100
}
```

### 2. Modular Clock Arithmetic (`RangedI32`)

```cp
import { RangedI32 } from "deranged";

fn clockExample() {
    let hour = RangedI32.new(23, 0, 23);
    let next = hour.wrappingAdd(3); // next.get() == 2
    let prev = next.wrappingSub(5); // prev.get() == 21
}
```

### 3. Expanding & Narrowing Ranges (`RangedI64`)

```cp
import { RangedI64 } from "deranged";

fn rangeTransform() {
    let v = RangedI64.new(50, 0, 100);
    let wide = v.expand(-100, 200); // wide.isValid() == true
    let tight = wide.narrow(10, 60); // tight.isValid() == true, tight.get() == 50
    let invalid = tight.narrow(60, 100); // invalid.isValid() == false
}
```

## Differences from Upstream & Safety Highlights

- **Runtime Bounds with Zero Heap**: Rust's `deranged` encodes bounds as const-generic type parameters (`RangedU8<MIN, MAX>`); CP's `deranged` stores `min_v`, `max_v`, and `ok` directly in a compact stack value struct so bounds can be dynamic without heap allocation.
- **No Panics**: Every constructor and arithmetic method propagates validity via `ok: bool` (`isValid()`), avoiding runtime panics on overflow or out-of-range inputs.
- **Overflow-Safe Internals**: Checked and saturating arithmetic guard against native integer overflow before comparing against `[min_v, max_v]`.
