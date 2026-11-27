# deranged — AI Agent Quick Ref

- **Import**: `import { RangedU8, RangedU16, RangedU32, RangedU64, RangedI8, RangedI16, RangedI32, RangedI64 } from "deranged";`
- **Origin**: crates.io `deranged` (v1.0.0, MIT, zero-heap)
- **Full Types**: `src/index.d.cp`

## Exported Classes (all 8 integer widths)
- Unsigned: `RangedU8`, `RangedU16`, `RangedU32`, `RangedU64`
- Signed: `RangedI8`, `RangedI16`, `RangedI32`, `RangedI64`

## Common Interface (`T` = underlying int type, `R` = `Ranged*` class)

```cp
class R {
    val: T,
    min_v: T,
    max_v: T,
    ok: bool,

    fn new(val: T, min_v: T, max_v: T): R;
    fn newSaturating(val: T, min_v: T, max_v: T): R;
    fn get(): T;
    fn min(): T;
    fn max(): T;
    fn isValid(): bool;
    fn inBounds(v: T): bool;
    fn checkedAdd(delta: T): R;
    fn checkedSub(delta: T): R;
    fn saturatingAdd(delta: T): R;
    fn saturatingSub(delta: T): R;
    fn expand(new_min: T, new_max: T): R;
    fn narrow(new_min: T, new_max: T): R;
}
```

## Extra Methods on `RangedI32` & `RangedI64` Only
- `wrappingAdd(delta: T): R`
- `wrappingSub(delta: T): R`

## Gotchas
1. Out-of-bounds operations do NOT throw or return `null` — they return a `Ranged*` instance with `ok == false`. Always check `r.isValid()` before calling `r.get()`.
2. `wrappingAdd` and `wrappingSub` exist **only** on `RangedI32` and `RangedI64`, not on `U8/U16/U32/U64/I8/I16`.
3. `new(val, min_v, max_v)` also sets `ok = false` if `min_v > max_v`.
4. All methods are immutable value-returning operations (no in-place mutation).
