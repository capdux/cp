// @cap/segment-tree — Zero-allocation iterative 2*N Segment Tree (sum / min / max) for CP
// Clean-room MIT implementation inspired by Rust `segment-tree`.

const MAX_LEN: usize = 256;
const TREE_CAP: usize = 512;

pub struct RangeStats {
    pub sum: i64,
    pub min_val: i64,
    pub max_val: i64,
    pub valid: bool,
}

fn minI64(a: i64, b: i64) -> i64 {
    if (a < b) {
        return a;
    }
    return b;
}

fn maxI64(a: i64, b: i64) -> i64 {
    if (a > b) {
        return a;
    }
    return b;
}

pub struct SegmentTree {
    pub size: usize,
    pub sums: [TREE_CAP]i64,
    pub mins: [TREE_CAP]i64,
    pub maxs: [TREE_CAP]i64,

    pub static fn init(n: usize) -> SegmentTree {
        let clamped = n;
        if (clamped > MAX_LEN) {
            clamped = MAX_LEN;
        }
        let s: [TREE_CAP]i64 = undefined;
        let mn: [TREE_CAP]i64 = undefined;
        let mx: [TREE_CAP]i64 = undefined;
        let i: usize = 0;
        while (i < TREE_CAP) {
            s[i] = 0 as i64;
            mn[i] = 0 as i64;
            mx[i] = 0 as i64;
            i += 1;
        }
        return SegmentTree{
            size: clamped,
            sums: s,
            mins: mn,
            maxs: mx,
        };
    }

    pub static fn fromSlice(values: []const i64) -> SegmentTree {
        let n = values.len;
        if (n > MAX_LEN) {
            n = MAX_LEN;
        }
        let st = SegmentTree.init(n);
        let i: usize = 0;
        while (i < n) {
            const v = values[i];
            st.sums[n + i] = v;
            st.mins[n + i] = v;
            st.maxs[n + i] = v;
            i += 1;
        }
        // O(N) bottom-up build
        if (n > (1 as usize)) {
            let p = n - (1 as usize);
            while (p >= (1 as usize)) {
                const l = p * (2 as usize);
                const r = l + (1 as usize);
                st.sums[p] = st.sums[l] + st.sums[r];
                st.mins[p] = minI64(st.mins[l], st.mins[r]);
                st.maxs[p] = maxI64(st.maxs[l], st.maxs[r]);
                p -= 1;
            }
        }
        return st;
    }

    pub fn len() -> usize {
        return _.size;
    }

    pub fn get(idx: usize) -> i64 {
        if (idx >= _.size) {
            return 0 as i64;
        }
        return _.sums[_.size + idx];
    }

    pub fn set(*_, idx: usize, value: i64) -> bool {
        if (idx >= _.size) {
            return false;
        }
        let p = _.size + idx;
        _.sums[p] = value;
        _.mins[p] = value;
        _.maxs[p] = value;
        while (p > (1 as usize)) {
            p = p / (2 as usize);
            const l = p * (2 as usize);
            const r = l + (1 as usize);
            _.sums[p] = _.sums[l] + _.sums[r];
            _.mins[p] = minI64(_.mins[l], _.mins[r]);
            _.maxs[p] = maxI64(_.maxs[l], _.maxs[r]);
        }
        return true;
    }

    pub fn add(*_, idx: usize, delta: i64) -> bool {
        if (idx >= _.size) {
            return false;
        }
        const nextVal = _.sums[_.size + idx] + delta;
        return _.set(idx, nextVal);
    }

    pub fn query(start: usize, end_exclusive: usize) -> RangeStats {
        if (_.size == (0 as usize) || start >= end_exclusive || start >= _.size) {
            return RangeStats{
                sum: 0 as i64,
                min_val: 0 as i64,
                max_val: 0 as i64,
                valid: false,
            };
        }
        let end = end_exclusive;
        if (end > _.size) {
            end = _.size;
        }
        let l = _.size + start;
        let r = _.size + end;
        let hasAny = false;
        let accSum: i64 = 0;
        let accMin: i64 = 0;
        let accMax: i64 = 0;

        while (l < r) {
            if ((l & (1 as usize)) != (0 as usize)) {
                accSum += _.sums[l];
                if (!hasAny) {
                    accMin = _.mins[l];
                    accMax = _.maxs[l];
                    hasAny = true;
                } else {
                    accMin = minI64(accMin, _.mins[l]);
                    accMax = maxI64(accMax, _.maxs[l]);
                }
                l += 1;
            }
            if ((r & (1 as usize)) != (0 as usize)) {
                r -= 1;
                accSum += _.sums[r];
                if (!hasAny) {
                    accMin = _.mins[r];
                    accMax = _.maxs[r];
                    hasAny = true;
                } else {
                    accMin = minI64(accMin, _.mins[r]);
                    accMax = maxI64(accMax, _.maxs[r]);
                }
            }
            l = l / (2 as usize);
            r = r / (2 as usize);
        }

        return RangeStats{
            sum: accSum,
            min_val: accMin,
            max_val: accMax,
            valid: hasAny,
        };
    }

    pub fn rangeSum(start: usize, end_exclusive: usize) -> i64 {
        const st = _.query(start, end_exclusive);
        if (!st.valid) {
            return 0 as i64;
        }
        return st.sum;
    }

    pub fn rangeMin(start: usize, end_exclusive: usize) -> i64 {
        const st = _.query(start, end_exclusive);
        if (!st.valid) {
            return 0 as i64;
        }
        return st.min_val;
    }

    pub fn rangeMax(start: usize, end_exclusive: usize) -> i64 {
        const st = _.query(start, end_exclusive);
        if (!st.valid) {
            return 0 as i64;
        }
        return st.max_val;
    }
}

export {
    RangeStats,
    SegmentTree,
};
