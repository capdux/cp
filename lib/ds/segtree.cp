// Source: cp/lib/segtree/segtree.cp (Fenwick Tree & Lazy-Propagation Segment Tree Range Query Engine)
// Pure CP zero-C-dependency range query engine with O(N) Fenwick tree construction,
// O(log N) binary-lifting prefix search, and lazy-propagation segment tree supporting
// both rangeAdd and rangeAssign over (sum, min, max) statistics.

const std = @import("std");

/// Error set for FenwickTree and LazySegmentTree operations.
pub err SegTreeError {
    IndexOutOfBounds,
    InvalidRange,
    EmptyTree,
    OutOfMemory,
}

/// Aggregated range statistics returned by LazySegmentTree queries.
pub struct RangeStats {
    sum: i64,
    min: i64,
    max: i64,
}

/// Binary Indexed Tree (Fenwick Tree) supporting O(N) build, O(log N) point updates,
/// prefix/range sum queries, and O(log N) lower-bound prefix search on non-negative arrays.
pub struct FenwickTree {
    alloc: std.mem.Allocator,
    tree: []i64 = [],
    raw: []i64 = [],
    size: usize = 0,

    /// Initialize an empty FenwickTree of length `n` with all elements set to 0.
    pub static fn init(alloc: std.mem.Allocator, n: usize): FenwickTree ! SegTreeError {
        if (n == 0) {
            return FenwickTree{
                alloc: alloc,
                tree: [],
                raw: [],
                size: 0,
            };
        }
        let tree = alloc.alloc(i64, n + 1) catch return SegTreeError.OutOfMemory;
        errdefer alloc.free(tree);
        const raw = alloc.alloc(i64, n) catch return SegTreeError.OutOfMemory;
        @memset(tree, 0);
        @memset(raw, 0);
        return FenwickTree{
            alloc: alloc,
            tree: tree,
            raw: raw,
            size: n,
        };
    }

    /// Construct a FenwickTree from `values` in O(N) time.
    pub fn fromSlice(alloc: std.mem.Allocator, values: []const i64): FenwickTree ! SegTreeError {
        const n = values.len;
        if (n == 0) {
            return FenwickTree{
                alloc: alloc,
                tree: [],
                raw: [],
                size: 0,
            };
        }
        let tree = alloc.alloc(i64, n + 1) catch return SegTreeError.OutOfMemory;
        errdefer alloc.free(tree);
        const raw = alloc.alloc(i64, n) catch return SegTreeError.OutOfMemory;

        @memset(tree, 0);
        @memcpy(raw, values);

        for (i of 0..n) {
            const idx1 = i + 1;
            tree[idx1] += values[i];
            const lsb = idx1 & (~idx1 +% 1);
            const parent = idx1 + lsb;
            if (parent <= n) {
                tree[parent] += tree[idx1];
            }
        }

        return FenwickTree{
            alloc: alloc,
            tree: tree,
            raw: raw,
            size: n,
        };
    }

    /// Free backing buffers.
    pub fn deinit(*_) {
        if (_.size > 0) {
            _.alloc.free(_.tree);
            _.alloc.free(_.raw);
            _.tree = [];
            _.raw = [];
            _.size = 0;
        }
    }

    /// Return the number of elements in the FenwickTree.
    pub fn len(): usize {
        return _.size;
    }

    /// Add `delta` to element at 0-based `idx` in O(log N) time.
    pub fn add(*_, idx: usize, delta: i64): void ! SegTreeError {
        if (_.size == 0) return SegTreeError.EmptyTree;
        if (idx >= _.size) return SegTreeError.IndexOutOfBounds;

        _.raw[idx] += delta;
        let i = idx + 1;
        while (i <= _.size) {
            _.tree[i] += delta;
            i += i & (~i +% 1);
        }
    }

    /// Set element at 0-based `idx` to `val` in O(log N) time.
    pub fn set(*_, idx: usize, val: i64): void ! SegTreeError {
        if (_.size == 0) return SegTreeError.EmptyTree;
        if (idx >= _.size) return SegTreeError.IndexOutOfBounds;

        const delta = val - _.raw[idx];
        if (delta != 0) {
            _.add(idx, delta)?;
        }
    }

