// Source: cp/lib/interval/interval.cp (Disjoint Interval Set Algebra & Augmented Interval Tree)
// Pure CP zero-C-dependency half-open interval [start, end) algebra, canonical disjoint
// IntervalSet with O(M+N) set operations, and AVL-balanced generic IntervalTree<V>.

const std = @import("std");

const NULL_IDX: usize = std.math.maxInt(usize);

/// Error set for `cap:interval` operations.
pub err IntervalError {
    InvalidInterval,
    EmptyInterval,
    OutOfMemory,
}

/// Half-open integer interval `[start, end)` where `start <= end`.
pub struct Interval {
    pub start: i64 = 0,
    pub end: i64 = 0,

    /// Constructs a validated half-open interval `[start, end)`.
    /// Returns `IntervalError.InvalidInterval` if `start > end`.
    pub static fn init(start: i64, end: i64): Interval ! IntervalError {
        if (start > end) {
            return IntervalError.InvalidInterval;
        }
        return Interval{ start: start, end: end };
    }

    /// Returns the length `end - start` as `u64` without signed overflow.
    pub fn len(): u64 {
        if (_.end <= _.start) {
            return 0;
        }
        const diff: i128 = @as(i128, _.end) - @as(i128, _.start);
        return @intCast(diff);
    }

    /// Returns `true` if the interval is empty (`start >= end`).
    pub fn isEmpty(): bool {
        return _.start >= _.end;
    }

    /// Returns `true` if point `p` lies within `[start, end)`.
    pub fn containsPoint(p: i64): bool {
        return p >= _.start && p < _.end;
    }

    /// Returns `true` if two non-empty intervals share at least one point.
    pub fn overlaps(other: Interval): bool {
        if (_.isEmpty() || other.isEmpty()) {
            return false;
        }
        return _.start < other.end && other.start < _.end;
    }

    /// Returns `true` if two non-empty intervals are strictly adjacent (`a.end == b.start` or `b.end == a.start`).
    pub fn touches(other: Interval): bool {
        if (_.isEmpty() || other.isEmpty()) {
            return false;
        }
        return _.end == other.start || other.end == _.start;
    }

    /// Returns the intersection `[max(start), min(end))`, or `null` if disjoint.
    pub fn intersect(other: Interval): Interval? {
        const s = if (_.start > other.start) _.start else other.start;
        const e = if (_.end < other.end) _.end else other.end;
        if (s >= e) {
            return null;
        }
        return Interval{ start: s, end: e };
    }

    /// Returns the smallest bounding interval `[min(start), max(end))` enclosing both intervals.
    pub fn hull(other: Interval): Interval {
        const s = if (_.start < other.start) _.start else other.start;
        const e = if (_.end > other.end) _.end else other.end;
        return Interval{ start: s, end: e };
    }
}

/// Canonical normalized set of disjoint, non-touching, strictly sorted half-open intervals.
pub struct IntervalSet {
    allocator: std.mem.Allocator,
    intervals: std.ArrayListUnmanaged(Interval),

    /// Initializes an empty `IntervalSet`.
    pub static fn init(allocator: std.mem.Allocator): IntervalSet {
        return IntervalSet{
            allocator: allocator,
            intervals: .empty,
        };
    }

    /// Releases all backing memory.
    pub fn deinit(*_) {
        _.intervals.deinit(_.allocator);
    }

    /// Clears all intervals while retaining allocated capacity.
    pub fn clear(*_) {
        _.intervals.clearRetainingCapacity();
    }

    /// Returns a read-only slice of the canonical sorted disjoint intervals.
    pub fn items(): []const Interval {
        return _.intervals.items;
    }

    /// Deep-clones this `IntervalSet` using `allocator`.
    pub fn clone(allocator: std.mem.Allocator): IntervalSet ! IntervalError {
        let copy = IntervalSet.init(allocator);
        errdefer copy.deinit();
        copy.intervals.appendSlice(allocator, _.intervals.items) catch return IntervalError.OutOfMemory;
        return copy;
    }

