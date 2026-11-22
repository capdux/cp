// Source: cp/lib/dsu/dsu.cp (Disjoint-Set Union, Weighted DSU & Rollback DSU)
// Pure CP zero-C-dependency Disjoint-Set Union family:
//   - DisjointSet: path compression + union by size
//   - WeightedDsu: difference-constraint DSU maintaining val[x] - val[parent[x]] = weight[x]
//   - RollbackDsu: union by size + history stack for O(log N) deterministic rollback

const std = @import("std");

/// Error set for DSU operations.
pub err DsuError {
    IndexOutOfBounds,
    ConflictConstraint,
    NothingToRollback,
    OutOfMemory,
}

/// Standard Disjoint-Set Union with iterative path compression and union by size.
pub struct DisjointSet {
    alloc: std.mem.Allocator,
    parent: []usize = [],
    size: []usize = [],
    count: usize = 0,

    /// Initialize a DisjointSet with `n` isolated elements `[0, n)`.
    pub fn init(allocator: std.mem.Allocator, n: usize): DisjointSet ! DsuError {
        if (n == 0) {
            return DisjointSet{
                alloc: allocator,
                parent: [],
                size: [],
                count: 0,
            };
        }
        let parent = allocator.alloc(usize, n) catch return DsuError.OutOfMemory;
        let size = allocator.alloc(usize, n) catch {
            allocator.free(parent);
            return DsuError.OutOfMemory;
        };
        let i: usize = 0;
        while (i < n) {
            parent[i] = i;
            size[i] = 1;
            i += 1;
        }
        return DisjointSet{
            alloc: allocator,
            parent: parent,
            size: size,
            count: n,
        };
    }

    /// Free internal buffers.
    pub fn deinit(*_) {
        if (_.parent.len > 0) {
            _.alloc.free(_.parent);
            _.parent = [];
        }
        if (_.size.len > 0) {
            _.alloc.free(_.size);
            _.size = [];
        }
        _.count = 0;
    }

    /// Find the representative root of `x` with full iterative path compression.
    pub fn find(*_, x: usize): usize ! DsuError {
        if (x >= _.parent.len) return DsuError.IndexOutOfBounds;
        let root = x;
        while (_.parent[root] != root) {
            root = _.parent[root];
        }
        let curr = x;
        while (curr != root) {
            const next = _.parent[curr];
            _.parent[curr] = root;
            curr = next;
        }
        return root;
    }

    /// Unite the sets containing `a` and `b`. Returns `true` if a merge occurred, `false` if already connected.
    pub fn unite(*_, a: usize, b: usize): bool ! DsuError {
        const ra = _.find(a)?;
        const rb = _.find(b)?;
        if (ra == rb) return false;

        if (_.size[ra] < _.size[rb]) {
            _.parent[ra] = rb;
            _.size[rb] += _.size[ra];
        } else {
            _.parent[rb] = ra;
            _.size[ra] += _.size[rb];
        }
        _.count -= 1;
        return true;
    }

    /// Return `true` if `a` and `b` belong to the same set.
    pub fn connected(*_, a: usize, b: usize): bool ! DsuError {
        const ra = _.find(a)?;
        const rb = _.find(b)?;
        return ra == rb;
    }

    /// Return the size of the connected component containing `x`.
    pub fn componentSize(*_, x: usize): usize ! DsuError {
        const root = _.find(x)?;
        return _.size[root];
    }

    /// Return the current number of disjoint components.
    pub fn componentCount(): usize {
        return _.count;
    }

    /// Reset all elements to isolated singletons without reallocating.
    pub fn reset(*_) {
        let i: usize = 0;
        while (i < _.parent.len) {
            _.parent[i] = i;
            _.size[i] = 1;
            i += 1;
        }
        _.count = _.parent.len;
    }