    /// Return the current value at 0-based `idx` in O(1) time.
    pub fn get(idx: usize): i64 ! SegTreeError {
        if (_.size == 0) return SegTreeError.EmptyTree;
        if (idx >= _.size) return SegTreeError.IndexOutOfBounds;
        return _.raw[idx];
    }

    /// Compute inclusive prefix sum over [0, idx_inclusive] in O(log N) time.
    pub fn prefixSum(idx_inclusive: usize): i64 ! SegTreeError {
        if (_.size == 0) return SegTreeError.EmptyTree;
        if (idx_inclusive >= _.size) return SegTreeError.IndexOutOfBounds;

        let sum: i64 = 0;
        let i = idx_inclusive + 1;
        while (i > 0) {
            sum += _.tree[i];
            i -= i & (~i +% 1);
        }
        return sum;
    }

    /// Compute half-open range sum over [left, right_excl) in O(log N) time.
    pub fn rangeSum(left: usize, right_excl: usize): i64 ! SegTreeError {
        if (left > right_excl) return SegTreeError.InvalidRange;
        if (right_excl > _.size) return SegTreeError.IndexOutOfBounds;
        if (left == right_excl) return 0;

        const right_sum = _.prefixSum(right_excl - 1)?;
        if (left == 0) return right_sum;
        const left_sum = _.prefixSum(left - 1)?;
        return right_sum - left_sum;
    }

    /// Binary-lift in O(log N) time over a non-negative FenwickTree to find the smallest
    /// 0-based index `idx` such that `prefixSum(idx) >= target`.
    /// Returns `null` if the tree is empty or total sum is strictly less than `target`.
    pub fn lowerBoundPrefix(target: i64): usize? {
        if (_.size == 0) return null;
        if (target <= 0) return 0;

        let step: usize = 1;
        while (step * 2 <= _.size) {
            step *= 2;
        }

        let pos: usize = 0;
        let acc: i64 = 0;
        while (step > 0) {
            const next_pos = pos + step;
            if (next_pos <= _.size) {
                const cand = acc + _.tree[next_pos];
                if (cand < target) {
                    pos = next_pos;
                    acc = cand;
                }
            }
            step /= 2;
        }

        if (pos >= _.size) return null;
        return pos;
    }
}

struct SegNode {
    sum: i64,
    min: i64,
    max: i64,
    has_assign: bool,
    assign_val: i64,
    add_val: i64,
}

fn combineStats(a: RangeStats, b: RangeStats): RangeStats {
    return RangeStats{
        sum: a.sum + b.sum,
        min: if (a.min < b.min) a.min else b.min,
        max: if (a.max > b.max) a.max else b.max,
    };
}

/// Lazy-propagation segment tree supporting O(log N) rangeAdd, rangeAssign,
/// and simultaneous (sum, min, max) range queries.
pub struct LazySegmentTree {
    alloc: std.mem.Allocator,
    nodes: []SegNode = [],
    size: usize = 0,

    /// Construct a LazySegmentTree from `values` in O(N) time.
    pub static fn init(alloc: std.mem.Allocator, values: []const i64): LazySegmentTree ! SegTreeError {
        const n = values.len;
        if (n == 0) {
            return LazySegmentTree{
                alloc: alloc,
                nodes: [],
                size: 0,
            };
        }
        let nodes = alloc.alloc(SegNode, n * 4) catch return SegTreeError.OutOfMemory;
        for (i of 0..nodes.len) {
            nodes[i] = SegNode{
                sum: 0,
                min: 0,
                max: 0,
                has_assign: false,
                assign_val: 0,
                add_val: 0,
            };
        }

        let tree = LazySegmentTree{
            alloc: alloc,
            nodes: nodes,
            size: n,
        };
        tree.build(1, 0, n - 1, values);
        return tree;
    }

