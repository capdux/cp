// @cap/deranged — Zero-heap ranged bounded integers (matching crates.io #187 deranged 801M+ downloads)
// Clean-room MIT implementation in pure CP covering RangedU8/U16/U32/U64/Usize & RangedI8/I16/I32/I64.

pub struct RangedI64 {
    pub val: i64,
    pub min_v: i64,
    pub max_v: i64,
    pub ok: bool,

    pub static fn new(val: i64, min_v: i64, max_v: i64): RangedI64 {
        if (min_v > max_v || val < min_v || val > max_v) {
            return RangedI64 { val: min_v, min_v: min_v, max_v: max_v, ok: false };
        }
        return RangedI64 { val: val, min_v: min_v, max_v: max_v, ok: true };
    }

    pub static fn newSaturating(val: i64, min_v: i64, max_v: i64): RangedI64 {
        let lo = if (min_v <= max_v) { min_v } else { max_v };
        let hi = if (min_v <= max_v) { max_v } else { min_v };
        let v = val;
        if (v < lo) { v = lo; }
        if (v > hi) { v = hi; }
        return RangedI64 { val: v, min_v: lo, max_v: hi, ok: true };
    }

    pub fn get(): i64 { return _.val; }
    pub fn min(): i64 { return _.min_v; }
    pub fn max(): i64 { return _.max_v; }
    pub fn isValid(): bool { return _.ok; }

    pub fn inBounds(v: i64): bool {
        return v >= _.min_v && v <= _.max_v;
    }

    pub fn checkedAdd(delta: i64): RangedI64 {
        if (!_.ok) { return RangedI64 { val: _.val, min_v: _.min_v, max_v: _.max_v, ok: false }; }
        let next = _.val + delta;
        return RangedI64.new(next, _.min_v, _.max_v);
    }

    pub fn saturatingAdd(delta: i64): RangedI64 {
        let next = _.val + delta;
        return RangedI64.newSaturating(next, _.min_v, _.max_v);
    }

    pub fn checkedSub(delta: i64): RangedI64 {
        return _.checkedAdd(-delta);
    }

    pub fn saturatingSub(delta: i64): RangedI64 {
        return _.saturatingAdd(-delta);
    }