    /// Partition all elements into grouped slices. Caller must free each inner slice and the outer slice via `allocator`.
    pub fn groups(*_, allocator: std.mem.Allocator): [][]usize ! DsuError {
        const n = _.parent.len;
        if (n == 0) {
            return [];
        }

        let roots = allocator.alloc(usize, n) catch return DsuError.OutOfMemory;
        defer allocator.free(roots);

        let root_to_gid = allocator.alloc(usize, n) catch return DsuError.OutOfMemory;
        defer allocator.free(root_to_gid);
        @memset(root_to_gid, n);

        let num_groups: usize = 0;
        let i: usize = 0;
        while (i < n) {
            const r = _.find(i)?;
            roots[i] = r;
            if (root_to_gid[r] == n) {
                root_to_gid[r] = num_groups;
                num_groups += 1;
            }
            i += 1;
        }

        let counts = allocator.alloc(usize, num_groups) catch return DsuError.OutOfMemory;
        defer allocator.free(counts);
        @memset(counts, 0);

        i = 0;
        while (i < n) {
            counts[root_to_gid[roots[i]]] += 1;
            i += 1;
        }

        let res = allocator.alloc([]usize, num_groups) catch return DsuError.OutOfMemory;
        let allocated: usize = 0;
        errdefer {
            let k: usize = 0;
            while (k < allocated) {
                allocator.free(res[k]);
                k += 1;
            }
            allocator.free(res);
        }

        while (allocated < num_groups) {
            res[allocated] = allocator.alloc(usize, counts[allocated]) catch return DsuError.OutOfMemory;
            allocated += 1;
        }

        let cursors = allocator.alloc(usize, num_groups) catch return DsuError.OutOfMemory;
        defer allocator.free(cursors);
        @memset(cursors, 0);

        i = 0;
        while (i < n) {
            const gid = root_to_gid[roots[i]];
            const pos = cursors[gid];
            res[gid][pos] = i;
            cursors[gid] = pos + 1;
            i += 1;
        }

        return res;
    }
}

/// Weighted / Difference-Constraint Disjoint-Set Union.
/// Maintains invariant: `val[x] - val[parent[x]] == weight[x]`.
pub struct WeightedDsu {
    alloc: std.mem.Allocator,
    parent: []usize = [],
    size: []usize = [],
    weight: []i64 = [],
    count: usize = 0,

    /// Initialize a WeightedDsu with `n` isolated elements `[0, n)`.
    pub fn init(allocator: std.mem.Allocator, n: usize): WeightedDsu ! DsuError {
        if (n == 0) {
            return WeightedDsu{
                alloc: allocator,
                parent: [],
                size: [],
                weight: [],
                count: 0,
            };
        }
        let parent = allocator.alloc(usize, n) catch return DsuError.OutOfMemory;
        let size = allocator.alloc(usize, n) catch {
            allocator.free(parent);
            return DsuError.OutOfMemory;
        };
        let weight = allocator.alloc(i64, n) catch {
            allocator.free(size);
            allocator.free(parent);
            return DsuError.OutOfMemory;
        };

        let i: usize = 0;
        while (i < n) {
            parent[i] = i;
            size[i] = 1;
            weight[i] = 0;
            i += 1;
        }
        return WeightedDsu{
            alloc: allocator,
            parent: parent,
            size: size,
            weight: weight,
            count: n,
        };
    }

    /// Free internal buffers.
    pub fn deinit(*_) {
        if (_.parent.len > 0) {
            _.alloc.free(_.parent);
            _.parent = [];
        }
        if (_.size.len > 0) {
            _.alloc.free(_.size);
            _.size = [];
        }
        if (_.weight.len > 0) {
            _.alloc.free(_.weight);
            _.weight = [];
        }
        _.count = 0;
    }

    /// Find root of `x` while compressing path and accumulating relative weights.
    pub fn find(*_, x: usize): usize ! DsuError {
        if (x >= _.parent.len) return DsuError.IndexOutOfBounds;
        let root = x;
        let total_w: i64 = 0;
        while (_.parent[root] != root) {
            total_w += _.weight[root];
            root = _.parent[root];
        }
        let curr = x;
        while (curr != root) {
            const next = _.parent[curr];
            const curr_w = _.weight[curr];
            _.weight[curr] = total_w;
            _.parent[curr] = root;
            total_w -= curr_w;
            curr = next;
        }
        return root;
    }

    /// Add constraint `val[a] - val[b] == difference`.
    /// Returns `true` if a new merge occurred, `false` if already consistent,
    /// or `DsuError.ConflictConstraint` if it contradicts existing constraints.
    pub fn relate(*_, a: usize, b: usize, difference: i64): bool ! DsuError {
        const ra = _.find(a)?;
        const rb = _.find(b)?;
        if (ra == rb) {
            const actual = @as(i128, _.weight[a]) - @as(i128, _.weight[b]);
            if (actual != @as(i128, difference)) {
                return DsuError.ConflictConstraint;
            }
            return false;
        }

        if (_.size[ra] < _.size[rb]) {
            _.parent[ra] = rb;
            _.weight[ra] = difference + _.weight[b] - _.weight[a];
            _.size[rb] += _.size[ra];
        } else {
            _.parent[rb] = ra;
            _.weight[rb] = _.weight[a] - _.weight[b] - difference;
            _.size[ra] += _.size[rb];
        }
        _.count -= 1;
        return true;
    }

    /// Query `val[a] - val[b]` if `a` and `b` are connected, or `null` otherwise.
    pub fn diff(*_, a: usize, b: usize): i64? ! DsuError {
        const ra = _.find(a)?;
        const rb = _.find(b)?;
        if (ra != rb) return null;
        return _.weight[a] - _.weight[b];
    }

