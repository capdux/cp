// Reference Only: Functional overlap with lib/ds/dsu.cp | Kept for comparison/analysis
// Source: cp/lib/disjoint_set/disjoint_set.cp (Pure CP Disjoint Set Union / Union-Find with Path Compression & Rank)
const std = @import("std");

pub const DsuError = error{
    IndexOutOfBounds,
};

pub const DisjointSet = struct {
    parent: []usize,
    rank: []u32,
    size: []usize,
    num_components: usize,
    alloc: std.mem.Allocator,

    pub static fn init(alloc: std.mem.Allocator, n: usize): !DisjointSet {
        let parent = try alloc.alloc(usize, n);
        errdefer alloc.free(parent);
        let rank = try alloc.alloc(u32, n);
        errdefer alloc.free(rank);
        let size = try alloc.alloc(usize, n);
        errdefer alloc.free(size);

        let i: usize = 0;
        while (i < n) : (i += 1) {
            parent[i] = i;
            rank[i] = 0;
            size[i] = 1;
        }
        return DisjointSet{
            .parent = parent,
            .rank = rank,
            .size = size,
            .num_components = n,
            .alloc = alloc,
        };
    }

    pub fn deinit(*_) {
        _.alloc.free(_.parent);
        _.alloc.free(_.rank);
        _.alloc.free(_.size);
    }

    pub fn len(): usize {
        return _.parent.len;
    }

    pub fn components(): usize {
        return _.num_components;
    }

    pub fn find(*_, x: usize): DsuError!usize {
        if (x >= _.parent.len) { return DsuError.IndexOutOfBounds; }
        let cur = x;
        while (_.parent[cur] != cur) {
            _.parent[cur] = _.parent[_.parent[cur]];
            cur = _.parent[cur];
        }
        return cur;
    }

    pub fn unite(*_, a: usize, b: usize): DsuError!bool {
        let ra = try _.find(a);
        let rb = try _.find(b);
        if (ra == rb) { return false; }

        if (_.rank[ra] < _.rank[rb]) {
            const tmp = ra;
            ra = rb;
            rb = tmp;
        }
        _.parent[rb] = ra;
        _.size[ra] += _.size[rb];
        if (_.rank[ra] == _.rank[rb]) {
            _.rank[ra] += 1;
        }
        _.num_components -= 1;
        return true;
    }

    pub fn connected(*_, a: usize, b: usize): DsuError!bool {
        return (try _.find(a)) == (try _.find(b));
    }

    pub fn componentSize(*_, x: usize): DsuError!usize {
        const r = try _.find(x);
        return _.size[r];
    }

    pub fn reset(*_) {
        let i: usize = 0;
        while (i < _.parent.len) : (i += 1) {
            _.parent[i] = i;
            _.rank[i] = 0;
            _.size[i] = 1;
        }
        _.num_components = _.parent.len;
    }
};