    /// Free backing segment tree nodes.
    pub fn deinit(*_) {
        if (_.size > 0) {
            _.alloc.free(_.nodes);
            _.nodes = [];
            _.size = 0;
        }
    }

    /// Return the number of elements in the LazySegmentTree.
    pub fn len(): usize {
        return _.size;
    }

    fn build(*_, node_idx: usize, l: usize, r: usize, values: []const i64) {
        if (l == r) {
            const v = values[l];
            _.nodes[node_idx] = SegNode{
                sum: v,
                min: v,
                max: v,
                has_assign: false,
                assign_val: 0,
                add_val: 0,
            };
            return;
        }
        const mid = l + (r - l) / 2;
        const left_idx = node_idx * 2;
        const right_idx = left_idx + 1;
        _.build(left_idx, l, mid, values);
        _.build(right_idx, mid + 1, r, values);
        _.pullUp(node_idx);
    }

    fn pullUp(*_, node_idx: usize) {
        const left = _.nodes[node_idx * 2];
        const right = _.nodes[node_idx * 2 + 1];
        _.nodes[node_idx].sum = left.sum + right.sum;
        _.nodes[node_idx].min = if (left.min < right.min) left.min else right.min;
        _.nodes[node_idx].max = if (left.max > right.max) left.max else right.max;
    }

    fn applyAssignNode(*_, node_idx: usize, l: usize, r: usize, val: i64) {
        const cnt: i64 = @as(i64, @intCast(r - l + 1));
        _.nodes[node_idx].sum = val * cnt;
        _.nodes[node_idx].min = val;
        _.nodes[node_idx].max = val;
        _.nodes[node_idx].has_assign = true;
        _.nodes[node_idx].assign_val = val;
        _.nodes[node_idx].add_val = 0;
    }

    fn applyAddNode(*_, node_idx: usize, l: usize, r: usize, delta: i64) {
        const cnt: i64 = @as(i64, @intCast(r - l + 1));
        _.nodes[node_idx].sum += delta * cnt;
        _.nodes[node_idx].min += delta;
        _.nodes[node_idx].max += delta;
        if (_.nodes[node_idx].has_assign) {
            _.nodes[node_idx].assign_val += delta;
        } else {
            _.nodes[node_idx].add_val += delta;
        }
    }

    fn pushDown(*_, node_idx: usize, l: usize, r: usize) {
        if (l == r) {
            _.nodes[node_idx].has_assign = false;
            _.nodes[node_idx].assign_val = 0;
            _.nodes[node_idx].add_val = 0;
            return;
        }
        const mid = l + (r - l) / 2;
        const left_idx = node_idx * 2;
        const right_idx = left_idx + 1;

        if (_.nodes[node_idx].has_assign) {
            const v = _.nodes[node_idx].assign_val;
            _.applyAssignNode(left_idx, l, mid, v);
            _.applyAssignNode(right_idx, mid + 1, r, v);
            _.nodes[node_idx].has_assign = false;
            _.nodes[node_idx].assign_val = 0;
            _.nodes[node_idx].add_val = 0;
        } else if (_.nodes[node_idx].add_val != 0) {
            const d = _.nodes[node_idx].add_val;
            _.applyAddNode(left_idx, l, mid, d);
            _.applyAddNode(right_idx, mid + 1, r, d);
            _.nodes[node_idx].add_val = 0;
        }
    }

    fn updateAdd(*_, node_idx: usize, l: usize, r: usize, ql: usize, qr: usize, delta: i64) {
        if (ql <= l && r <= qr) {
            _.applyAddNode(node_idx, l, r, delta);
            return;
        }
        _.pushDown(node_idx, l, r);
        const mid = l + (r - l) / 2;
        if (ql <= mid) {
            _.updateAdd(node_idx * 2, l, mid, ql, qr, delta);
        }
        if (qr > mid) {
            _.updateAdd(node_idx * 2 + 1, mid + 1, r, ql, qr, delta);
        }
        _.pullUp(node_idx);
    }

