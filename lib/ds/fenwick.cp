// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Fenwick BIT, O(log N) Order Statistics & Dynamic Weighted Sampler) | Created: 2026-11-22
// Pure CP zero-heap-allocation Binary Indexed Tree (Fenwick Tree) with O(N) linear construction,
// O(log N) Binary Lifting order-statistic `find_lower_bound`, and O(log N) dynamic weighted sampling:
//   - Fixed inline capacity `FENWICK_MAX_LEN = 1024` (zero dynamic heap allocation)
//   - `FenwickTreeI64`: signed `i64` point add/set, point value, prefix sum & range sum
//   - `FenwickOrderStat`: non-negative `u64` frequency/weight tree with underflow-safe `sub_weight`,
//     O(log N) Binary Lifting `find_lower_bound` (auto-skipping zero-weight holes) & `sample_weighted_index`

// ============================================================================
// 1. Constants, Helpers & Public Models
// ============================================================================

/// Maximum logical length supported by `FenwickTreeI64` and `FenwickOrderStat` (zero dynamic heap).
pub const FENWICK_MAX_LEN: usize = 1024;

const FENWICK_TREE_CAP: usize = 1025;
const U64_MAX: u64 = 0xFFFFFFFFFFFFFFFF;
const I64_MAX_U64: u64 = 0x7FFFFFFFFFFFFFFF;

fn lowbit(i: usize): usize {
    const neg: usize = (~i) +% 1;
    return i & neg;
}

/// Summary statistics snapshot for Fenwick tree structures.
pub struct FenwickStats {
    len: usize = 0;
    total_sum: i64 = 0;
    updates_count: u64 = 0;
}

// ============================================================================
// 2. FenwickTreeI64 (Signed i64 Dynamic Prefix & Range Sum Tree)
// ============================================================================

/// Zero-heap-allocation 0-based Fenwick Tree over signed `i64` values (`len <= 1024`).
pub struct FenwickTreeI64 {
    len: usize = 0;
    updates_count: u64 = 0;
    tree: [FENWICK_TREE_CAP]i64 = [0; FENWICK_TREE_CAP];

    /// Initializes a zeroed `FenwickTreeI64` of length `len` (`1..=FENWICK_MAX_LEN`).
    static fn init(len: usize): FenwickTreeI64? {
        if (len == 0 || len > FENWICK_MAX_LEN) return null;
        return FenwickTreeI64{
            len: len,
            updates_count: 0,
            tree: [0; FENWICK_TREE_CAP],
        };
    }

    /// Builds a `FenwickTreeI64` from `values` in strict O(N) linear time.
    static fn build_from_slice(values: []const i64): FenwickTreeI64? {
        if (values.len == 0 || values.len > FENWICK_MAX_LEN) return null;
        let ft = FenwickTreeI64{
            len: values.len,
            updates_count: 0,
            tree: [0; FENWICK_TREE_CAP],
        };
        let i: usize = 1;
        while (i <= values.len) : (i += 1) {
            ft.tree[i] += values[i - 1];
            const parent: usize = i + lowbit(i);
            if (parent <= values.len) {
                ft.tree[parent] += ft.tree[i];
            }
        }
        return ft;
    }

    /// Adds signed `delta` to 0-based index `idx` in O(log N) time. Returns `false` if `idx >= len`.
    fn add(*_, idx: usize, delta: i64): bool {
        if (idx >= _.len) return false;
        let i: usize = idx + 1;
        while (i <= _.len) {
            _.tree[i] += delta;
            i += lowbit(i);
        }
        _.updates_count += 1;
        return true;
    }

    /// Returns the prefix sum over `[0..=idx_inclusive]` in O(log N) time.
    /// Clamps `idx_inclusive` to `len - 1` if out of range.
    fn prefix_sum(idx_inclusive: usize): i64 {
        if (_.len == 0) return 0;
        const clamped: usize = if (idx_inclusive >= _.len) _.len - 1 else idx_inclusive;
        let i: usize = clamped + 1;
        let sum: i64 = 0;
        while (i > 0) {
            sum += _.tree[i];
            i -= lowbit(i);
        }
        return sum;
    }

    /// Returns the single-element value at 0-based index `idx` (0 if out of range).
    fn point_value(idx: usize): i64 {
        if (idx >= _.len) return 0;
        if (idx == 0) return _.prefix_sum(0);
        return _.prefix_sum(idx) - _.prefix_sum(idx - 1);
    }

    /// Sets 0-based index `idx` to `new_val` in O(log N) time. Returns `false` if `idx >= len`.
    fn set(*_, idx: usize, new_val: i64): bool {
        if (idx >= _.len) return false;
        const cur: i64 = _.point_value(idx);
        return _.add(idx, new_val - cur);
    }

    /// Returns the closed range sum over `[lo..=hi_inclusive]`. Returns `0` if `lo > hi_inclusive` or `lo >= len`.
    fn range_sum(lo: usize, hi_inclusive: usize): i64 {
        if (_.len == 0 || lo > hi_inclusive || lo >= _.len) return 0;
        const hi: usize = if (hi_inclusive >= _.len) _.len - 1 else hi_inclusive;
        if (lo == 0) return _.prefix_sum(hi);
        return _.prefix_sum(hi) - _.prefix_sum(lo - 1);
    }

