// @cap/bimap — Zero-heap Bidirectional 1-to-1 Map (u64 <-> u64)
// Clean-room MIT implementation in pure CP (no heap allocations, no FFI).

export const MAX_PAIRS: usize = 128;

export struct BiPair {
    left: u64,
    right: u64,
}

export struct BiMap {
    pairs: [128]BiPair,
    count: usize,

    pub static fn init(): BiMap {
        let pairs: [128]BiPair = undefined;
        return BiMap {
            pairs: pairs,
            count: 0,
        };
    }

    pub fn len(): usize {
        return _.count;
    }

    pub fn isEmpty(): bool {
        return _.count == 0;
    }

    pub fn capacity(): usize {
        return MAX_PAIRS;
    }

    pub fn findLeftIndex(left: u64): i64 {
        let i: usize = 0;
        while (i < _.count) {
            if (_.pairs[i].left == left) {
                return i as i64;
            }
            i += 1;
        }
        return -1;
    }

    pub fn findRightIndex(right: u64): i64 {
        let i: usize = 0;
        while (i < _.count) {
            if (_.pairs[i].right == right) {
                return i as i64;
            }
            i += 1;
        }
        return -1;
    }

    pub fn containsLeft(left: u64): bool {
        return _.findLeftIndex(left) >= 0;
    }

    pub fn containsRight(right: u64): bool {
        return _.findRightIndex(right) >= 0;
    }

    pub fn getByLeft(left: u64): i64 {
        let idx: i64 = _.findLeftIndex(left);
        if (idx < 0) {
            return -1;
        }
        return _.pairs[idx as usize].right as i64;
    }

    pub fn getByRight(right: u64): i64 {
        let idx: i64 = _.findRightIndex(right);
        if (idx < 0) {
            return -1;
        }
        return _.pairs[idx as usize].left as i64;
    }

    // Insert (left, right) overwriting any conflicting existing left or right bindings
    pub fn insert(*_, left: u64, right: u64): bool {
        let li: i64 = _.findLeftIndex(left);
        let ri: i64 = _.findRightIndex(right);
        if (li >= 0 && li == ri) {
            return true;
        }
        if (li >= 0 && ri >= 0) {
            // Update left's slot and remove right's old slot
            _.pairs[li as usize].right = right;
            let last: usize = _.count - 1;
            let r_idx: usize = ri as usize;
            if (r_idx != last) {
                _.pairs[r_idx] = _.pairs[last];
            }
            _.count = _.count - 1;
            return true;
        }
        if (li >= 0) {
            _.pairs[li as usize].right = right;
            return true;
        }
        if (ri >= 0) {
            _.pairs[ri as usize].left = left;
            return true;
        }
        if (_.count >= MAX_PAIRS) {
            return false;
        }
        _.pairs[_.count] = BiPair { left: left, right: right };
        _.count = _.count + 1;
        return true;
    }

    // Insert only if neither left nor right already exists
    pub fn insertNoOverwrite(*_, left: u64, right: u64): bool {
        if (_.findLeftIndex(left) >= 0 || _.findRightIndex(right) >= 0) {
            return false;
        }
        if (_.count >= MAX_PAIRS) {
            return false;
        }
        _.pairs[_.count] = BiPair { left: left, right: right };
        _.count = _.count + 1;
        return true;
    }

    pub fn removeByLeft(*_, left: u64): bool {
        let idx: i64 = _.findLeftIndex(left);
        if (idx < 0) {
            return false;
        }
        let last: usize = _.count - 1;
        let i: usize = idx as usize;
        if (i != last) {
            _.pairs[i] = _.pairs[last];
        }
        _.count = _.count - 1;
        return true;
    }

    pub fn removeByRight(*_, right: u64): bool {
        let idx: i64 = _.findRightIndex(right);
        if (idx < 0) {
            return false;
        }
        let last: usize = _.count - 1;
        let i: usize = idx as usize;
        if (i != last) {
            _.pairs[i] = _.pairs[last];
        }
        _.count = _.count - 1;
        return true;
    }

    pub fn clear(*_) {
        _.count = 0;
    }
}
