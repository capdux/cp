// Type definitions for CP

export class RangedI64 {
    val: i64,
    min_v: i64,
    max_v: i64,
    ok: bool,
    fn new(val: i64, min_v: i64, max_v: i64): RangedI64;
    fn newSaturating(val: i64, min_v: i64, max_v: i64): RangedI64;
    fn get(): i64;
    fn min(): i64;
    fn max(): i64;
    fn isValid(): bool;
    fn inBounds(v: i64): bool;
    fn checkedAdd(delta: i64): RangedI64;
    fn saturatingAdd(delta: i64): RangedI64;
    fn checkedSub(delta: i64): RangedI64;
    fn saturatingSub(delta: i64): RangedI64;
    fn wrappingAdd(delta: i64): RangedI64;
    fn wrappingSub(delta: i64): RangedI64;
    fn expand(new_min: i64, new_max: i64): RangedI64;
    fn narrow(new_min: i64, new_max: i64): RangedI64;
}

export class RangedU8 {
    val: u8,
    min_v: u8,
    max_v: u8,
    ok: bool,
    fn new(val: u8, min_v: u8, max_v: u8): RangedU8;
    fn newSaturating(val: u8, min_v: u8, max_v: u8): RangedU8;
    fn get(): u8;
    fn min(): u8;
    fn max(): u8;
    fn isValid(): bool;
    fn inBounds(v: u8): bool;
    fn checkedAdd(delta: u8): RangedU8;
    fn checkedSub(delta: u8): RangedU8;
    fn saturatingAdd(delta: u8): RangedU8;
    fn saturatingSub(delta: u8): RangedU8;
    fn expand(new_min: u8, new_max: u8): RangedU8;
    fn narrow(new_min: u8, new_max: u8): RangedU8;
}

export class RangedU16 {
    val: u16,
    min_v: u16,
    max_v: u16,
    ok: bool,
    fn new(val: u16, min_v: u16, max_v: u16): RangedU16;
    fn newSaturating(val: u16, min_v: u16, max_v: u16): RangedU16;
    fn get(): u16;
    fn min(): u16;
    fn max(): u16;
    fn isValid(): bool;
    fn inBounds(v: u16): bool;
    fn checkedAdd(delta: u16): RangedU16;
    fn checkedSub(delta: u16): RangedU16;
    fn saturatingAdd(delta: u16): RangedU16;
    fn saturatingSub(delta: u16): RangedU16;
    fn expand(new_min: u16, new_max: u16): RangedU16;
    fn narrow(new_min: u16, new_max: u16): RangedU16;
}

export class RangedU32 {
    val: u32,
    min_v: u32,
    max_v: u32,
    ok: bool,
    fn new(val: u32, min_v: u32, max_v: u32): RangedU32;
    fn newSaturating(val: u32, min_v: u32, max_v: u32): RangedU32;
    fn get(): u32;
    fn min(): u32;
    fn max(): u32;
    fn isValid(): bool;
    fn inBounds(v: u32): bool;
    fn checkedAdd(delta: u32): RangedU32;
    fn checkedSub(delta: u32): RangedU32;
    fn saturatingAdd(delta: u32): RangedU32;
    fn saturatingSub(delta: u32): RangedU32;
    fn expand(new_min: u32, new_max: u32): RangedU32;
    fn narrow(new_min: u32, new_max: u32): RangedU32;
}

export class RangedU64 {
    val: u64,
    min_v: u64,
    max_v: u64,
    ok: bool,
    fn new(val: u64, min_v: u64, max_v: u64): RangedU64;
    fn newSaturating(val: u64, min_v: u64, max_v: u64): RangedU64;
    fn get(): u64;
    fn min(): u64;
    fn max(): u64;
    fn isValid(): bool;
    fn inBounds(v: u64): bool;
    fn checkedAdd(delta: u64): RangedU64;
    fn checkedSub(delta: u64): RangedU64;
    fn saturatingAdd(delta: u64): RangedU64;
    fn saturatingSub(delta: u64): RangedU64;
    fn expand(new_min: u64, new_max: u64): RangedU64;
    fn narrow(new_min: u64, new_max: u64): RangedU64;
}

export class RangedI8 {
    val: i8,
    min_v: i8,
    max_v: i8,
    ok: bool,
    fn new(val: i8, min_v: i8, max_v: i8): RangedI8;
    fn newSaturating(val: i8, min_v: i8, max_v: i8): RangedI8;
    fn get(): i8;
    fn min(): i8;
    fn max(): i8;
    fn isValid(): bool;
    fn inBounds(v: i8): bool;
    fn checkedAdd(delta: i8): RangedI8;
    fn checkedSub(delta: i8): RangedI8;
    fn saturatingAdd(delta: i8): RangedI8;
    fn saturatingSub(delta: i8): RangedI8;
    fn expand(new_min: i8, new_max: i8): RangedI8;
    fn narrow(new_min: i8, new_max: i8): RangedI8;
}

export class RangedI16 {
    val: i16,
    min_v: i16,
    max_v: i16,
    ok: bool,
    fn new(val: i16, min_v: i16, max_v: i16): RangedI16;
    fn newSaturating(val: i16, min_v: i16, max_v: i16): RangedI16;
    fn get(): i16;
    fn min(): i16;
    fn max(): i16;
    fn isValid(): bool;
    fn inBounds(v: i16): bool;
    fn checkedAdd(delta: i16): RangedI16;
    fn checkedSub(delta: i16): RangedI16;
    fn saturatingAdd(delta: i16): RangedI16;
    fn saturatingSub(delta: i16): RangedI16;
    fn expand(new_min: i16, new_max: i16): RangedI16;
    fn narrow(new_min: i16, new_max: i16): RangedI16;
}

export class RangedI32 {
    val: i32,
    min_v: i32,
    max_v: i32,
    ok: bool,
    fn new(val: i32, min_v: i32, max_v: i32): RangedI32;
    fn newSaturating(val: i32, min_v: i32, max_v: i32): RangedI32;
    fn get(): i32;
    fn min(): i32;
    fn max(): i32;
    fn isValid(): bool;
    fn inBounds(v: i32): bool;
    fn checkedAdd(delta: i32): RangedI32;
    fn checkedSub(delta: i32): RangedI32;
    fn saturatingAdd(delta: i32): RangedI32;
    fn saturatingSub(delta: i32): RangedI32;
    fn wrappingAdd(delta: i32): RangedI32;
    fn wrappingSub(delta: i32): RangedI32;
    fn expand(new_min: i32, new_max: i32): RangedI32;
    fn narrow(new_min: i32, new_max: i32): RangedI32;
}

