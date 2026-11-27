pub const THIN_VEC_CAP: usize = 64;

pub struct ThinVecI64 {
    pub len_val: usize,
    pub data: [64]i64,

    pub static fn new(): ThinVecI64 {
        return ThinVecI64 { len_val: 0, data: undefined };
    }

    pub static fn fromSlice(src: []i64): ThinVecI64 {
        let v = ThinVecI64.new();
        let n = if (src.len > THIN_VEC_CAP) { THIN_VEC_CAP } else { src.len };
        if (n > 0) {
            @memcpy(v.data[0..n], src[0..n]);
        }
        v.len_val = n;
        return v;
    }

    pub fn len(): usize {
        return _.len_val;
    }

    pub fn capacity(): usize {
        return THIN_VEC_CAP;
    }

    pub fn isEmpty(): bool {
        return _.len_val == 0;
    }

    pub fn get(idx: usize): i64? {
        if (idx >= _.len_val) { return null; }
        return _.data[idx];
    }

    pub fn first(): i64? {
        if (_.len_val == 0) { return null; }
        return _.data[0];
    }

    pub fn last(): i64? {
        if (_.len_val == 0) { return null; }
        return _.data[_.len_val - 1];
    }

    pub fn push(*_, val: i64): bool {
        if (_.len_val >= THIN_VEC_CAP) { return false; }
        _.data[_.len_val] = val;
        _.len_val = _.len_val + 1;
        return true;
    }

    pub fn pop(*_): i64? {
        if (_.len_val == 0) { return null; }
        _.len_val = _.len_val - 1;
        return _.data[_.len_val];
    }

    pub fn insert(*_, idx: usize, val: i64): bool {
        if (idx > _.len_val || _.len_val >= THIN_VEC_CAP) { return false; }
        let i = _.len_val;
        while (i > idx) {
            _.data[i] = _.data[i - 1];
            i = i - 1;
        }
        _.data[idx] = val;
        _.len_val = _.len_val + 1;
        return true;
    }

    pub fn remove(*_, idx: usize): i64? {
        if (idx >= _.len_val) { return null; }
        let removed = _.data[idx];
        let i = idx;
        while (i + 1 < _.len_val) {
            _.data[i] = _.data[i + 1];
            i = i + 1;
        }
        _.len_val = _.len_val - 1;
        return removed;
    }

    pub fn swapRemove(*_, idx: usize): i64? {
        if (idx >= _.len_val) { return null; }
        let removed = _.data[idx];
        _.len_val = _.len_val - 1;
        if (idx < _.len_val) {
            _.data[idx] = _.data[_.len_val];
        }
        return removed;
    }

    pub fn truncate(*_, new_len: usize) {
        if (new_len < _.len_val) {
            _.len_val = new_len;
        }
    }

    pub fn clear(*_) {
        _.len_val = 0;
    }

    pub fn dedup(*_): usize {
        if (_.len_val <= 1) { return 0; }
        let write: usize = 1;
        let read: usize = 1;
        let removed: usize = 0;
        while (read < _.len_val) {
            if (_.data[read] != _.data[write - 1]) {
                _.data[write] = _.data[read];
                write = write + 1;
            } else {
                removed = removed + 1;
            }
            read = read + 1;
        }
        _.len_val = write;
        return removed;
    }

    pub fn extendFromSlice(*_, src: []i64): usize {
        let avail = THIN_VEC_CAP - _.len_val;
        let take = if (src.len > avail) { avail } else { src.len };
        if (take > 0) {
            @memcpy(_.data[_.len_val.._.len_val + take], src[0..take]);
            _.len_val = _.len_val + take;
        }
        return take;
    }
}