    /// Return the current number of disjoint components.
    pub fn componentCount(): usize {
        return _.count;
    }
}

struct RollbackOp {
    child: usize,
    parent: usize,
    prev_parent_size: usize,
}

/// Persistent Rollback Disjoint-Set Union (union by size without path compression).
/// Guarantees O(log N) find and O(1) per-merge undo / snapshot rollback.
pub struct RollbackDsu {
    alloc: std.mem.Allocator,
    parent: []usize = [],
    size: []usize = [],
    count: usize = 0,
    history: []RollbackOp = [],
    history_len: usize = 0,

    /// Initialize a RollbackDsu with `n` isolated elements `[0, n)`.
    pub fn init(allocator: std.mem.Allocator, n: usize): RollbackDsu ! DsuError {
        if (n == 0) {
            return RollbackDsu{
                alloc: allocator,
                parent: [],
                size: [],
                count: 0,
                history: [],
                history_len: 0,
            };
        }
        let parent = allocator.alloc(usize, n) catch return DsuError.OutOfMemory;
        let size = allocator.alloc(usize, n) catch {
            allocator.free(parent);
            return DsuError.OutOfMemory;
        };
        let i: usize = 0;
        while (i < n) {
            parent[i] = i;
            size[i] = 1;
            i += 1;
        }
        return RollbackDsu{
            alloc: allocator,
            parent: parent,
            size: size,
            count: n,
            history: [],
            history_len: 0,
        };
    }

    /// Free internal buffers.
    pub fn deinit(*_) {
        if (_.parent.len > 0) {
            _.alloc.free(_.parent);
            _.parent = [];
        }
        if (_.size.len > 0) {
            _.alloc.free(_.size);
            _.size = [];
        }
        if (_.history.len > 0) {
            _.alloc.free(_.history);
            _.history = [];
        }
        _.history_len = 0;
        _.count = 0;
    }

    fn ensureHistoryCapacity(*_, needed: usize): void ! DsuError {
        if (needed <= _.history.len) return;
        let new_cap = if (_.history.len == 0) 8 else _.history.len * 2;
        if (new_cap < needed) new_cap = needed;
        const next_buf = _.alloc.alloc(RollbackOp, new_cap) catch return DsuError.OutOfMemory;
        if (_.history_len > 0) {
            @memcpy(next_buf[0.._.history_len], _.history[0.._.history_len]);
        }
        if (_.history.len > 0) {
            _.alloc.free(_.history);
        }
        _.history = next_buf;
    }

    /// Find root of `x` without path compression (strict O(log N)).
    pub fn find(x: usize): usize ! DsuError {
        if (x >= _.parent.len) return DsuError.IndexOutOfBounds;
        let curr = x;
        while (_.parent[curr] != curr) {
            curr = _.parent[curr];
        }
        return curr;
    }

    /// Capture an opaque snapshot token representing current history depth.
    pub fn snapshot(): usize {
        return _.history_len;
    }

    /// Unite `a` and `b` and push an undo record onto the history stack.
    pub fn unite(*_, a: usize, b: usize): bool ! DsuError {
        const ra = _.find(a)?;
        const rb = _.find(b)?;
        if (ra == rb) return false;

        _.ensureHistoryCapacity(_.history_len + 1)?;

        let child = ra;
        let p = rb;
        if (_.size[ra] >= _.size[rb]) {
            child = rb;
            p = ra;
        }

        _.history[_.history_len] = RollbackOp{
            child: child,
            parent: p,
            prev_parent_size: _.size[p],
        };
        _.history_len += 1;

        _.parent[child] = p;
        _.size[p] += _.size[child];
        _.count -= 1;
        return true;
    }

    /// Undo the most recent merge operation.
    pub fn undo(*_): void ! DsuError {
        if (_.history_len == 0) return DsuError.NothingToRollback;
        _.history_len -= 1;
        const op = _.history[_.history_len];
        _.parent[op.child] = op.child;
        _.size[op.parent] = op.prev_parent_size;
        _.count += 1;
    }

    /// Roll back all merges performed after `snap` was captured.
    pub fn rollbackTo(*_, snap: usize) {
        while (_.history_len > snap) {
            _.undo() catch break;
        }
    }

    /// Return `true` if `a` and `b` belong to the same set.
    pub fn connected(a: usize, b: usize): bool ! DsuError {
        const ra = _.find(a)?;
        const rb = _.find(b)?;
        return ra == rb;
    }

    /// Return the current number of disjoint components.
    pub fn componentCount(): usize {
        return _.count;
    }
}
