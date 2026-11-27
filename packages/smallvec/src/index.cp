// @cap/smallvec — Zero-heap inline stack vector (SmallVecU64 & SmallVecU8)
// Clean-room MIT CP implementation of crates.io `smallvec` (1.7B+ downloads)
// Zero-heap, = undefined fast init, O(1) push/pop/swapRemove, insert/remove/dedup/reverse.

pub const SMALLVEC_U64_CAP: usize = 64;
pub const SMALLVEC_U8_CAP: usize = 256;

pub struct SmallPopU64 {
    pub ok: bool,
    pub value: u64,
}

/// Inline stack vector of up to 64 `u64` elements.
pub struct SmallVecU64 {
    data: [64]u64,
    used: usize,

    pub static fn new(): SmallVecU64 {
        return SmallVecU64 { data: undefined, used: 0 };
    }

    pub fn len(): usize { return _.used; }
    pub fn capacity(): usize { return SMALLVEC_U64_CAP; }
    pub fn isEmpty(): bool { return _.used == 0; }
    pub fn isFull(): bool { return _.used >= SMALLVEC_U64_CAP; }

    pub fn clear(*_) { _.used = 0; }

    pub fn truncate(*_, new_len: usize) {
        if (new_len < _.used) { _.used = new_len; }
    }

    pub fn get(idx: usize): SmallPopU64 {
        if (idx >= _.used) { return SmallPopU64 { ok: false, value: 0 }; }
        return SmallPopU64 { ok: true, value: _.data[idx] };
    }

    pub fn set(*_, idx: usize, val: u64): bool {
        if (idx >= _.used) { return false; }
        _.data[idx] = val;
        return true;
    }

    pub fn push(*_, val: u64): bool {
        if (_.used >= SMALLVEC_U64_CAP) { return false; }
        _.data[_.used] = val;
        _.used = _.used + 1;
        return true;
    }

    pub fn pop(*_): SmallPopU64 {
        if (_.used == 0) { return SmallPopU64 { ok: false, value: 0 }; }
        _.used = _.used - 1;
        return SmallPopU64 { ok: true, value: _.data[_.used] };
    }

    pub fn insert(*_, idx: usize, val: u64): bool {
        if (idx > _.used || _.used >= SMALLVEC_U64_CAP) { return false; }
        let k = _.used;
        while (k > idx) {
            _.data[k] = _.data[k - 1];
            k = k - 1;
        }
        _.data[idx] = val;
        _.used = _.used + 1;
        return true;
    }

    pub fn remove(*_, idx: usize): SmallPopU64 {
        if (idx >= _.used) { return SmallPopU64 { ok: false, value: 0 }; }
        let val = _.data[idx];
        let k = idx;
        while (k + 1 < _.used) {
            _.data[k] = _.data[k + 1];
            k = k + 1;
        }
        _.used = _.used - 1;
        return SmallPopU64 { ok: true, value: val };
    }

    /// O(1) unordered removal by swapping with the last element.
    pub fn swapRemove(*_, idx: usize): SmallPopU64 {
        if (idx >= _.used) { return SmallPopU64 { ok: false, value: 0 }; }
        let val = _.data[idx];
        _.used = _.used - 1;
        if (idx < _.used) {
            _.data[idx] = _.data[_.used];
        }
        return SmallPopU64 { ok: true, value: val };
    }

    pub fn extendFromSlice(*_, src: []const u64): bool {
        if (_.used + src.len > SMALLVEC_U64_CAP) { return false; }
        let i: usize = 0;
        while (i < src.len) {
            _.data[_.used + i] = src[i];
            i = i + 1;
        }
        _.used = _.used + src.len;
        return true;
    }

    /// Removes consecutive duplicate elements in-place.
    pub fn dedup(*_) {
        if (_.used <= 1) { return; }
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

    pub fn reverse(*_) {
        if (_.used <= 1) { return; }
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

    pub fn contains(needle: u64): bool {
        let i: usize = 0;
        while (i < _.used) {
            if (_.data[i] == needle) { return true; }
            i = i + 1;
        }
        return false;
    }
}

/// Inline stack byte vector of up to 256 `u8` elements.
pub struct SmallVecU8 {
    data: [256]u8,
    used: usize,

    pub static fn new(): SmallVecU8 {
        return SmallVecU8 { data: undefined, used: 0 };
    }

    pub fn len(): usize { return _.used; }
    pub fn capacity(): usize { return SMALLVEC_U8_CAP; }
    pub fn isEmpty(): bool { return _.used == 0; }
    pub fn clear(*_) { _.used = 0; }

    pub fn push(*_, b: u8): bool {
        if (_.used >= SMALLVEC_U8_CAP) { return false; }
        _.data[_.used] = b;
        _.used = _.used + 1;
        return true;
    }

    pub fn extendFromSlice(*_, src: []const u8): bool {
        if (_.used + src.len > SMALLVEC_U8_CAP) { return false; }
        let i: usize = 0;
        while (i < src.len) {
            _.data[_.used + i] = src[i];
            i = i + 1;
        }
        _.used = _.used + src.len;
        return true;
    }

    pub fn copyTo(out: []u8): usize {
        let n = if (_.used <= out.len) { _.used } else { out.len };
        let i: usize = 0;
        while (i < n) {
            out[i] = _.data[i];
            i = i + 1;
        }
        return n;
    }
}
