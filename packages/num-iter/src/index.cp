// @cap/num-iter — Zero-heap stepped numeric range iterators.
// Clean-room MIT implementation modeled on crates.io #140 `num-iter` v0.1 (850M+ downloads),
// reusing `@cap/num-integer` for exact integer division (`divCeil`, `divFloor`).

import { divCeil, divFloor } from "../../num-integer/src/index.cp";

const NITR_I64_MAX: i64 = 9223372036854775807;
const NITR_I64_MIN: i64 = -9223372036854775807 - 1;
const NITR_U64_MAX: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);

fn nitrAbsI64(v: i64): u64 {
    if (v >= 0) { return v as u64; }
    if (v == NITR_I64_MIN) { return (1 as u64) << 63; }
    return (-v) as u64;
}

fn nitrCheckedAddI64(a: i64, b: i64): i64? {
    if (b > 0 && a > NITR_I64_MAX - b) { return null; }
    if (b < 0 && a < NITR_I64_MIN - b) { return null; }
    return a + b;
}

fn nitrDiffU64(hi: i64, lo: i64): u64 {
    if ((hi >= 0) == (lo >= 0)) {
        return (hi - lo) as u64;
    }
    let pos_part: u64 = hi as u64;
    let neg_part: u64 = nitrAbsI64(lo);
    return pos_part + neg_part;
}

fn nitrAdvanceSigned(state: i64, step: i64, n: usize): i64 {
    let step_abs: u64 = nitrAbsI64(step);
    let offset: u64 = (n as u64) * step_abs;
    let min_mag: u64 = (1 as u64) << 63;
    if (step > 0) {
        if (state >= 0) {
            return ((state as u64) + offset) as i64;
        }
        let neg_mag: u64 = nitrAbsI64(state);
        if (offset >= neg_mag) {
            return (offset - neg_mag) as i64;
        }
        return -((neg_mag - offset) as i64);
    }
    if (state <= 0) {
        let mag: u64 = nitrAbsI64(state) + offset;
        if (mag == min_mag) { return NITR_I64_MIN; }
        return -(mag as i64);
    }
    let pos_mag: u64 = state as u64;
    if (pos_mag >= offset) {
        return (pos_mag - offset) as i64;
    }
    let rem_neg: u64 = offset - pos_mag;
    if (rem_neg == min_mag) { return NITR_I64_MIN; }
    return -(rem_neg as i64);
}

/// Half-open stepped iterator over `[start, stop)` with non-zero signed step.
pub struct RangeStep {
    pub state: i64,
    pub stop: i64,
    pub step: i64,
    pub done: bool,

    pub fn isDone(): bool {
        if (_.done || _.step == 0) { return true; }
        if (_.step > 0) { return _.state >= _.stop; }
        return _.state <= _.stop;
    }

    pub fn remaining(): usize {
        if (_.isDone()) { return 0; }
        if (_.step > 0) {
            if (_.state >= 0 || _.stop <= NITR_I64_MAX + _.state) {
                return divCeil(_.stop - _.state, _.step) as usize;
            }
            let diff: u64 = nitrDiffU64(_.stop, _.state);
            let s: u64 = _.step as u64;
            return (((diff - 1) / s) + 1) as usize;
        }
        if ((_.state <= 0 || _.stop >= NITR_I64_MIN + _.state) && _.step != NITR_I64_MIN) {
            return divCeil(_.stop - _.state, _.step) as usize;
        }
        let diff: u64 = nitrDiffU64(_.state, _.stop);
        let s: u64 = nitrAbsI64(_.step);
        return (((diff - 1) / s) + 1) as usize;
    }

    pub fn next(*_): i64? {
        if (_.isDone()) {
            _.done = true;
            return null;
        }
        let val: i64 = _.state;
        let next_val = nitrCheckedAddI64(_.state, _.step);
        if (next_val == null) {
            _.done = true;
        } else {
            _.state = next_val ?? 0;
            if ((_.step > 0 && _.state >= _.stop) || (_.step < 0 && _.state <= _.stop)) {
                _.done = true;
            }
        }
        return val;
    }

    pub fn nth(*_, n: usize): i64? {
        let rem: usize = _.remaining();
        if (n >= rem) {
            _.done = true;
            return null;
        }
        _.state = nitrAdvanceSigned(_.state, _.step, n);
        return _.next();
    }
}

