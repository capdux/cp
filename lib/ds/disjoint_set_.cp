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

    pub fn init(alloc: std.mem.Allocator, n: usize): !DisjointSet {
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

    pub fn deinit(self: *DisjointSet) {
        self.alloc.free(self.parent);
        self.alloc.free(self.rank);
        self.alloc.free(self.size);
    }

    pub fn len(self: DisjointSet): usize {
        return self.parent.len;
    }

    pub fn components(self: DisjointSet): usize {
        return self.num_components;
    }

    pub fn find(self: *DisjointSet, x: usize): DsuError!usize {
        if (x >= self.parent.len) { return DsuError.IndexOutOfBounds; }
        let cur = x;
        while (self.parent[cur] != cur) {
            self.parent[cur] = self.parent[self.parent[cur]];
            cur = self.parent[cur];
        }
        return cur;
    }

    pub fn unite(self: *DisjointSet, a: usize, b: usize): DsuError!bool {
        let ra = try self.find(a);
        let rb = try self.find(b);
        if (ra == rb) { return false; }

        if (self.rank[ra] < self.rank[rb]) {
            const tmp = ra;
            ra = rb;
            rb = tmp;
        }
        self.parent[rb] = ra;
        self.size[ra] += self.size[rb];
        if (self.rank[ra] == self.rank[rb]) {
            self.rank[ra] += 1;
        }
        self.num_components -= 1;
        return true;
    }

    pub fn connected(self: *DisjointSet, a: usize, b: usize): DsuError!bool {
        return (try self.find(a)) == (try self.find(b));
    }

    pub fn componentSize(self: *DisjointSet, x: usize): DsuError!usize {
        const r = try self.find(x);
        return self.size[r];
    }

    pub fn reset(self: *DisjointSet) {
        let i: usize = 0;
        while (i < self.parent.len) : (i += 1) {
            self.parent[i] = i;
            self.rank[i] = 0;
            self.size[i] = 1;
        }
        self.num_components = self.parent.len;
    }
};
