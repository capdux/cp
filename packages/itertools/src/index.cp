// @cap/itertools — Pure CP zero-allocation iterator combinators & sequence algorithms
// Clean-room MIT implementation (Rust `itertools` core algorithms parity).

pub struct MinMaxResult {
    pub empty: bool,
    pub min: i64,
    pub max: i64,
}

pub struct PartitionResult {
    pub true_count: usize,
    pub false_count: usize,
}

pub struct CartesianPair {
    pub found: bool,
    pub first: i64,
    pub second: i64,
}

fn itCopyBytes(dst: []u8, pos: *usize, src: []const u8): bool {
    if (pos.* + src.len > dst.len) {
        return false;
    }
    let i: usize = 0;
    while (i < src.len) {
        dst[pos.* + i] = src[i];
        i = i + 1;
    }
    pos.* = pos.* + src.len;
    return true;
}

/// Computes minimum and maximum values in a single pass.
pub fn minmax(slice: []const i64): MinMaxResult {
    if (slice.len == 0) {
        return MinMaxResult { empty: true, min: 0, max: 0 };
    }
    let lo = slice[0];
    let hi = slice[0];
    let i: usize = 1;
    while (i < slice.len) {
        let v = slice[i];
        if (v < lo) {
            lo = v;
        }
        if (v > hi) {
            hi = v;
        }
        i = i + 1;
    }
    return MinMaxResult { empty: false, min: lo, max: hi };
}

