// @cap/ordered-multimap — Zero-heap insertion-ordered multimap in pure CP.

pub const OMM_CAP: usize = 128;

fn ommEmptyStrings(): [OMM_CAP]string {
    let arr: [OMM_CAP]string = undefined;
    let i: usize = 0;
    while (i < OMM_CAP) : (i += 1) {
        arr[i] = "";
    }
    return arr;
}

fn ommStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

pub struct OrderedMultiMap {
    keys: [OMM_CAP]string,
    vals: [OMM_CAP]string,
    count: usize,

    static fn new(): OrderedMultiMap {
        return OrderedMultiMap{
            keys: ommEmptyStrings(),
            vals: ommEmptyStrings(),
            count: 0,
        };
    }

    fn append(*_, key: string, val: string): bool {
        if (_.count >= OMM_CAP) {
            return false;
        }
        _.keys[_.count] = key;
        _.vals[_.count] = val;
        _.count += 1;
        return true;
    }

    fn insert(*_, key: string, val: string): bool {
        let found = false;
        let readIdx: usize = 0;
        let writeIdx: usize = 0;
        while (readIdx < _.count) : (readIdx += 1) {
            if (ommStrEq(_.keys[readIdx], key)) {
                if (!found) {
                    _.keys[writeIdx] = key;
                    _.vals[writeIdx] = val;
                    writeIdx += 1;
                    found = true;
                }
            } else {
                if (writeIdx != readIdx) {
                    _.keys[writeIdx] = _.keys[readIdx];
                    _.vals[writeIdx] = _.vals[readIdx];
                }
                writeIdx += 1;
            }
        }
        _.count = writeIdx;
        if (found) {
            return true;
        }
        if (_.count >= OMM_CAP) {
            return false;
        }
        _.keys[_.count] = key;
        _.vals[_.count] = val;
        _.count += 1;
        return true;
    }

    fn getFirst(key: string): string? {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (ommStrEq(_.keys[i], key)) {
                return _.vals[i];
            }
        }
        return null;
    }

    fn getAll(key: string, out: []string): usize {
        let written: usize = 0;
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (ommStrEq(_.keys[i], key)) {
                if (written < out.len) {
                    out[written] = _.vals[i];
                    written += 1;
                }
            }
        }
        return written;
    }

    fn removeAll(*_, key: string): usize {
        let readIdx: usize = 0;
        let writeIdx: usize = 0;
        let removed: usize = 0;
        while (readIdx < _.count) : (readIdx += 1) {
            if (ommStrEq(_.keys[readIdx], key)) {
                removed += 1;
            } else {
                if (writeIdx != readIdx) {
                    _.keys[writeIdx] = _.keys[readIdx];
                    _.vals[writeIdx] = _.vals[readIdx];
                }
                writeIdx += 1;
            }
        }
        _.count = writeIdx;
        return removed;
    }

    fn containsKey(key: string): bool {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (ommStrEq(_.keys[i], key)) {
                return true;
            }
        }
        return false;
    }

    fn len(): usize {
        return _.count;
    }

    fn keysLen(): usize {
        let distinct: usize = 0;
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            let seen = false;
            let j: usize = 0;
            while (j < i) : (j += 1) {
                if (ommStrEq(_.keys[j], _.keys[i])) {
                    seen = true;
                    break;
                }
            }
            if (!seen) {
                distinct += 1;
            }
        }
        return distinct;
    }
}
