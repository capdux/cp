// Source: cp/lib/consistent_hash/consistent_hash.cp (Pure CP Virtual-Node Consistent Hashing Ring)
const std = @import("std");
type StrSlice = []const u8;

pub const ConsistentHashError = error{
    EmptyRing,
    InvalidVirtualNodes,
    NodeNotFound,
};

const RingPoint = struct {
    hash: u64,
    node_idx: usize,
};

fn ringPointLess(_ctx: void, a: RingPoint, b: RingPoint): bool {
    * = _ctx;
    if (a.hash != b.hash) { return a.hash < b.hash; }
    return a.node_idx < b.node_idx;
}

fn hashKey(data: []const u8): u64 {
    return std.hash.Wyhash.hash(0x517cc1b727220a95, data);
}

fn hashVNode(node_id: []const u8, vnode_idx: usize): u64 {
    let buf: [8]u8 = undefined;
    std.mem.writeInt(u64, &buf, @intCast(vnode_idx), .little);
    let h = std.hash.Wyhash.init(0x9e3779b97f4a7c15);
    h.update(node_id);
    h.update(":");
    h.update(&buf);
    return h.final();
}

pub const HashRing = struct {
    nodes: std.ArrayList(StrSlice),
    points: std.ArrayList(RingPoint),
    vnodes_per_node: usize,
    alloc: std.mem.Allocator,

    pub fn init(alloc: std.mem.Allocator, vnodes_per_node: usize): ConsistentHashError!HashRing {
        if (vnodes_per_node == 0) { return ConsistentHashError.InvalidVirtualNodes; }
        return HashRing{
            .nodes = std.ArrayList(StrSlice).empty,
            .points = std.ArrayList(RingPoint).empty,
            .vnodes_per_node = vnodes_per_node,
            .alloc = alloc,
        };
    }

    pub fn deinit(self: *HashRing) {
        for (n of self.nodes.items) {
            self.alloc.free(n);
        }
        self.nodes.deinit(self.alloc);
        self.points.deinit(self.alloc);
    }

    pub fn nodeCount(self: HashRing): usize {
        return self.nodes.items.len;
    }

    pub fn addNode(self: *HashRing, node_id: []const u8): !void {
        for (existing of self.nodes.items) {
            if (std.mem.eql(u8, existing, node_id)) { return; }
        }
        const dup = try self.alloc.dupe(u8, node_id);
        errdefer self.alloc.free(dup);
        const node_idx = self.nodes.items.len;
        try self.nodes.append(self.alloc, dup);

        let v: usize = 0;
        while (v < self.vnodes_per_node) : (v += 1) {
            try self.points.append(self.alloc, RingPoint{
                .hash = hashVNode(dup, v),
                .node_idx = node_idx,
            });
        }
        std.mem.sort(RingPoint, self.points.items, {}, ringPointLess);
    }

    pub fn removeNode(self: *HashRing, node_id: []const u8): ConsistentHashError!void {
        let found_idx: usize? = null;
        let i: usize = 0;
        while (i < self.nodes.items.len) : (i += 1) {
            if (std.mem.eql(u8, self.nodes.items[i], node_id)) {
                found_idx = i;
                break;
            }
        }
        const target_idx = found_idx orelse return ConsistentHashError.NodeNotFound;

        self.alloc.free(self.nodes.items[target_idx]);
        * = self.nodes.orderedRemove(target_idx);

        // Rebuild ring points cleanly
        self.points.clearRetainingCapacity();
        let ni: usize = 0;
        while (ni < self.nodes.items.len) : (ni += 1) {
            let v: usize = 0;
            while (v < self.vnodes_per_node) : (v += 1) {
                self.points.append(self.alloc, RingPoint{
                    .hash = hashVNode(self.nodes.items[ni], v),
                    .node_idx = ni,
                }) catch unreachable;
            }
        }
        std.mem.sort(RingPoint, self.points.items, {}, ringPointLess);
    }

    fn lowerBound(self: HashRing, h: u64): usize {
        let lo: usize = 0;
        let hi: usize = self.points.items.len;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            if (self.points.items[mid].hash < h) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return if (lo == self.points.items.len) 0 else lo;
    }

    pub fn getNode(self: HashRing, key: []const u8): ConsistentHashError![]const u8 {
        if (self.points.items.len == 0) { return ConsistentHashError.EmptyRing; }
        const idx = self.lowerBound(hashKey(key));
        return self.nodes.items[self.points.items[idx].node_idx];
    }

    /// Return up to `count` distinct replica nodes clockwise on the ring
    pub fn getReplicas(self: HashRing, alloc: std.mem.Allocator, key: []const u8, count: usize): ![]const []const u8 {
        if (self.points.items.len == 0) { return ConsistentHashError.EmptyRing; }
        const max_n = @min(count, self.nodes.items.len);
        let out = std.ArrayList(StrSlice).empty;
        errdefer out.deinit(alloc);

        let start = self.lowerBound(hashKey(key));
        let visited: usize = 0;
        while (visited < self.points.items.len and out.items.len < max_n) : (visited += 1) {
            const pos = (start + visited) % self.points.items.len;
            const candidate = self.nodes.items[self.points.items[pos].node_idx];
            let dup = false;
            for (chosen of out.items) {
                if (std.mem.eql(u8, chosen, candidate)) {
                    dup = true;
                    break;
                }
            }
            if (!dup) {
                try out.append(alloc, candidate);
            }
        }
        return out.toOwnedSlice(alloc);
    }
};