/// Returns true if `slice` is sorted in non-decreasing (ascending) order.
pub fn isSorted(slice: []const i64): bool {
    if (slice.len <= 1) {
        return true;
    }
    let i: usize = 1;
    while (i < slice.len) {
        if (slice[i] < slice[i - 1]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// Returns true if `slice` is sorted in non-increasing (descending) order.
pub fn isSortedDesc(slice: []const i64): bool {
    if (slice.len <= 1) {
        return true;
    }
    let i: usize = 1;
    while (i < slice.len) {
        if (slice[i] > slice[i - 1]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// Returns true if all elements in `slice` are equal (empty or single-element returns true).
pub fn allEqual(slice: []const i64): bool {
    if (slice.len <= 1) {
        return true;
    }
    let first = slice[0];
    let i: usize = 1;
    while (i < slice.len) {
        if (slice[i] != first) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// Order-preserving deduplication: keeps the first occurrence of each distinct value.
/// Writes unique values into `out` and returns the count written.
pub fn uniqueI64(src: []const i64, out: []i64): usize {
    let written: usize = 0;
    let i: usize = 0;
    while (i < src.len && written < out.len) {
        let v = src[i];
        let seen = false;
        let j: usize = 0;
        while (j < written) {
            if (out[j] == v) {
                seen = true;
                break;
            }
            j = j + 1;
        }
        if (!seen) {
            out[written] = v;
            written = written + 1;
        }
        i = i + 1;
    }
    return written;
}

/// Removes consecutive duplicate values, keeping one copy per adjacent run.
pub fn dedupConsecutive(src: []const i64, out: []i64): usize {
    if (src.len == 0 || out.len == 0) {
        return 0;
    }
    out[0] = src[0];
    let written: usize = 1;
    let i: usize = 1;
    while (i < src.len && written < out.len) {
        if (src[i] != out[written - 1]) {
            out[written] = src[i];
            written = written + 1;
        }
        i = i + 1;
    }
    return written;
}

/// Interleaves `a` and `b` (`a[0], b[0], a[1], b[1], ...`), appending the remaining tail
/// of whichever slice is longer.
pub fn interleave(a: []const i64, b: []const i64, out: []i64): usize {
    let ia: usize = 0;
    let ib: usize = 0;
    let pos: usize = 0;
    while ((ia < a.len || ib < b.len) && pos < out.len) {
        if (ia < a.len && pos < out.len) {
            out[pos] = a[ia];
            pos = pos + 1;
            ia = ia + 1;
        }
        if (ib < b.len && pos < out.len) {
            out[pos] = b[ib];
            pos = pos + 1;
            ib = ib + 1;
        }
    }
    return pos;
}

/// Inserts `sep` between every pair of adjacent elements of `src`.
pub fn intersperse(src: []const i64, sep: i64, out: []i64): usize {
    if (src.len == 0 || out.len == 0) {
        return 0;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < src.len && pos < out.len) {
        if (i > 0) {
            out[pos] = sep;
            pos = pos + 1;
            if (pos >= out.len) {
                break;
            }
        }
        out[pos] = src[i];
        pos = pos + 1;
        i = i + 1;
    }
    return pos;
}

/// Stable merge of two ascending sorted slices `a` and `b` into `out` (`itertools::merge` parity).
pub fn mergeSorted(a: []const i64, b: []const i64, out: []i64): usize {
    let ia: usize = 0;
    let ib: usize = 0;
    let pos: usize = 0;
    while (ia < a.len && ib < b.len && pos < out.len) {
        if (a[ia] <= b[ib]) {
            out[pos] = a[ia];
            ia = ia + 1;
        } else {
            out[pos] = b[ib];
            ib = ib + 1;
        }
        pos = pos + 1;
    }
    while (ia < a.len && pos < out.len) {
        out[pos] = a[ia];
        ia = ia + 1;
        pos = pos + 1;
    }
    while (ib < b.len && pos < out.len) {
        out[pos] = b[ib];
        ib = ib + 1;
        pos = pos + 1;
    }
    return pos;
}

/// Zero-heap string join (`itertools::join` parity).
/// Writes joined bytes into `out` and returns total bytes written (stops safely if `out` is full).
pub fn joinStrings(items: []const string, sep: string, out: []u8): usize {
    if (items.len == 0) {
        return 0;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < items.len) {
        if (i > 0) {
            if (!itCopyBytes(out, &pos, sep)) {
                return pos;
            }
        }
        if (!itCopyBytes(out, &pos, items[i])) {
            return pos;
        }
        i = i + 1;
    }
    return pos;
}

/// Partitions `src` into `out_true` (`>= threshold`) and `out_false` (`< threshold`).
pub fn partitionByThreshold(src: []const i64, threshold: i64, out_true: []i64, out_false: []i64): PartitionResult {
    let tc: usize = 0;
    let fc: usize = 0;
    let i: usize = 0;
    while (i < src.len) {
        if (src[i] >= threshold) {
            if (tc < out_true.len) {
                out_true[tc] = src[i];
                tc = tc + 1;
            }
        } else {
            if (fc < out_false.len) {
                out_false[fc] = src[i];
                fc = fc + 1;
            }
        }
        i = i + 1;
    }
    return PartitionResult { true_count: tc, false_count: fc };
}

/// Total number of pairs in the Cartesian product of slices of lengths `a_len` and `b_len`.
pub fn cartesianProductCount(a_len: usize, b_len: usize): usize {
    return a_len * b_len;
}

/// O(1) zero-heap random access to the `idx`-th pair of the Cartesian product `a x b`.
pub fn nthCartesianPair(a: []const i64, b: []const i64, idx: usize): CartesianPair {
    if (a.len == 0 || b.len == 0) {
        return CartesianPair { found: false, first: 0, second: 0 };
    }
    let total = a.len * b.len;
    if (idx >= total) {
        return CartesianPair { found: false, first: 0, second: 0 };
    }
    return CartesianPair {
        found: true,
        first: a[idx / b.len],
        second: b[idx % b.len]
    };
}

/// Number of chunks of size `chunk_size` needed to cover `total_len` items (0 if either is 0).
pub fn chunkCount(total_len: usize, chunk_size: usize): usize {
    if (total_len == 0 || chunk_size == 0) {
        return 0;
    }
    return (total_len + chunk_size - 1) / chunk_size;
}

/// Number of sliding windows of size `win_size` in a slice of length `total_len`.
pub fn windowCount(total_len: usize, win_size: usize): usize {
    if (win_size == 0 || win_size > total_len) {
        return 0;
    }
    return total_len - win_size + 1;
}

export {
    MinMaxResult,
    PartitionResult,
    CartesianPair,
    minmax,
    isSorted,
    isSortedDesc,
    allEqual,
    uniqueI64,
    dedupConsecutive,
    interleave,
    intersperse,
    mergeSorted,
    joinStrings,
    partitionByThreshold,
    cartesianProductCount,
    nthCartesianPair,
    chunkCount,
    windowCount
};
