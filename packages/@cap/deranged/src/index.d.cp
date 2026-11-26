// Type definitions for CP

export class RangedI64 {
    val: i64,
    min_v: i64,
    max_v: i64,
    ok: bool,
    fn new(val: i64, min_v: i64, max_v: i64): RangedI64;
    fn newSaturating(val: i64, min_v: i64, max_v: i64): RangedI64;
    fn get(): i64;
    fn inBounds(v: i64): bool;
    fn checkedAdd(delta: i64): RangedI64;
    fn saturatingAdd(delta: i64): RangedI64;
    fn checkedSub(delta: i64): RangedI64;
    fn saturatingSub(delta: i64): RangedI64;
    fn wrappingAdd(delta: i64): RangedI64;
    fn expand(new_min: i64, new_max: i64): RangedI64;
    fn narrow(new_min: i64, new_max: i64): RangedI64;
}

