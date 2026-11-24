// Reference-hardened against rust-lang/rust BTree & rust-bio IntervalTree (MIT OR Apache-2.0)
// Source: cp/lib/interval_tree/interval_tree.cp (Pure CP Augmented Interval Tree with 4-Lane SIMD Overlap Batching & Invariant Verification)
const std = @import("std");

pub const IntervalError = error{
    InvalidInterval,
};

type ItBptVec4i64 = @Vector(4, i64);

/// Scalar reference: counts intervals `(lows[i], highs[i])` overlapping closed query `[q_low, q_high]`.
/// Returns 0 if `q_low > q_high` or `lows.len != highs.len`; skips malformed `lows[i] > highs[i]` entries.
pub fn scalarIntervalCountOverlaps(
    lows: []const i64,
    highs: []const i64,
    q_low: i64,
    q_high: i64,
): usize {
    if (q_low > q_high or lows.len != highs.len) {
        return 0;
    }
    let cnt: usize = 0;
    let i: usize = 0;
    while (i < lows.len) : (i += 1) {
        const lo = lows[i];
        const hi = highs[i];
        if (lo <= hi and lo <= q_high and q_low <= hi) {
            cnt += 1;
        }
    }
    return cnt;
}

/// 4-lane `@Vector(4, i64)` SIMD batch overlap counter matching `scalarIntervalCountOverlaps` with 0 discrepancy.
pub fn simdIntervalCountOverlaps(
    lows: []const i64,
    highs: []const i64,
    q_low: i64,
    q_high: i64,
): usize {
    if (q_low > q_high or lows.len != highs.len) {
        return 0;
    }
    const n: usize = lows.len;
    const v_qlo: ItBptVec4i64 = @splat(q_low);
    const v_qhi: ItBptVec4i64 = @splat(q_high);
    let cnt: usize = 0;
    let i: usize = 0;
    while (i + 4 <= n) : (i += 4) {
        const vlo: ItBptVec4i64 = lows[i .. i + 4][0..4].*;
        const vhi: ItBptVec4i64 = highs[i .. i + 4][0..4].*;
        const c0: usize = @intFromBool(vlo[0] <= vhi[0] and vlo[0] <= v_qhi[0] and v_qlo[0] <= vhi[0]);
        const c1: usize = @intFromBool(vlo[1] <= vhi[1] and vlo[1] <= v_qhi[1] and v_qlo[1] <= vhi[1]);
        const c2: usize = @intFromBool(vlo[2] <= vhi[2] and vlo[2] <= v_qhi[2] and v_qlo[2] <= vhi[2]);
        const c3: usize = @intFromBool(vlo[3] <= vhi[3] and vlo[3] <= v_qhi[3] and v_qlo[3] <= vhi[3]);
        cnt += c0 + c1 + c2 + c3;
    }
    while (i < n) : (i += 1) {
        const lo = lows[i];
        const hi = highs[i];
        if (lo <= hi and lo <= q_high and q_low <= hi) {
            cnt += 1;
        }
    }
    return cnt;
}

pub fn Interval(comptime T: type, comptime V: type): type {
    return struct {
        low: T,
        high: T,
        value: V,
    };
}