    fn updateAssign(*_, node_idx: usize, l: usize, r: usize, ql: usize, qr: usize, val: i64) {
        if (ql <= l && r <= qr) {
            _.applyAssignNode(node_idx, l, r, val);
            return;
        }
        _.pushDown(node_idx, l, r);
        const mid = l + (r - l) / 2;
        if (ql <= mid) {
            _.updateAssign(node_idx * 2, l, mid, ql, qr, val);
        }
        if (qr > mid) {
            _.updateAssign(node_idx * 2 + 1, mid + 1, r, ql, qr, val);
        }
        _.pullUp(node_idx);
    }

    fn queryRange(*_, node_idx: usize, l: usize, r: usize, ql: usize, qr: usize): RangeStats {
        if (ql <= l && r <= qr) {
            const n = _.nodes[node_idx];
            return RangeStats{
                sum: n.sum,
                min: n.min,
                max: n.max,
            };
        }
        _.pushDown(node_idx, l, r);
        const mid = l + (r - l) / 2;
        if (qr <= mid) {
            return _.queryRange(node_idx * 2, l, mid, ql, qr);
        }
        if (ql > mid) {
            return _.queryRange(node_idx * 2 + 1, mid + 1, r, ql, qr);
        }
        const left_res = _.queryRange(node_idx * 2, l, mid, ql, qr);
        const right_res = _.queryRange(node_idx * 2 + 1, mid + 1, r, ql, qr);
        return combineStats(left_res, right_res);
    }

    /// Add `delta` to all elements in half-open range [left, right_excl) in O(log N) time.
    pub fn rangeAdd(*_, left: usize, right_excl: usize, delta: i64): void ! SegTreeError {
        if (_.size == 0) return SegTreeError.EmptyTree;
        if (left > right_excl) return SegTreeError.InvalidRange;
        if (right_excl > _.size) return SegTreeError.IndexOutOfBounds;
        if (left == right_excl) return;

        _.updateAdd(1, 0, _.size - 1, left, right_excl - 1, delta);
    }

    /// Assign `val` to all elements in half-open range [left, right_excl) in O(log N) time.
    pub fn rangeAssign(*_, left: usize, right_excl: usize, val: i64): void ! SegTreeError {
        if (_.size == 0) return SegTreeError.EmptyTree;
        if (left > right_excl) return SegTreeError.InvalidRange;
        if (right_excl > _.size) return SegTreeError.IndexOutOfBounds;
        if (left == right_excl) return;

        _.updateAssign(1, 0, _.size - 1, left, right_excl - 1, val);
    }

    /// Query (sum, min, max) over half-open range [left, right_excl) in O(log N) time.
    pub fn query(*_, left: usize, right_excl: usize): RangeStats ! SegTreeError {
        if (_.size == 0) return SegTreeError.EmptyTree;
        if (left >= right_excl) return SegTreeError.InvalidRange;
        if (right_excl > _.size) return SegTreeError.IndexOutOfBounds;

        return _.queryRange(1, 0, _.size - 1, left, right_excl - 1);
    }

    /// Query sum over half-open range [left, right_excl) in O(log N) time.
    pub fn querySum(*_, left: usize, right_excl: usize): i64 ! SegTreeError {
        const stats = _.query(left, right_excl)?;
        return stats.sum;
    }

    /// Query minimum over half-open range [left, right_excl) in O(log N) time.
    pub fn queryMin(*_, left: usize, right_excl: usize): i64 ! SegTreeError {
        const stats = _.query(left, right_excl)?;
        return stats.min;
    }

    /// Query maximum over half-open range [left, right_excl) in O(log N) time.
    pub fn queryMax(*_, left: usize, right_excl: usize): i64 ! SegTreeError {
        const stats = _.query(left, right_excl)?;
        return stats.max;
    }

    /// Query single element at 0-based `idx` in O(log N) time.
    pub fn get(*_, idx: usize): i64 ! SegTreeError {
        if (_.size == 0) return SegTreeError.EmptyTree;
        if (idx >= _.size) return SegTreeError.IndexOutOfBounds;

        const stats = _.queryRange(1, 0, _.size - 1, idx, idx);
        return stats.sum;
    }
}