    /// Adds `[start, end)` to the set, merging any overlapping or adjacent intervals.
    /// Empty intervals (`start == end`) are a no-op; `start > end` returns `IntervalError.InvalidInterval`.
    pub fn add(*_, start: i64, end: i64): void ! IntervalError {
        if (start > end) {
            return IntervalError.InvalidInterval;
        }
        if (start == end) {
            return;
        }

        const list = _.intervals.items;
        // Find first index `lo` where list[lo].end >= start (touches or overlaps).
        let lo: usize = 0;
        let hi: usize = list.len;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            if (list[mid].end < start) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }

        let new_start = start;
        let new_end = end;
        let merge_end = lo;
        while (merge_end < list.len && list[merge_end].start <= end) : (merge_end += 1) {
            if (list[merge_end].start < new_start) {
                new_start = list[merge_end].start;
            }
            if (list[merge_end].end > new_end) {
                new_end = list[merge_end].end;
            }
        }

        const merged = Interval{ start: new_start, end: new_end };
        if (lo == merge_end) {
            _.intervals.insert(_.allocator, lo, merged) catch return IntervalError.OutOfMemory;
            return;
        }

        _.intervals.items[lo] = merged;
        const remove_count = merge_end - lo - 1;
        if (remove_count > 0) {
            let dst = lo + 1;
            let src = merge_end;
            while (src < _.intervals.items.len) {
                _.intervals.items[dst] = _.intervals.items[src];
                dst += 1;
                src += 1;
            }
            _.intervals.items.len -= remove_count;
        }
    }

    /// Subtracts `[start, end)` from the set, handling full removal, truncation, and splitting.
    /// Empty intervals (`start == end`) are a no-op; `start > end` returns `IntervalError.InvalidInterval`.
    pub fn subtract(*_, start: i64, end: i64): void ! IntervalError {
        if (start > end) {
            return IntervalError.InvalidInterval;
        }
        if (start == end || _.intervals.items.len == 0) {
            return;
        }

        const list = _.intervals.items;
        // Find first index `lo` where list[lo].end > start.
        let lo: usize = 0;
        let hi: usize = list.len;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            if (list[mid].end <= start) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }

        if (lo >= list.len || list[lo].start >= end) {
            return;
        }

        // Single interval strictly containing [start, end): split into two.
        if (list[lo].start < start && list[lo].end > end) {
            const left = Interval{ start: list[lo].start, end: start };
            const right = Interval{ start: end, end: list[lo].end };
            _.intervals.items[lo] = left;
            _.intervals.insert(_.allocator, lo + 1, right) catch return IntervalError.OutOfMemory;
            return;
        }

        let span_end = lo;
        while (span_end < list.len && list[span_end].start < end) : (span_end += 1) {}

        let has_left = false;
        let left_iv = Interval{ start: 0, end: 0 };
        if (list[lo].start < start) {
            has_left = true;
            left_iv = Interval{ start: list[lo].start, end: start };
        }

        let has_right = false;
        let right_iv = Interval{ start: 0, end: 0 };
        if (list[span_end - 1].end > end) {
            has_right = true;
            right_iv = Interval{ start: end, end: list[span_end - 1].end };
        }

        let write_idx = lo;
        if (has_left) {
            _.intervals.items[write_idx] = left_iv;
            write_idx += 1;
        }
        if (has_right) {
            _.intervals.items[write_idx] = right_iv;
            write_idx += 1;
        }

        const old_count = span_end - lo;
        const new_count = write_idx - lo;
        const remove_count = old_count - new_count;
        if (remove_count > 0) {
            let dst = write_idx;
            let src = span_end;
            while (src < _.intervals.items.len) {
                _.intervals.items[dst] = _.intervals.items[src];
                dst += 1;
                src += 1;
            }
            _.intervals.items.len -= remove_count;
        }
    }

    /// Returns `true` if point `p` is contained in any interval of the set.
    pub fn contains(p: i64): bool {
        const list = _.intervals.items;
        let lo: usize = 0;
        let hi: usize = list.len;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            if (list[mid].end <= p) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo < list.len && list[lo].start <= p;
    }

    /// Returns `true` if the entire half-open range `[start, end)` is covered by this set.
    /// Empty range `[x, x)` is vacuously `true`; `start > end` returns `IntervalError.InvalidInterval`.
    pub fn containsRange(start: i64, end: i64): bool ! IntervalError {
        if (start > end) {
            return IntervalError.InvalidInterval;
        }
        if (start == end) {
            return true;
        }
        const list = _.intervals.items;
        let lo: usize = 0;
        let hi: usize = list.len;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            if (list[mid].end < end) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo < list.len && list[lo].start <= start;
    }

    /// Returns the total covered length across all intervals in the set.
    pub fn totalLength(): u64 {
        let total: u64 = 0;
        for (iv of _.intervals.items) {
            total += iv.len();
        }
        return total;
    }

    /// Extracts the gap intervals between consecutive intervals in the set.
    /// Caller owns the returned slice and must free it with `allocator.free(gaps)`.
    pub fn gaps(allocator: std.mem.Allocator): []Interval ! IntervalError {
        const list = _.intervals.items;
        if (list.len <= 1) {
            return allocator.alloc(Interval, 0) catch return IntervalError.OutOfMemory;
        }
        let out = allocator.alloc(Interval, list.len - 1) catch return IntervalError.OutOfMemory;
        let i: usize = 0;
        while (i + 1 < list.len) : (i += 1) {
            out[i] = Interval{ start: list[i].end, end: list[i + 1].start };
        }
        return out;
    }

    /// Linear O(M + N) set union returning a new canonical `IntervalSet`.
    pub fn unionWith(allocator: std.mem.Allocator, other: IntervalSet): IntervalSet ! IntervalError {
        let res = IntervalSet.init(allocator);
        errdefer res.deinit();

        const a = _.intervals.items;
        const b = other.intervals.items;
        let i: usize = 0;
        let j: usize = 0;

        while (i < a.len || j < b.len) {
            let next: Interval = undefined;
            if (j >= b.len || (i < a.len && a[i].start <= b[j].start)) {
                next = a[i];
                i += 1;
            } else {
                next = b[j];
                j += 1;
            }

            const rlen = res.intervals.items.len;
            if (rlen > 0 && res.intervals.items[rlen - 1].end >= next.start) {
                if (next.end > res.intervals.items[rlen - 1].end) {
                    res.intervals.items[rlen - 1].end = next.end;
                }
            } else {
                res.intervals.append(allocator, next) catch return IntervalError.OutOfMemory;
            }
        }
        return res;
    }

    /// Linear O(M + N) set intersection returning a new canonical `IntervalSet`.
    pub fn intersectWith(allocator: std.mem.Allocator, other: IntervalSet): IntervalSet ! IntervalError {
        let res = IntervalSet.init(allocator);
        errdefer res.deinit();

        const a = _.intervals.items;
        const b = other.intervals.items;
        let i: usize = 0;
        let j: usize = 0;

        while (i < a.len && j < b.len) {
            const s = if (a[i].start > b[j].start) a[i].start else b[j].start;
            const e = if (a[i].end < b[j].end) a[i].end else b[j].end;
            if (s < e) {
                res.intervals.append(allocator, Interval{ start: s, end: e }) catch return IntervalError.OutOfMemory;
            }
            if (a[i].end < b[j].end) {
                i += 1;
            } else {
                j += 1;
            }
        }
        return res;
    }

    /// Linear O(M + N) set difference (`self \ other`) returning a new canonical `IntervalSet`.
    pub fn differenceWith(allocator: std.mem.Allocator, other: IntervalSet): IntervalSet ! IntervalError {
        let res = IntervalSet.init(allocator);
        errdefer res.deinit();

        const a = _.intervals.items;
        const b = other.intervals.items;
        let i: usize = 0;
        let j: usize = 0;

        while (i < a.len) : (i += 1) {
            let cur_start = a[i].start;
            const cur_end = a[i].end;

            while (j < b.len && b[j].end <= cur_start) : (j += 1) {}

            while (j < b.len && b[j].start < cur_end) {
                if (b[j].start > cur_start) {
                    res.intervals.append(allocator, Interval{ start: cur_start, end: b[j].start }) catch return IntervalError.OutOfMemory;
                }
                if (b[j].end >= cur_end) {
                    cur_start = cur_end;
                    break;
                }
                cur_start = b[j].end;
                j += 1;
            }

            if (cur_start < cur_end) {
                res.intervals.append(allocator, Interval{ start: cur_start, end: cur_end }) catch return IntervalError.OutOfMemory;
            }
        }
        return res;
    }
}

