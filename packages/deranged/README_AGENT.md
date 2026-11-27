# deranged — AI Agent Quick Ref

Zero-heap bounded integer wrappers (`RangedI8/I16/I32/I64`, `RangedU8/U16/U32/U64`) with checked, saturating, and wrapping arithmetic (Rust `deranged` parity).

## Import

```cp
import {
    RangedI8, RangedI16, RangedI32, RangedI64,
    RangedU8, RangedU16, RangedU32, RangedU64,
} from "deranged";
```

## Core API Signatures

```cp
// All 8 types share the same layout & core methods (T = i8|i16|i32|i64|u8|u16|u32|u64)
class RangedT { val: T, min_v: T, max_v: T, ok: bool }

RangedT.new(val: T, min_v: T, max_v: T): RangedT
RangedT.newSaturating(val: T, min_v: T, max_v: T): RangedT
r.get(): T
r.min(): T
r.max(): T
r.isValid(): bool
r.inBounds(v: T): bool
r.checkedAdd(delta: T): RangedT
r.checkedSub(delta: T): RangedT
r.saturatingAdd(delta: T): RangedT
r.saturatingSub(delta: T): RangedT
r.expand(new_min: T, new_max: T): RangedT
r.narrow(new_min: T, new_max: T): RangedT

// Modular range wrapping (RangedI32 and RangedI64 ONLY)
r.wrappingAdd(delta: T): RangedT
r.wrappingSub(delta: T): RangedT
```

## Minimal Example

```cp
import { RangedU8, RangedI32 } from "deranged";

fn demo() {
    let day = RangedU8.new(31, 1, 31);
    let next = day.checkedAdd(1);       // next.isValid() == false
    let clamped = day.saturatingAdd(5); // clamped.get() == 31

    let hour = RangedI32.new(23, 0, 23);
    let wrapped = hour.wrappingAdd(2);  // wrapped.get() == 1
}
```

## Gotchas & Borrow Rules

- **Value semantics**: All `Ranged*` structs are pure stack value types; all methods return a new `Ranged*` without mutating `self`.
- **Always check `isValid()`**: Out-of-bounds construction, `checkedAdd`/`checkedSub`, `expand`, and `narrow` never panic—they return a struct with `ok = false`.
- **Wrapping availability**: Only `RangedI32` and `RangedI64` expose `wrappingAdd` and `wrappingSub`; `RangedU*` and `RangedI8`/`RangedI16` do not.
- **Invalid range**: Passing `min_v > max_v` yields `ok = false` even for `newSaturating`.
- **Expand vs Narrow**: `expand` requires `new_min <= min_v && new_max >= max_v`; `narrow` requires `new_min <= val && val <= new_max`.