    /// Returns the total sum over all elements `[0..len - 1]`.
    fn total_sum(): i64 {
        if (_.len == 0) return 0;
        return _.prefix_sum(_.len - 1);
    }

    /// Returns a snapshot of length, total sum, and mutation count.
    fn stats(): FenwickStats {
        return FenwickStats{
            len: _.len,
            total_sum: _.total_sum(),
            updates_count: _.updates_count,
        };
    }
}

// ============================================================================
// 3. FenwickOrderStat (Non-Negative u64 Order-Statistic & Weighted Sampler)
// ============================================================================

/// Zero-heap-allocation non-negative `u64` Fenwick Tree supporting O(log N) Binary Lifting
/// order statistics (`find_lower_bound`) and dynamic weighted sampling (`sample_weighted_index`).
pub struct FenwickOrderStat {
    len: usize = 0;
    total_w: u64 = 0;
    updates_count: u64 = 0;
    weights: [FENWICK_MAX_LEN]u64 = [0; FENWICK_MAX_LEN];
    tree: [FENWICK_TREE_CAP]u64 = [0; FENWICK_TREE_CAP];

    /// Initializes a zeroed `FenwickOrderStat` of length `len` (`1..=FENWICK_MAX_LEN`).
    static fn init(len: usize): FenwickOrderStat? {
        if (len == 0 || len > FENWICK_MAX_LEN) return null;
        return FenwickOrderStat{
            len: len,
            total_w: 0,
            updates_count: 0,
            weights: [0; FENWICK_MAX_LEN],
            tree: [0; FENWICK_TREE_CAP],
        };
    }

    /// Adds non-negative `delta` weight to 0-based index `idx` (overflow-safe).
    fn add_weight(*_, idx: usize, delta: u64): bool {
        if (idx >= _.len) return false;
        if (U64_MAX - _.total_w < delta) return false;
        _.weights[idx] += delta;
        _.total_w += delta;
        let i: usize = idx + 1;
        while (i <= _.len) {
            _.tree[i] += delta;
            i += lowbit(i);
        }
        _.updates_count += 1;
        return true;
    }

    /// Subtracts `delta` weight from 0-based index `idx`.
    /// Rejects underflow (`delta > weights[idx]`), returning `false` without mutating state.
    fn sub_weight(*_, idx: usize, delta: u64): bool {
        if (idx >= _.len) return false;
        if (delta > _.weights[idx]) return false;
        _.weights[idx] -= delta;
        _.total_w -= delta;
        let i: usize = idx + 1;
        while (i <= _.len) {
            _.tree[i] -= delta;
            i += lowbit(i);
        }
        _.updates_count += 1;
        return true;
    }

    /// Returns cumulative weight over `[0..=idx_inclusive]` in O(log N) time.
    fn prefix_weight(idx_inclusive: usize): u64 {
        if (_.len == 0) return 0;
        const clamped: usize = if (idx_inclusive >= _.len) _.len - 1 else idx_inclusive;
        let i: usize = clamped + 1;
        let sum: u64 = 0;
        while (i > 0) {
            sum += _.tree[i];
            i -= lowbit(i);
        }
        return sum;
    }

    /// Returns total weight across all indices in O(1) time.
    fn total_weight(): u64 {
        return _.total_w;
    }

    /// Strict O(log N) Binary Lifting lower-bound search:
    /// finds the smallest 0-based index `idx` such that `prefix_weight(idx) >= target_cumulative`.
    /// Automatically skips zero-weight holes. Returns `null` if `target_cumulative == 0` or `> total_weight()`.
    fn find_lower_bound(target_cumulative: u64): usize? {
        if (_.len == 0 || target_cumulative == 0 || target_cumulative > _.total_w) {
            return null;
        }
        let step: usize = 1;
        while ((step << 1) <= _.len) {
            step = step << 1;
        }
        let pos: usize = 0;
        let acc: u64 = 0;
        while (step > 0) {
            const next_pos: usize = pos + step;
            if (next_pos <= _.len && acc + _.tree[next_pos] < target_cumulative) {
                acc += _.tree[next_pos];
                pos = next_pos;
            }
            step = step >> 1;
        }
        if (pos >= _.len) return null;
        return pos;
    }

    /// Maps an arbitrary `rng_val: u64` uniformly into `[1, total_weight()]` and returns the
    /// sampled 0-based index in O(log N) time proportional to each slot's weight.
    /// Returns `null` when `total_weight() == 0`.
    fn sample_weighted_index(rng_val: u64): usize? {
        if (_.total_w == 0) return null;
        const target: u64 = (rng_val % _.total_w) + 1;
        return _.find_lower_bound(target);
    }

    /// Returns a statistics snapshot (`total_sum` clamped to `i64` max if `total_weight > i64::MAX`).
    fn stats(): FenwickStats {
        const clamped_sum: i64 = if (_.total_w > I64_MAX_U64)
            @intCast(I64_MAX_U64)
        else
            @intCast(_.total_w);
        return FenwickStats{
            len: _.len,
            total_sum: clamped_sum,
            updates_count: _.updates_count,
        };
    }
}