/// Entry returned by `IntervalTree<V>` point and overlap queries.
pub struct IntervalEntry<V> {
    pub start: i64,
    pub end: i64,
    pub value: V,
}

struct IntervalNode<V> {
    start: i64,
    end: i64,
    max_end: i64,
    value: V,
    left: usize,
    right: usize,
    height: i32,
}

/// AVL-balanced Augmented Interval Tree supporting overlapping intervals with payload `V`
/// and subtree `max_end` pruning for point and range overlap queries.
pub struct IntervalTree<V> {
    allocator: std.mem.Allocator,
    nodes: std.ArrayListUnmanaged(IntervalNode(V)),
    root: usize,
    len_count: usize,

    /// Initializes an empty `IntervalTree<V>`.
    pub static fn init(allocator: std.mem.Allocator): IntervalTree<V> {
        return IntervalTree<V>{
            allocator: allocator,
            nodes: .empty,
            root: NULL_IDX,
            len_count: 0,
        };
    }

    /// Releases all tree nodes.
    pub fn deinit(*_) {
        _.nodes.deinit(_.allocator);
        _.root = NULL_IDX;
        _.len_count = 0;
    }

    /// Returns the number of intervals stored in the tree.
    pub fn count(): usize {
        return _.len_count;
    }

    fn nodeHeight(idx: usize): i32 {
        if (idx == NULL_IDX) {
            return 0;
        }
        return _.nodes.items[idx].height;
    }