    pub fn wrappingAdd(delta: i64): RangedI64 {
        let span = (_.max_v - _.min_v) + 1;
        if (span <= 0) {
            return RangedI64 { val: _.min_v, min_v: _.min_v, max_v: _.max_v, ok: false };
        }
        let offset = (_.val - _.min_v) + delta;
        while (offset < 0) { offset = offset + span; }
        while (offset >= span) { offset = offset - span; }
        return RangedI64 { val: _.min_v + offset, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn wrappingSub(delta: i64): RangedI64 {
        return _.wrappingAdd(-delta);
    }

    pub fn expand(new_min: i64, new_max: i64): RangedI64 {
        let lo = if (new_min < _.min_v) { new_min } else { _.min_v };
        let hi = if (new_max > _.max_v) { new_max } else { _.max_v };
        return RangedI64 { val: _.val, min_v: lo, max_v: hi, ok: _.ok };
    }

    pub fn narrow(new_min: i64, new_max: i64): RangedI64 {
        if (!_.ok) { return RangedI64 { val: _.val, min_v: new_min, max_v: new_max, ok: false }; }
        return RangedI64.new(_.val, new_min, new_max);
    }
}

pub struct RangedU8 {
    pub val: u8,
    pub min_v: u8,
    pub max_v: u8,
    pub ok: bool,

    pub static fn new(val: u8, min_v: u8, max_v: u8): RangedU8 {
        if (min_v > max_v || val < min_v || val > max_v) {
            return RangedU8 { val: min_v, min_v: min_v, max_v: max_v, ok: false };
        }
        return RangedU8 { val: val, min_v: min_v, max_v: max_v, ok: true };
    }

    pub static fn newSaturating(val: u8, min_v: u8, max_v: u8): RangedU8 {
        let lo = if (min_v <= max_v) { min_v } else { max_v };
        let hi = if (min_v <= max_v) { max_v } else { min_v };
        let v = val;
        if (v < lo) { v = lo; }
        if (v > hi) { v = hi; }
        return RangedU8 { val: v, min_v: lo, max_v: hi, ok: true };
    }

    pub fn get(): u8 { return _.val; }
    pub fn min(): u8 { return _.min_v; }
    pub fn max(): u8 { return _.max_v; }
    pub fn isValid(): bool { return _.ok; }
    pub fn inBounds(v: u8): bool { return v >= _.min_v && v <= _.max_v; }

    pub fn checkedAdd(delta: u8): RangedU8 {
        if (!_.ok) { return RangedU8 { val: _.val, min_v: _.min_v, max_v: _.max_v, ok: false }; }
        let sum: u16 = (_.val as u16) + (delta as u16);
        if (sum > (_.max_v as u16) || sum < (_.min_v as u16)) {
            return RangedU8 { val: _.min_v, min_v: _.min_v, max_v: _.max_v, ok: false };
        }
        return RangedU8 { val: sum as u8, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn checkedSub(delta: u8): RangedU8 {
        if (!_.ok) { return RangedU8 { val: _.val, min_v: _.min_v, max_v: _.max_v, ok: false }; }
        let diff: i32 = (_.val as i32) - (delta as i32);
        if (diff < (_.min_v as i32) || diff > (_.max_v as i32)) {
            return RangedU8 { val: _.min_v, min_v: _.min_v, max_v: _.max_v, ok: false };
        }
        return RangedU8 { val: diff as u8, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn saturatingAdd(delta: u8): RangedU8 {
        let sum: u16 = (_.val as u16) + (delta as u16);
        let hi: u16 = _.max_v as u16;
        let v: u8 = if (sum > hi) { _.max_v } else { sum as u8 };
        return RangedU8.newSaturating(v, _.min_v, _.max_v);
    }

    pub fn saturatingSub(delta: u8): RangedU8 {
        let diff: i32 = (_.val as i32) - (delta as i32);
        let lo: i32 = _.min_v as i32;
        let v: u8 = if (diff < lo) { _.min_v } else { diff as u8 };
        return RangedU8.newSaturating(v, _.min_v, _.max_v);
    }

    pub fn expand(new_min: u8, new_max: u8): RangedU8 {
        let lo = if (new_min < _.min_v) { new_min } else { _.min_v };
        let hi = if (new_max > _.max_v) { new_max } else { _.max_v };
        return RangedU8 { val: _.val, min_v: lo, max_v: hi, ok: _.ok };
    }

    pub fn narrow(new_min: u8, new_max: u8): RangedU8 {
        if (!_.ok) { return RangedU8 { val: _.val, min_v: new_min, max_v: new_max, ok: false }; }
        return RangedU8.new(_.val, new_min, new_max);
    }
}

pub struct RangedU16 {
    pub val: u16,
    pub min_v: u16,
    pub max_v: u16,
    pub ok: bool,

    pub static fn new(val: u16, min_v: u16, max_v: u16): RangedU16 {
        if (min_v > max_v || val < min_v || val > max_v) {
            return RangedU16 { val: min_v, min_v: min_v, max_v: max_v, ok: false };
        }
        return RangedU16 { val: val, min_v: min_v, max_v: max_v, ok: true };
    }

    pub static fn newSaturating(val: u16, min_v: u16, max_v: u16): RangedU16 {
        let lo = if (min_v <= max_v) { min_v } else { max_v };
        let hi = if (min_v <= max_v) { max_v } else { min_v };
        let v = val;
        if (v < lo) { v = lo; }
        if (v > hi) { v = hi; }
        return RangedU16 { val: v, min_v: lo, max_v: hi, ok: true };
    }

    pub fn get(): u16 { return _.val; }
    pub fn min(): u16 { return _.min_v; }
    pub fn max(): u16 { return _.max_v; }
    pub fn isValid(): bool { return _.ok; }
    pub fn inBounds(v: u16): bool { return v >= _.min_v && v <= _.max_v; }

    pub fn checkedAdd(delta: u16): RangedU16 {
        if (!_.ok) { return RangedU16 { val: _.val, min_v: _.min_v, max_v: _.max_v, ok: false }; }
        let sum: u32 = (_.val as u32) + (delta as u32);
        if (sum > (_.max_v as u32) || sum < (_.min_v as u32)) {
            return RangedU16 { val: _.min_v, min_v: _.min_v, max_v: _.max_v, ok: false };
        }
        return RangedU16 { val: sum as u16, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn checkedSub(delta: u16): RangedU16 {
        if (!_.ok) { return RangedU16 { val: _.val, min_v: _.min_v, max_v: _.max_v, ok: false }; }
        let diff: i64 = (_.val as i64) - (delta as i64);
        if (diff < (_.min_v as i64) || diff > (_.max_v as i64)) {
            return RangedU16 { val: _.min_v, min_v: _.min_v, max_v: _.max_v, ok: false };
        }
        return RangedU16 { val: diff as u16, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn saturatingAdd(delta: u16): RangedU16 {
        let sum: u32 = (_.val as u32) + (delta as u32);
        let hi: u32 = _.max_v as u32;
        let v: u16 = if (sum > hi) { _.max_v } else { sum as u16 };
        return RangedU16.newSaturating(v, _.min_v, _.max_v);
    }

    pub fn saturatingSub(delta: u16): RangedU16 {
        let diff: i64 = (_.val as i64) - (delta as i64);
        let lo: i64 = _.min_v as i64;
        let v: u16 = if (diff < lo) { _.min_v } else { diff as u16 };
        return RangedU16.newSaturating(v, _.min_v, _.max_v);
    }

    pub fn expand(new_min: u16, new_max: u16): RangedU16 {
        let lo = if (new_min < _.min_v) { new_min } else { _.min_v };
        let hi = if (new_max > _.max_v) { new_max } else { _.max_v };
        return RangedU16 { val: _.val, min_v: lo, max_v: hi, ok: _.ok };
    }

    pub fn narrow(new_min: u16, new_max: u16): RangedU16 {
        if (!_.ok) { return RangedU16 { val: _.val, min_v: new_min, max_v: new_max, ok: false }; }
        return RangedU16.new(_.val, new_min, new_max);
    }
}

pub struct RangedU32 {
    pub val: u32,
    pub min_v: u32,
    pub max_v: u32,
    pub ok: bool,

    pub static fn new(val: u32, min_v: u32, max_v: u32): RangedU32 {
        if (min_v > max_v || val < min_v || val > max_v) {
            return RangedU32 { val: min_v, min_v: min_v, max_v: max_v, ok: false };
        }
        return RangedU32 { val: val, min_v: min_v, max_v: max_v, ok: true };
    }

    pub static fn newSaturating(val: u32, min_v: u32, max_v: u32): RangedU32 {
        let lo = if (min_v <= max_v) { min_v } else { max_v };
        let hi = if (min_v <= max_v) { max_v } else { min_v };
        let v = val;
        if (v < lo) { v = lo; }
        if (v > hi) { v = hi; }
        return RangedU32 { val: v, min_v: lo, max_v: hi, ok: true };
    }

    pub fn get(): u32 { return _.val; }
    pub fn min(): u32 { return _.min_v; }
    pub fn max(): u32 { return _.max_v; }
    pub fn isValid(): bool { return _.ok; }
    pub fn inBounds(v: u32): bool { return v >= _.min_v && v <= _.max_v; }

    pub fn checkedAdd(delta: u32): RangedU32 {
        if (!_.ok) { return RangedU32 { val: _.val, min_v: _.min_v, max_v: _.max_v, ok: false }; }
        let sum: u64 = (_.val as u64) + (delta as u64);
        if (sum > (_.max_v as u64) || sum < (_.min_v as u64)) {
            return RangedU32 { val: _.min_v, min_v: _.min_v, max_v: _.max_v, ok: false };
        }
        return RangedU32 { val: sum as u32, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn checkedSub(delta: u32): RangedU32 {
        if (!_.ok) { return RangedU32 { val: _.val, min_v: _.min_v, max_v: _.max_v, ok: false }; }
        let diff: i64 = (_.val as i64) - (delta as i64);
        if (diff < (_.min_v as i64) || diff > (_.max_v as i64)) {
            return RangedU32 { val: _.min_v, min_v: _.min_v, max_v: _.max_v, ok: false };
        }
        return RangedU32 { val: diff as u32, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn saturatingAdd(delta: u32): RangedU32 {
        let sum: u64 = (_.val as u64) + (delta as u64);
        let hi: u64 = _.max_v as u64;
        let v: u32 = if (sum > hi) { _.max_v } else { sum as u32 };
        return RangedU32.newSaturating(v, _.min_v, _.max_v);
    }

    pub fn saturatingSub(delta: u32): RangedU32 {
        let diff: i64 = (_.val as i64) - (delta as i64);
        let lo: i64 = _.min_v as i64;
        let v: u32 = if (diff < lo) { _.min_v } else { diff as u32 };
        return RangedU32.newSaturating(v, _.min_v, _.max_v);
    }

    pub fn expand(new_min: u32, new_max: u32): RangedU32 {
        let lo = if (new_min < _.min_v) { new_min } else { _.min_v };
        let hi = if (new_max > _.max_v) { new_max } else { _.max_v };
        return RangedU32 { val: _.val, min_v: lo, max_v: hi, ok: _.ok };
    }

    pub fn narrow(new_min: u32, new_max: u32): RangedU32 {
        if (!_.ok) { return RangedU32 { val: _.val, min_v: new_min, max_v: new_max, ok: false }; }
        return RangedU32.new(_.val, new_min, new_max);
    }
}

pub struct RangedU64 {
    pub val: u64,
    pub min_v: u64,
    pub max_v: u64,
    pub ok: bool,

    pub static fn new(val: u64, min_v: u64, max_v: u64): RangedU64 {
        if (min_v > max_v || val < min_v || val > max_v) {
            return RangedU64 { val: min_v, min_v: min_v, max_v: max_v, ok: false };
        }
        return RangedU64 { val: val, min_v: min_v, max_v: max_v, ok: true };
    }

    pub static fn newSaturating(val: u64, min_v: u64, max_v: u64): RangedU64 {
        let lo = if (min_v <= max_v) { min_v } else { max_v };
        let hi = if (min_v <= max_v) { max_v } else { min_v };
        let v = val;
        if (v < lo) { v = lo; }
        if (v > hi) { v = hi; }
        return RangedU64 { val: v, min_v: lo, max_v: hi, ok: true };
    }

    pub fn get(): u64 { return _.val; }
    pub fn min(): u64 { return _.min_v; }
    pub fn max(): u64 { return _.max_v; }
    pub fn isValid(): bool { return _.ok; }
    pub fn inBounds(v: u64): bool { return v >= _.min_v && v <= _.max_v; }

    pub fn checkedAdd(delta: u64): RangedU64 {
        if (!_.ok || delta > (_.max_v - _.val)) {
            return RangedU64 { val: _.min_v, min_v: _.min_v, max_v: _.max_v, ok: false };
        }
        return RangedU64 { val: _.val + delta, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn checkedSub(delta: u64): RangedU64 {
        if (!_.ok || delta > (_.val - _.min_v)) {
            return RangedU64 { val: _.min_v, min_v: _.min_v, max_v: _.max_v, ok: false };
        }
        return RangedU64 { val: _.val - delta, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn saturatingAdd(delta: u64): RangedU64 {
        if (delta >= (_.max_v - _.val)) {
            return RangedU64 { val: _.max_v, min_v: _.min_v, max_v: _.max_v, ok: true };
        }
        return RangedU64 { val: _.val + delta, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn saturatingSub(delta: u64): RangedU64 {
        if (delta >= (_.val - _.min_v)) {
            return RangedU64 { val: _.min_v, min_v: _.min_v, max_v: _.max_v, ok: true };
        }
        return RangedU64 { val: _.val - delta, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn expand(new_min: u64, new_max: u64): RangedU64 {
        let lo = if (new_min < _.min_v) { new_min } else { _.min_v };
        let hi = if (new_max > _.max_v) { new_max } else { _.max_v };
        return RangedU64 { val: _.val, min_v: lo, max_v: hi, ok: _.ok };
    }

    pub fn narrow(new_min: u64, new_max: u64): RangedU64 {
        if (!_.ok) { return RangedU64 { val: _.val, min_v: new_min, max_v: new_max, ok: false }; }
        return RangedU64.new(_.val, new_min, new_max);
    }
}

pub struct RangedI8 {
    pub val: i8,
    pub min_v: i8,
    pub max_v: i8,
    pub ok: bool,

    pub static fn new(val: i8, min_v: i8, max_v: i8): RangedI8 {
        if (min_v > max_v || val < min_v || val > max_v) {
            return RangedI8 { val: min_v, min_v: min_v, max_v: max_v, ok: false };
        }
        return RangedI8 { val: val, min_v: min_v, max_v: max_v, ok: true };
    }

    pub static fn newSaturating(val: i8, min_v: i8, max_v: i8): RangedI8 {
        let lo = if (min_v <= max_v) { min_v } else { max_v };
        let hi = if (min_v <= max_v) { max_v } else { min_v };
        let v = val;
        if (v < lo) { v = lo; }
        if (v > hi) { v = hi; }
        return RangedI8 { val: v, min_v: lo, max_v: hi, ok: true };
    }

    pub fn get(): i8 { return _.val; }
    pub fn min(): i8 { return _.min_v; }
    pub fn max(): i8 { return _.max_v; }
    pub fn isValid(): bool { return _.ok; }
    pub fn inBounds(v: i8): bool { return v >= _.min_v && v <= _.max_v; }

    pub fn checkedAdd(delta: i8): RangedI8 {
        if (!_.ok) { return RangedI8 { val: _.val, min_v: _.min_v, max_v: _.max_v, ok: false }; }
        let sum: i32 = (_.val as i32) + (delta as i32);
        if (sum < (_.min_v as i32) || sum > (_.max_v as i32)) {
            return RangedI8 { val: _.min_v, min_v: _.min_v, max_v: _.max_v, ok: false };
        }
        return RangedI8 { val: sum as i8, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn checkedSub(delta: i8): RangedI8 {
        if (!_.ok) { return RangedI8 { val: _.val, min_v: _.min_v, max_v: _.max_v, ok: false }; }
        let diff: i32 = (_.val as i32) - (delta as i32);
        if (diff < (_.min_v as i32) || diff > (_.max_v as i32)) {
            return RangedI8 { val: _.min_v, min_v: _.min_v, max_v: _.max_v, ok: false };
        }
        return RangedI8 { val: diff as i8, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn saturatingAdd(delta: i8): RangedI8 {
        let sum: i32 = (_.val as i32) + (delta as i32);
        let lo: i32 = _.min_v as i32;
        let hi: i32 = _.max_v as i32;
        let v: i8 = if (sum < lo) { _.min_v } else if (sum > hi) { _.max_v } else { sum as i8 };
        return RangedI8 { val: v, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn saturatingSub(delta: i8): RangedI8 {
        let diff: i32 = (_.val as i32) - (delta as i32);
        let lo: i32 = _.min_v as i32;
        let hi: i32 = _.max_v as i32;
        let v: i8 = if (diff < lo) { _.min_v } else if (diff > hi) { _.max_v } else { diff as i8 };
        return RangedI8 { val: v, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn expand(new_min: i8, new_max: i8): RangedI8 {
        let lo = if (new_min < _.min_v) { new_min } else { _.min_v };
        let hi = if (new_max > _.max_v) { new_max } else { _.max_v };
        return RangedI8 { val: _.val, min_v: lo, max_v: hi, ok: _.ok };
    }

    pub fn narrow(new_min: i8, new_max: i8): RangedI8 {
        if (!_.ok) { return RangedI8 { val: _.val, min_v: new_min, max_v: new_max, ok: false }; }
        return RangedI8.new(_.val, new_min, new_max);
    }
}

pub struct RangedI16 {
    pub val: i16,
    pub min_v: i16,
    pub max_v: i16,
    pub ok: bool,

    pub static fn new(val: i16, min_v: i16, max_v: i16): RangedI16 {
        if (min_v > max_v || val < min_v || val > max_v) {
            return RangedI16 { val: min_v, min_v: min_v, max_v: max_v, ok: false };
        }
        return RangedI16 { val: val, min_v: min_v, max_v: max_v, ok: true };
    }

    pub static fn newSaturating(val: i16, min_v: i16, max_v: i16): RangedI16 {
        let lo = if (min_v <= max_v) { min_v } else { max_v };
        let hi = if (min_v <= max_v) { max_v } else { min_v };
        let v = val;
        if (v < lo) { v = lo; }
        if (v > hi) { v = hi; }
        return RangedI16 { val: v, min_v: lo, max_v: hi, ok: true };
    }

    pub fn get(): i16 { return _.val; }
    pub fn min(): i16 { return _.min_v; }
    pub fn max(): i16 { return _.max_v; }
    pub fn isValid(): bool { return _.ok; }
    pub fn inBounds(v: i16): bool { return v >= _.min_v && v <= _.max_v; }

    pub fn checkedAdd(delta: i16): RangedI16 {
        if (!_.ok) { return RangedI16 { val: _.val, min_v: _.min_v, max_v: _.max_v, ok: false }; }
        let sum: i64 = (_.val as i64) + (delta as i64);
        if (sum < (_.min_v as i64) || sum > (_.max_v as i64)) {
            return RangedI16 { val: _.min_v, min_v: _.min_v, max_v: _.max_v, ok: false };
        }
        return RangedI16 { val: sum as i16, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn checkedSub(delta: i16): RangedI16 {
        if (!_.ok) { return RangedI16 { val: _.val, min_v: _.min_v, max_v: _.max_v, ok: false }; }
        let diff: i64 = (_.val as i64) - (delta as i64);
        if (diff < (_.min_v as i64) || diff > (_.max_v as i64)) {
            return RangedI16 { val: _.min_v, min_v: _.min_v, max_v: _.max_v, ok: false };
        }
        return RangedI16 { val: diff as i16, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn saturatingAdd(delta: i16): RangedI16 {
        let sum: i64 = (_.val as i64) + (delta as i64);
        let lo: i64 = _.min_v as i64;
        let hi: i64 = _.max_v as i64;
        let v: i16 = if (sum < lo) { _.min_v } else if (sum > hi) { _.max_v } else { sum as i16 };
        return RangedI16 { val: v, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn saturatingSub(delta: i16): RangedI16 {
        let diff: i64 = (_.val as i64) - (delta as i64);
        let lo: i64 = _.min_v as i64;
        let hi: i64 = _.max_v as i64;
        let v: i16 = if (diff < lo) { _.min_v } else if (diff > hi) { _.max_v } else { diff as i16 };
        return RangedI16 { val: v, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn expand(new_min: i16, new_max: i16): RangedI16 {
        let lo = if (new_min < _.min_v) { new_min } else { _.min_v };
        let hi = if (new_max > _.max_v) { new_max } else { _.max_v };
        return RangedI16 { val: _.val, min_v: lo, max_v: hi, ok: _.ok };
    }

    pub fn narrow(new_min: i16, new_max: i16): RangedI16 {
        if (!_.ok) { return RangedI16 { val: _.val, min_v: new_min, max_v: new_max, ok: false }; }
        return RangedI16.new(_.val, new_min, new_max);
    }
}

pub struct RangedI32 {
    pub val: i32,
    pub min_v: i32,
    pub max_v: i32,
    pub ok: bool,

    pub static fn new(val: i32, min_v: i32, max_v: i32): RangedI32 {
        if (min_v > max_v || val < min_v || val > max_v) {
            return RangedI32 { val: min_v, min_v: min_v, max_v: max_v, ok: false };
        }
        return RangedI32 { val: val, min_v: min_v, max_v: max_v, ok: true };
    }

    pub static fn newSaturating(val: i32, min_v: i32, max_v: i32): RangedI32 {
        let lo = if (min_v <= max_v) { min_v } else { max_v };
        let hi = if (min_v <= max_v) { max_v } else { min_v };
        let v = val;
        if (v < lo) { v = lo; }
        if (v > hi) { v = hi; }
        return RangedI32 { val: v, min_v: lo, max_v: hi, ok: true };
    }

    pub fn get(): i32 { return _.val; }
    pub fn min(): i32 { return _.min_v; }
    pub fn max(): i32 { return _.max_v; }
    pub fn isValid(): bool { return _.ok; }
    pub fn inBounds(v: i32): bool { return v >= _.min_v && v <= _.max_v; }

    pub fn checkedAdd(delta: i32): RangedI32 {
        if (!_.ok) { return RangedI32 { val: _.val, min_v: _.min_v, max_v: _.max_v, ok: false }; }
        let sum: i64 = (_.val as i64) + (delta as i64);
        if (sum < (_.min_v as i64) || sum > (_.max_v as i64)) {
            return RangedI32 { val: _.min_v, min_v: _.min_v, max_v: _.max_v, ok: false };
        }
        return RangedI32 { val: sum as i32, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn checkedSub(delta: i32): RangedI32 {
        if (!_.ok) { return RangedI32 { val: _.val, min_v: _.min_v, max_v: _.max_v, ok: false }; }
        let diff: i64 = (_.val as i64) - (delta as i64);
        if (diff < (_.min_v as i64) || diff > (_.max_v as i64)) {
            return RangedI32 { val: _.min_v, min_v: _.min_v, max_v: _.max_v, ok: false };
        }
        return RangedI32 { val: diff as i32, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn saturatingAdd(delta: i32): RangedI32 {
        let sum: i64 = (_.val as i64) + (delta as i64);
        let lo: i64 = _.min_v as i64;
        let hi: i64 = _.max_v as i64;
        let v: i32 = if (sum < lo) { _.min_v } else if (sum > hi) { _.max_v } else { sum as i32 };
        return RangedI32 { val: v, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn saturatingSub(delta: i32): RangedI32 {
        let diff: i64 = (_.val as i64) - (delta as i64);
        let lo: i64 = _.min_v as i64;
        let hi: i64 = _.max_v as i64;
        let v: i32 = if (diff < lo) { _.min_v } else if (diff > hi) { _.max_v } else { diff as i32 };
        return RangedI32 { val: v, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn wrappingAdd(delta: i32): RangedI32 {
        let span: i64 = ((_.max_v as i64) - (_.min_v as i64)) + 1;
        if (span <= 0) {
            return RangedI32 { val: _.min_v, min_v: _.min_v, max_v: _.max_v, ok: false };
        }
        let offset: i64 = ((_.val as i64) - (_.min_v as i64)) + (delta as i64);
        while (offset < 0) { offset = offset + span; }
        while (offset >= span) { offset = offset - span; }
        return RangedI32 { val: ((_.min_v as i64) + offset) as i32, min_v: _.min_v, max_v: _.max_v, ok: true };
    }

    pub fn wrappingSub(delta: i32): RangedI32 {
        return _.wrappingAdd(-delta);
    }

    pub fn expand(new_min: i32, new_max: i32): RangedI32 {
        let lo = if (new_min < _.min_v) { new_min } else { _.min_v };
        let hi = if (new_max > _.max_v) { new_max } else { _.max_v };
        return RangedI32 { val: _.val, min_v: lo, max_v: hi, ok: _.ok };
    }

    pub fn narrow(new_min: i32, new_max: i32): RangedI32 {
        if (!_.ok) { return RangedI32 { val: _.val, min_v: new_min, max_v: new_max, ok: false }; }
        return RangedI32.new(_.val, new_min, new_max);
    }
}
