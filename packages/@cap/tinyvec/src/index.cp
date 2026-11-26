// @cap/tinyvec — Zero-heap 100%-safe inline stack vectors modeled on Rust's `tinyvec` crate.
// Clean-room MIT implementation providing `TinyVecI64` (cap 64) and `TinyVecU32` (cap 64).

pub const TV_CAP: usize = 64;

/// Inline stack vector of up to 64 `i64` elements (`tinyvec::ArrayVec<[i64; 64]>`).
pub struct TinyVecI64 {
    pub data: [64]i64,
    pub used: usize,

    pub static fn new(): TinyVecI64 {
        let arr: [64]i64 = undefined;
        let i: usize = 0;
        while (i < TV_CAP) { arr[i] = 0; i = i + 1; }
        return TinyVecI64 { data: arr, used: 0 };
    }

    pub fn len(): usize { return _.used; }
    pub fn capacity(): usize { return TV_CAP; }
    pub fn isEmpty(): bool { return _.used == 0; }
    pub fn isFull(): bool { return _.used >= TV_CAP; }

    pub fn get(idx: usize): i64? {
        if (idx >= _.used) { return null; }
        return _.data[idx];
    }

    pub fn first(): i64? {
        if (_.used == 0) { return null; }
        return _.data[0];
    }

    pub fn last(): i64? {
        if (_.used == 0) { return null; }
        return _.data[_.used - 1];
    }

    pub fn push(*_, val: i64): bool {
        if (_.used >= TV_CAP) { return false; }
        _.data[_.used] = val;
        _.used = _.used + 1;
        return true;
    }

    pub fn pop(*_): i64? {
        if (_.used == 0) { return null; }
        _.used = _.used - 1;
        return _.data[_.used];
    }

    pub fn insert(*_, idx: usize, val: i64): bool {
        if (_.used >= TV_CAP || idx > _.used) { return false; }
        let i = _.used;
        while (i > idx) {
            _.data[i] = _.data[i - 1];
            i = i - 1;
        }
        _.data[idx] = val;
        _.used = _.used + 1;
        return true;
    }

    pub fn remove(*_, idx: usize): i64? {
        if (idx >= _.used) { return null; }
        let out = _.data[idx];
        let i = idx;
        while (i + 1 < _.used) {
            _.data[i] = _.data[i + 1];
            i = i + 1;
        }
        _.used = _.used - 1;
        return out;
    }

    pub fn swapRemove(*_, idx: usize): i64? {
        if (idx >= _.used) { return null; }
        let out = _.data[idx];
        _.used = _.used - 1;
        if (idx < _.used) {
            _.data[idx] = _.data[_.used];
        }
        return out;
    }

    pub fn truncate(*_, new_len: usize) {
        if (new_len < _.used) { _.used = new_len; }
    }

    pub fn clear(*_) {
        _.used = 0;
    }

    pub fn reverse(*_) {
        if (_.used < 2) { return; }
        let lo: usize = 0;
        let hi: usize = _.used - 1;
        while (lo < hi) {
            let tmp = _.data[lo];
            _.data[lo] = _.data[hi];
            _.data[hi] = tmp;
            lo = lo + 1;
            hi = hi - 1;
        }
    }

    /// Removes consecutive duplicate elements in-place.
    pub fn dedup(*_) {
        if (_.used < 2) { return; }
        let write: usize = 1;
        let read: usize = 1;
        while (read < _.used) {
            if (_.data[read] != _.data[write - 1]) {
                _.data[write] = _.data[read];
                write = write + 1;
            }
            read = read + 1;
        }
        _.used = write;
    }

    pub fn extendFromSlice(*_, src: []const i64): usize {
        let added: usize = 0;
        let i: usize = 0;
        while (i < src.len && _.used < TV_CAP) {
            _.data[_.used] = src[i];
            _.used = _.used + 1;
            added = added + 1;
            i = i + 1;
        }
        return added;
    }

    /// Binary search on a sorted `TinyVecI64`; returns index if found or `null`.
    pub fn binarySearch(target: i64): usize? {
        let lo: usize = 0;
        let hi: usize = _.used;
        while (lo < hi) {
            let mid = lo + ((hi - lo) >> 1);
            let v = _.data[mid];
            if (v == target) { return mid; }
            if (v < target) { lo = mid + 1; } else { hi = mid; }
        }
        return null;
    }

    pub fn sum(): i64 {
        let s: i64 = 0;
        let i: usize = 0;
        while (i < _.used) {
            s = s + _.data[i];
            i = i + 1;
        }
        return s;
    }
}

/// Inline stack vector of up to 64 `u32` elements (`tinyvec::ArrayVec<[u32; 64]>`).
pub struct TinyVecU32 {
    pub data: [64]u32,
    pub used: usize,

    pub static fn new(): TinyVecU32 {
        let arr: [64]u32 = undefined;
        let i: usize = 0;
        while (i < TV_CAP) { arr[i] = 0; i = i + 1; }
        return TinyVecU32 { data: arr, used: 0 };
    }

    pub fn len(): usize { return _.used; }
    pub fn isEmpty(): bool { return _.used == 0; }

    pub fn push(*_, val: u32): bool {
        if (_.used >= TV_CAP) { return false; }
        _.data[_.used] = val;
        _.used = _.used + 1;
        return true;
    }

    pub fn pop(*_): u32? {
        if (_.used == 0) { return null; }
        _.used = _.used - 1;
        return _.data[_.used];
    }

    pub fn get(idx: usize): u32? {
        if (idx >= _.used) { return null; }
        return _.data[idx];
    }

    pub fn swapRemove(*_, idx: usize): u32? {
        if (idx >= _.used) { return null; }
        let out = _.data[idx];
        _.used = _.used - 1;
        if (idx < _.used) { _.data[idx] = _.data[_.used]; }
        return out;
    }

    pub fn clear(*_) { _.used = 0; }
}