    fn updateNode(*_, idx: usize) {
        let max_e = _.nodes.items[idx].end;
        const l = _.nodes.items[idx].left;
        const r = _.nodes.items[idx].right;
        let lh: i32 = 0;
        let rh: i32 = 0;
        if (l != NULL_IDX) {
            if (_.nodes.items[l].max_end > max_e) {
                max_e = _.nodes.items[l].max_end;
            }
            lh = _.nodes.items[l].height;
        }
        if (r != NULL_IDX) {
            if (_.nodes.items[r].max_end > max_e) {
                max_e = _.nodes.items[r].max_end;
            }
            rh = _.nodes.items[r].height;
        }
        _.nodes.items[idx].max_end = max_e;
        _.nodes.items[idx].height = 1 + (if (lh > rh) lh else rh);
    }

    fn balanceFactor(idx: usize): i32 {
        if (idx == NULL_IDX) {
            return 0;
        }
        return _.nodeHeight(_.nodes.items[idx].left) - _.nodeHeight(_.nodes.items[idx].right);
    }

    fn rotateRight(*_, y: usize): usize {
        const x = _.nodes.items[y].left;
        const t2 = _.nodes.items[x].right;
        _.nodes.items[x].right = y;
        _.nodes.items[y].left = t2;
        _.updateNode(y);
        _.updateNode(x);
        return x;
    }

    fn rotateLeft(*_, x: usize): usize {
        const y = _.nodes.items[x].right;
        const t2 = _.nodes.items[y].left;
        _.nodes.items[y].left = x;
        _.nodes.items[x].right = t2;
        _.updateNode(x);
        _.updateNode(y);
        return y;
    }

    fn insertNode(*_, node_idx: usize, new_idx: usize): usize {
        if (node_idx == NULL_IDX) {
            return new_idx;
        }

        const new_start = _.nodes.items[new_idx].start;
        const new_end = _.nodes.items[new_idx].end;
        const cur_start = _.nodes.items[node_idx].start;
        const cur_end = _.nodes.items[node_idx].end;

        if (new_start < cur_start || (new_start == cur_start && new_end < cur_end)) {
            const left_root = _.insertNode(_.nodes.items[node_idx].left, new_idx);
            _.nodes.items[node_idx].left = left_root;
        } else {
            const right_root = _.insertNode(_.nodes.items[node_idx].right, new_idx);
            _.nodes.items[node_idx].right = right_root;
        }

        _.updateNode(node_idx);
        const bf = _.balanceFactor(node_idx);

        if (bf > 1) {
            const l = _.nodes.items[node_idx].left;
            if (_.balanceFactor(l) < 0) {
                _.nodes.items[node_idx].left = _.rotateLeft(l);
            }
            return _.rotateRight(node_idx);
        }
        if (bf < -1) {
            const r = _.nodes.items[node_idx].right;
            if (_.balanceFactor(r) > 0) {
                _.nodes.items[node_idx].right = _.rotateRight(r);
            }
            return _.rotateLeft(node_idx);
        }

        return node_idx;
    }