pub fn IntervalTree(comptime T: type, comptime V: type): type {
    const Node = struct {
        low: T,
        high: T,
        max_high: T,
        value: V,
        left: (*@This())?,
        right: (*@This())?,
    };

    const InvariantCheck = struct {
        ok: bool,
        node_count: usize,
        computed_max: T,
    };

    const SimdBatch = struct {
        lows: [4]i64 = [0; 4],
        highs: [4]i64 = [0; 4],
        len: usize = 0,
        total: usize = 0,
    };

    return struct {
        root: (*Node)?,
        count: usize,
        alloc: std.mem.Allocator,

        const Self = @This();
        pub const Entry = Interval(T, V);

        pub fn init(alloc: std.mem.Allocator): Self {
            return Self{
                .root = null,
                .count = 0,
                .alloc = alloc,
            };
        }

        static fn freeNode(alloc: std.mem.Allocator, maybe_node: (*Node)?) {
            if (n := maybe_node) {
                freeNode(alloc, n.left);
                freeNode(alloc, n.right);
                alloc.destroy(n);
            }
        }

        pub fn deinit(*_) {
            freeNode(_.alloc, _.root);
            _.root = null;
            _.count = 0;
        }

        pub fn len(): usize {
            return _.count;
        }

        pub fn isEmpty(): bool {
            return _.count == 0;
        }

        static fn insertNode(alloc: std.mem.Allocator, maybe_node: (*Node)?, low: T, high: T, value: V): !*Node {
            if (n := maybe_node) {
                if (low < n.low) {
                    n.left = try insertNode(alloc, n.left, low, high, value);
                } else {
                    n.right = try insertNode(alloc, n.right, low, high, value);
                }
                if (high > n.max_high) {
                    n.max_high = high;
                }
                return n;
            }
            let node = try alloc.create(Node);
            node.* = Node{
                .low = low,
                .high = high,
                .max_high = high,
                .value = value,
                .left = null,
                .right = null,
            };
            return node;
        }

        pub fn insert(*_, low: T, high: T, value: V): !void {
            if (low > high) { return IntervalError.InvalidInterval; }
            _.root = try insertNode(_.alloc, _.root, low, high, value);
            _.count += 1;
        }

        static fn overlaps(a_low: T, a_high: T, b_low: T, b_high: T): bool {
            return a_low <= b_high and b_low <= a_high;
        }

        static fn searchOverlap(maybe_node: (*Node)?, low: T, high: T, alloc: std.mem.Allocator, out: *std.ArrayList(Entry)): !void {
            if (n := maybe_node) {
                if (left := n.left) {
                    if (left.max_high >= low) {
                        try searchOverlap(left, low, high, alloc, out);
                    }
                }
                if (overlaps(n.low, n.high, low, high)) {
                    try out.append(alloc, Entry{
                        .low = n.low,
                        .high = n.high,
                        .value = n.value,
                    });
                }
                if (n.low <= high) {
                    try searchOverlap(n.right, low, high, alloc, out);
                }
            }
        }

        pub fn queryOverlap(alloc: std.mem.Allocator, low: T, high: T): ![]Entry {
            if (low > high) { return IntervalError.InvalidInterval; }
            let out = std.ArrayList(Entry).empty;
            errdefer out.deinit(alloc);
            try searchOverlap(_.root, low, high, alloc, &out);
            return out.toOwnedSlice(alloc);
        }

        pub fn queryPoint(alloc: std.mem.Allocator, point: T): ![]Entry {
            return _.queryOverlap(alloc, point, point);
        }

        static fn countOverlapSimdI64(maybe_node: (*Node)?, low: i64, high: i64, batch: *SimdBatch) {
            if (n := maybe_node) {
                if (left := n.left) {
                    if (left.max_high >= low) {
                        countOverlapSimdI64(left, low, high, batch);
                    }
                }
                batch.lows[batch.len] = n.low;
                batch.highs[batch.len] = n.high;
                batch.len += 1;
                if (batch.len == 4) {
                    batch.total += simdIntervalCountOverlaps(batch.lows[0..4], batch.highs[0..4], low, high);
                    batch.len = 0;
                }
                if (n.low <= high) {
                    countOverlapSimdI64(n.right, low, high, batch);
                }
            }
        }

        static fn countOverlapGeneric(maybe_node: (*Node)?, low: T, high: T): usize {
            if (n := maybe_node) {
                let acc: usize = 0;
                if (left := n.left) {
                    if (left.max_high >= low) {
                        acc += countOverlapGeneric(left, low, high);
                    }
                }
                if (overlaps(n.low, n.high, low, high)) {
                    acc += 1;
                }
                if (n.low <= high) {
                    acc += countOverlapGeneric(n.right, low, high);
                }
                return acc;
            }
            return 0;
        }

        /// Counts intervals overlapping `[low, high]` (routes `i64` queries through 4-lane SIMD batching).
        pub fn countOverlaps(low: T, high: T): !usize {
            if (low > high) { return IntervalError.InvalidInterval; }
            if (comptime T == i64) {
                let batch = SimdBatch{};
                countOverlapSimdI64(_.root, low, high, *batch);
                if (batch.len > 0) {
                    batch.total += simdIntervalCountOverlaps(batch.lows[0..batch.len], batch.highs[0..batch.len], low, high);
                }
                return batch.total;
            }
            return countOverlapGeneric(_.root, low, high);
        }

        static fn hasOverlapNode(maybe_node: (*Node)?, low: T, high: T): bool {
            let cur = maybe_node;
            while (n := cur) {
                if (overlaps(n.low, n.high, low, high)) { return true; }
                if (left := n.left) {
                    if (left.max_high >= low) {
                        cur = left;
                        continue;
                    }
                }
                cur = n.right;
            }
            return false;
        }

        pub fn hasOverlap(low: T, high: T): !bool {
            if (low > high) { return IntervalError.InvalidInterval; }
            return hasOverlapNode(_.root, low, high);
        }

        static fn checkInvariantsNode(
            n: *Node,
            has_lo: bool,
            lo_bound: T,
            has_hi: bool,
            hi_bound: T,
        ): InvariantCheck {
            if (n.low > n.high) {
                return InvariantCheck{ .ok = false, .node_count = 0, .computed_max = n.high };
            }
            if (has_lo and n.low < lo_bound) {
                return InvariantCheck{ .ok = false, .node_count = 0, .computed_max = n.high };
            }
            if (has_hi and !(n.low < hi_bound)) {
                return InvariantCheck{ .ok = false, .node_count = 0, .computed_max = n.high };
            }

            let total: usize = 1;
            let mx: T = n.high;

            if (left := n.left) {
                const lchk = checkInvariantsNode(left, has_lo, lo_bound, true, n.low);
                if (!lchk.ok) {
                    return InvariantCheck{ .ok = false, .node_count = 0, .computed_max = n.high };
                }
                total += lchk.node_count;
                if (lchk.computed_max > mx) mx = lchk.computed_max;
            }
            if (right := n.right) {
                const rchk = checkInvariantsNode(right, true, n.low, has_hi, hi_bound);
                if (!rchk.ok) {
                    return InvariantCheck{ .ok = false, .node_count = 0, .computed_max = n.high };
                }
                total += rchk.node_count;
                if (rchk.computed_max > mx) mx = rchk.computed_max;
            }
            if (n.max_high != mx) {
                return InvariantCheck{ .ok = false, .node_count = 0, .computed_max = mx };
            }
            return InvariantCheck{ .ok = true, .node_count = total, .computed_max = mx };
        }

        /// Verifies BST low-key ordering, `low <= high`, exact augmented `max_high`, and node count.
        pub fn verifyInvariants(): bool {
            if (_.root == null) {
                return _.count == 0;
            }
            const r = _.root.?;
            const chk = checkInvariantsNode(r, false, r.low, false, r.low);
            return chk.ok and chk.node_count == _.count;
        }
    };
}