/// Closed stepped iterator over `[start, stop]` with non-zero signed step and overflow protection.
pub struct RangeStepInclusive {
    pub state: i64,
    pub stop: i64,
    pub step: i64,
    pub done: bool,

    pub fn isDone(): bool {
        if (_.done || _.step == 0) { return true; }
        if (_.step > 0) { return _.state > _.stop; }
        return _.state < _.stop;
    }

    pub fn remaining(): usize {
        if (_.isDone()) { return 0; }
        if (_.step > 0) {
            if (_.state >= 0 || _.stop <= NITR_I64_MAX + _.state) {
                return (divFloor(_.stop - _.state, _.step) as usize) + 1;
            }
            let diff: u64 = nitrDiffU64(_.stop, _.state);
            let s: u64 = _.step as u64;
            return ((diff / s) + 1) as usize;
        }
        if ((_.state <= 0 || _.stop >= NITR_I64_MIN + _.state) && _.step != NITR_I64_MIN) {
            return (divFloor(_.stop - _.state, _.step) as usize) + 1;
        }
        let diff: u64 = nitrDiffU64(_.state, _.stop);
        let s: u64 = nitrAbsI64(_.step);
        return ((diff / s) + 1) as usize;
    }

    pub fn next(*_): i64? {
        if (_.isDone()) {
            _.done = true;
            return null;
        }
        let val: i64 = _.state;
        let next_val = nitrCheckedAddI64(_.state, _.step);
        if (next_val == null) {
            _.done = true;
        } else {
            _.state = next_val ?? 0;
            if ((_.step > 0 && _.state > _.stop) || (_.step < 0 && _.state < _.stop)) {
                _.done = true;
            }
        }
        return val;
    }
}

/// Half-open stepped iterator over unsigned `[start, stop)` with positive `u64` step.
pub struct RangeStepU64 {
    pub state: u64,
    pub stop: u64,
    pub step: u64,
    pub done: bool,

    pub fn isDone(): bool {
        return _.done || _.step == 0 || _.state >= _.stop;
    }

    pub fn remaining(): usize {
        if (_.isDone()) { return 0; }
        let diff: u64 = _.stop - _.state;
        return (((diff - 1) / _.step) + 1) as usize;
    }

    pub fn next(*_): u64? {
        if (_.isDone()) {
            _.done = true;
            return null;
        }
        let val: u64 = _.state;
        if (_.state > NITR_U64_MAX - _.step) {
            _.done = true;
        } else {
            _.state = _.state + _.step;
            if (_.state >= _.stop) {
                _.done = true;
            }
        }
        return val;
    }

    pub fn nth(*_, n: usize): u64? {
        let rem: usize = _.remaining();
        if (n >= rem) {
            _.done = true;
            return null;
        }
        _.state = _.state + ((n as u64) * _.step);
        return _.next();
    }
}

/// Constructs a half-open `RangeStep` over `[start, stop)` with step `step`.
pub fn rangeStep(start: i64, stop: i64, step: i64): RangeStep {
    let is_empty: bool = step == 0 || (step > 0 && start >= stop) || (step < 0 && start <= stop);
    return RangeStep {
        state: start,
        stop: stop,
        step: step,
        done: is_empty,
    };
}

/// Constructs a closed `RangeStepInclusive` over `[start, stop]` with step `step`.
pub fn rangeStepInclusive(start: i64, stop: i64, step: i64): RangeStepInclusive {
    let is_empty: bool = step == 0 || (step > 0 && start > stop) || (step < 0 && start < stop);
    return RangeStepInclusive {
        state: start,
        stop: stop,
        step: step,
        done: is_empty,
    };
}

/// Constructs a half-open `RangeStepU64` over `[start, stop)` with step `step`.
pub fn rangeStepU64(start: u64, stop: u64, step: u64): RangeStepU64 {
    let is_empty: bool = step == 0 || start >= stop;
    return RangeStepU64 {
        state: start,
        stop: stop,
        step: step,
        done: is_empty,
    };
}

/// Drains up to `out.len` elements from `r` into `out`, returning the number of elements written.
pub fn collectRangeStep(r: *RangeStep, out: []i64): usize {
    let count: usize = 0;
    while (count < out.len) {
        let v = r.next();
        if (v == null) { break; }
        out[count] = v ?? 0;
        count = count + 1;
    }
    return count;
}
