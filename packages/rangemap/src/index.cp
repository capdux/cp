// @cap/range-set — Zero-allocation sorted disjoint half-open interval set [start, end) for CP
// Clean-room MIT implementation inspired by Rust `rangemap` / `range-set` and npm `merge-ranges`.
//
// Invariant:
//   For all 0 <= i < count: ranges[i].start < ranges[i].end
//   For all 0 <= i < count - 1: ranges[i].end < ranges[i + 1].start (strictly disjoint & coalesced)
//
// Features:
// - Fixed stack capacity `MAX_RANGES = 128` (zero heap allocation).
// - Automatic merging of overlapping and adjacent (`a.end == b.start`) intervals on `insert`.
// - Full interval subtraction on `remove` (truncation, full deletion, and mid-interval splitting).
// - O(log N) binary-search `contains`, `containsRange`, `overlaps`, and `totalSpan`.

pub const MAX_RANGES: usize = 128;

/// Half-open integer interval `[start, end)` (`start < end` when non-empty).
pub struct Range {
    pub start: i64,
    pub end: i64,
}

/// Zero-allocation sorted set of disjoint, coalesced half-open intervals `[start, end)`.
pub struct RangeSet {
    pub ranges: [MAX_RANGES]Range,
    pub count: usize,

    pub static fn init() -> RangeSet {
        let arr: [MAX_RANGES]Range = undefined;
        return RangeSet{ ranges: arr, count: 0 };
    }

    pub fn len() -> usize {
        return _.count;
    }

    pub fn isEmpty() -> bool {
        return _.count == 0;
    }

    pub fn clear(*_) {
        _.count = 0;
    }

    /// Returns the `idx`-th sorted interval, or `Range{ start: 0, end: 0 }` if out of bounds.
    pub fn get(idx: usize) -> Range {
        if (idx >= _.count) {
            return Range{ start: 0, end: 0 };
        }
        return _.ranges[idx];
    }

    /// Inserts half-open interval `[start, end)`, coalescing all overlapping and adjacent ranges.
    /// Ignores empty/invalid intervals (`start >= end`). Returns `false` if capacity (128) is full.
    pub fn insert(*_, start: i64, end: i64) -> bool {
        if (start >= end) {
            return true;
        }

        // First range with ranges[first].end >= start (touches or overlaps)
        let first: usize = 0;
        while (first < _.count && _.ranges[first].end < start) {
            first += 1;
        }

        // First range strictly after end (ranges[last].start > end)
        let last = first;
        while (last < _.count && _.ranges[last].start <= end) {
            last += 1;
        }

        if (first == last) {
            // Pure insertion of a brand new disjoint range
            if (_.count >= MAX_RANGES) {
                return false;
            }
            let j = _.count;
            while (j > first) {
                _.ranges[j] = _.ranges[j - 1];
                j -= 1;
            }
            _.ranges[first] = Range{ start: start, end: end };
            _.count += 1;
            return true;
        }

        // Merge [first..last) into a single coalesced range at `first`
        let merged_start = start;
        if (_.ranges[first].start < merged_start) {
            merged_start = _.ranges[first].start;
        }
        let merged_end = end;
        if (_.ranges[last - 1].end > merged_end) {
            merged_end = _.ranges[last - 1].end;
        }

        _.ranges[first] = Range{ start: merged_start, end: merged_end };

        const removed = last - first - 1;
        if (removed > 0) {
            let k = last;
            while (k < _.count) {
                _.ranges[k - removed] = _.ranges[k];
                k += 1;
            }
            _.count -= removed;
        }

        return true;
    }

    /// Subtracts `[start, end)` from the set, handling truncation, deletion, and mid-interval splitting.
    /// Returns `false` only if splitting a single interval would exceed `MAX_RANGES` (128).
    pub fn remove(*_, start: i64, end: i64) -> bool {
        if (start >= end || _.count == 0) {
            return true;
        }

        // First range strictly intersecting: ranges[first].end > start
        let first: usize = 0;
        while (first < _.count && _.ranges[first].end <= start) {
            first += 1;
        }

        // First range at or after end: ranges[last].start >= end
        let last = first;
        while (last < _.count && _.ranges[last].start < end) {
            last += 1;
        }

        if (first == last) {
            return true;
        }

        // Special case: single range strictly enclosing [start, end) -> splits into 2 ranges
        if (first + 1 == last && _.ranges[first].start < start && _.ranges[first].end > end) {
            if (_.count >= MAX_RANGES) {
                return false;
            }
            const orig_end = _.ranges[first].end;
            let j = _.count;
            while (j > first + 1) {
                _.ranges[j] = _.ranges[j - 1];
                j -= 1;
            }
            _.ranges[first].end = start;
            _.ranges[first + 1] = Range{ start: end, end: orig_end };
            _.count += 1;
            return true;
        }

        let keep_first = false;
        if (_.ranges[first].start < start) {
            _.ranges[first].end = start;
            keep_first = true;
        }

        let keep_last = false;
        let right_piece = Range{ start: 0, end: 0 };
        if (_.ranges[last - 1].end > end) {
            right_piece = Range{ start: end, end: _.ranges[last - 1].end };
            keep_last = true;
        }

        let write_idx = first;
        if (keep_first) {
            write_idx += 1;
        }
        if (keep_last) {
            _.ranges[write_idx] = right_piece;
            write_idx += 1;
        }

        let read_idx = last;
        while (read_idx < _.count) {
            _.ranges[write_idx] = _.ranges[read_idx];
            write_idx += 1;
            read_idx += 1;
        }
        _.count = write_idx;
        return true;
    }

    /// Binary search for the first range index with `ranges[idx].end > value`.
    fn lowerBoundEnd(value: i64) -> usize {
        let lo: usize = 0;
        let hi: usize = _.count;
        while (lo < hi) {
            const mid = lo + ((hi - lo) >> 1);
            if (_.ranges[mid].end <= value) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    }

    /// Returns `true` if `point` lies inside any interval (`start <= point < end`).
    pub fn contains(point: i64) -> bool {
        const idx = _.lowerBoundEnd(point);
        if (idx >= _.count) {
            return false;
        }
        return _.ranges[idx].start <= point && point < _.ranges[idx].end;
    }

    /// Returns `true` if the entire interval `[start, end)` is covered by the set.
    pub fn containsRange(start: i64, end: i64) -> bool {
        if (start >= end) {
            return true;
        }
        const idx = _.lowerBoundEnd(start);
        if (idx >= _.count) {
            return false;
        }
        return _.ranges[idx].start <= start && _.ranges[idx].end >= end;
    }

    /// Returns `true` if `[start, end)` has a non-empty intersection with any interval in the set.
    pub fn overlaps(start: i64, end: i64) -> bool {
        if (start >= end || _.count == 0) {
            return false;
        }
        const idx = _.lowerBoundEnd(start);
        if (idx >= _.count) {
            return false;
        }
        return _.ranges[idx].start < end;
    }

    /// Returns the total covered length across all disjoint intervals.
    pub fn totalSpan() -> i64 {
        let sum: i64 = 0;
        let i: usize = 0;
        while (i < _.count) {
            sum += _.ranges[i].end - _.ranges[i].start;
            i += 1;
        }
        return sum;
    }
}