    /// Inserts `[start, end)` with payload `value`.
    /// Returns `IntervalError.InvalidInterval` if `start > end` and `IntervalError.EmptyInterval` if `start == end`.
    pub fn insert(*_, start: i64, end: i64, value: V): void ! IntervalError {
        if (start > end) {
            return IntervalError.InvalidInterval;
        }
        if (start == end) {
            return IntervalError.EmptyInterval;
        }

        const new_idx = _.nodes.items.len;
        _.nodes.append(_.allocator, IntervalNode<V>{
            start: start,
            end: end,
            max_end: end,
            value: value,
            left: NULL_IDX,
            right: NULL_IDX,
            height: 1,
        }) catch return IntervalError.OutOfMemory;

        _.root = _.insertNode(_.root, new_idx);
        _.len_count += 1;
    }

    /// Queries all intervals containing point `p` (`start <= p < end`) using `max_end` subtree pruning.
    /// Caller owns the returned slice and must free it with `allocator.free(res)`.
    pub fn queryPoint(allocator: std.mem.Allocator, p: i64): []IntervalEntry<V> ! IntervalError {
        let results = std.ArrayListUnmanaged(IntervalEntry(V)).empty;
        errdefer results.deinit(allocator);

        if (_.root == NULL_IDX || _.nodes.items[_.root].max_end <= p) {
            return results.toOwnedSlice(allocator) catch return IntervalError.OutOfMemory;
        }

        let stack = std.ArrayListUnmanaged(usize).empty;
        defer stack.deinit(allocator);
        stack.append(allocator, _.root) catch return IntervalError.OutOfMemory;

        while (stack.items.len > 0) {
            const idx = stack.pop().?;
            const node = _.nodes.items[idx];
            if (node.max_end <= p) {
                continue;
            }
            if (node.right != NULL_IDX && node.start <= p && _.nodes.items[node.right].max_end > p) {
                stack.append(allocator, node.right) catch return IntervalError.OutOfMemory;
            }
            if (node.start <= p && p < node.end) {
                results.append(allocator, IntervalEntry<V>{
                    start: node.start,
                    end: node.end,
                    value: node.value,
                }) catch return IntervalError.OutOfMemory;
            }
            if (node.left != NULL_IDX && _.nodes.items[node.left].max_end > p) {
                stack.append(allocator, node.left) catch return IntervalError.OutOfMemory;
            }
        }

        return results.toOwnedSlice(allocator) catch return IntervalError.OutOfMemory;
    }

    /// Queries all intervals overlapping `[start, end)` using `max_end` subtree pruning.
    /// Returns `IntervalError.InvalidInterval` if `start > end`.
    /// Caller owns the returned slice and must free it with `allocator.free(res)`.
    pub fn queryOverlap(allocator: std.mem.Allocator, start: i64, end: i64): []IntervalEntry<V> ! IntervalError {
        if (start > end) {
            return IntervalError.InvalidInterval;
        }

        let results = std.ArrayListUnmanaged(IntervalEntry(V)).empty;
        errdefer results.deinit(allocator);

        if (start == end || _.root == NULL_IDX || _.nodes.items[_.root].max_end <= start) {
            return results.toOwnedSlice(allocator) catch return IntervalError.OutOfMemory;
        }

        let stack = std.ArrayListUnmanaged(usize).empty;
        defer stack.deinit(allocator);
        stack.append(allocator, _.root) catch return IntervalError.OutOfMemory;

        while (stack.items.len > 0) {
            const idx = stack.pop().?;
            const node = _.nodes.items[idx];
            if (node.max_end <= start) {
                continue;
            }
            if (node.right != NULL_IDX && node.start < end && _.nodes.items[node.right].max_end > start) {
                stack.append(allocator, node.right) catch return IntervalError.OutOfMemory;
            }
            if (node.start < end && start < node.end) {
                results.append(allocator, IntervalEntry<V>{
                    start: node.start,
                    end: node.end,
                    value: node.value,
                }) catch return IntervalError.OutOfMemory;
            }
            if (node.left != NULL_IDX && _.nodes.items[node.left].max_end > start) {
                stack.append(allocator, node.left) catch return IntervalError.OutOfMemory;
            }
        }

        return results.toOwnedSlice(allocator) catch return IntervalError.OutOfMemory;
    }
}
