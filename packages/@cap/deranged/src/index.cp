// @cap/deranged — Zero-heap ranged bounded integers (matching crates.io #187 deranged 801M+ downloads)
// Clean-room MIT implementation in pure CP.

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

    pub fn get(): i64 {
        return _.val;
    }

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
