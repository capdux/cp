// @cap/fenwick — Zero-allocation Binary Indexed Tree (Fenwick Tree) for CP
// Clean-room MIT implementation inspired by Rust `fenwick`.

const MAX_SIZE: usize = 256;
const TREE_LEN: usize = 257;

fn lowbit(x: usize) -> usize {
    if (x == 0) {
        return 0 as usize;
    }
    return x - (x & (x - (1 as usize)));
}

pub struct FenwickTree {
    pub tree: [TREE_LEN]i64,
    pub length: usize,

    pub static fn init(size: usize) -> FenwickTree {
        const clamped = if (size > MAX_SIZE) { MAX_SIZE } else { size };
        let arr: [TREE_LEN]i64 = undefined;
        let i: usize = 0;
        while (i < TREE_LEN) {
            arr[i] = 0 as i64;
            i += 1;
        }
        return FenwickTree{
            tree: arr,
            length: clamped,
        };
    }

    pub static fn fromSlice(values: []const i64) -> FenwickTree {
        const n = if (values.len > MAX_SIZE) { MAX_SIZE } else { values.len };
        let arr: [TREE_LEN]i64 = undefined;
        let i: usize = 0;
        while (i < TREE_LEN) {
            arr[i] = 0 as i64;
            i += 1;
        }
        let k: usize = 0;
        while (k < n) {
            arr[k + 1] = values[k];
            k += 1;
        }
        // O(N) linear-time Fenwick tree construction
        let idx: usize = 1;
        while (idx <= n) {
            const parent = idx + lowbit(idx);
            if (parent <= n) {
                arr[parent] += arr[idx];
            }
            idx += 1;
        }
        return FenwickTree{
            tree: arr,
            length: n,
        };
    }

    pub fn len() -> usize {
        return _.length;
    }

    pub fn add(*_, idx: usize, delta: i64) -> bool {
        if (idx >= _.length) {
            return false;
        }
        let i = idx + (1 as usize);
        while (i <= _.length) {
            _.tree[i] += delta;
            i += lowbit(i);
        }
        return true;
    }

    pub fn prefixSum(end_exclusive: usize) -> i64 {
        const limit = if (end_exclusive > _.length) { _.length } else { end_exclusive };
        let sum: i64 = 0;
        let i = limit;
        while (i > 0) {
            sum += _.tree[i];
            i -= lowbit(i);
        }
        return sum;
    }

    pub fn rangeSum(start: usize, end_exclusive: usize) -> i64 {
        if (start >= end_exclusive || start >= _.length) {
            return 0 as i64;
        }
        return _.prefixSum(end_exclusive) - _.prefixSum(start);
    }

    pub fn get(idx: usize) -> i64 {
        if (idx >= _.length) {
            return 0 as i64;
        }
        return _.rangeSum(idx, idx + (1 as usize));
    }

    pub fn set(*_, idx: usize, value: i64) -> bool {
        if (idx >= _.length) {
            return false;
        }
        const cur = _.get(idx);
        return _.add(idx, value - cur);
    }

    pub fn total() -> i64 {
        return _.prefixSum(_.length);
    }

    // O(log N) binary lifting for non-negative weights:
    // returns smallest 0-based idx such that prefixSum(idx + 1) >= target, or _.length if total < target.
    pub fn lowerBound(target: i64) -> usize {
        if (_.length == 0 || target <= (0 as i64)) {
            return 0 as usize;
        }
        let pos: usize = 0;
        let acc: i64 = 0;
        let step: usize = 256;
        while (step > 0) {
            const next = pos + step;
            if (next <= _.length && (acc + _.tree[next]) < target) {
                acc += _.tree[next];
                pos = next;
            }
            step = step / (2 as usize);
        }
        return pos;
    }

    pub fn reset(*_) {
        let i: usize = 0;
        while (i <= _.length) {
            _.tree[i] = 0 as i64;
            i += 1;
        }
    }
}

export {
    FenwickTree,
};
