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

    pub fn deinit(*_) {
        for (n of _.nodes.items) {
            _.alloc.free(n);
        }
        _.nodes.deinit(_.alloc);
        _.points.deinit(_.alloc);
    }

    pub fn nodeCount(): usize {
        return _.nodes.items.len;
    }

    pub fn addNode(*_, node_id: []const u8): !void {
        for (existing of _.nodes.items) {
            if (std.mem.eql(u8, existing, node_id)) { return; }
        }
        const dup = try _.alloc.dupe(u8, node_id);
        errdefer _.alloc.free(dup);
        const node_idx = _.nodes.items.len;
        try _.nodes.append(_.alloc, dup);

        let v: usize = 0;
        while (v < _.vnodes_per_node) : (v += 1) {
            try _.points.append(_.alloc, RingPoint{
                .hash = hashVNode(dup, v),
                .node_idx = node_idx,
            });
        }
        std.mem.sort(RingPoint, _.points.items, {}, ringPointLess);
    }

    pub fn removeNode(*_, node_id: []const u8): ConsistentHashError!void {
        let found_idx: usize? = null;
        let i: usize = 0;
        while (i < _.nodes.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.nodes.items[i], node_id)) {
                found_idx = i;
                break;
            }
        }
        const target_idx = found_idx orelse return ConsistentHashError.NodeNotFound;

        _.alloc.free(_.nodes.items[target_idx]);
        * = _.nodes.orderedRemove(target_idx);

        // Rebuild ring points cleanly
        _.points.clearRetainingCapacity();
        let ni: usize = 0;
        while (ni < _.nodes.items.len) : (ni += 1) {
            let v: usize = 0;
            while (v < _.vnodes_per_node) : (v += 1) {
                _.points.append(_.alloc, RingPoint{
                    .hash = hashVNode(_.nodes.items[ni], v),
                    .node_idx = ni,
                }) catch unreachable;
            }
        }
        std.mem.sort(RingPoint, _.points.items, {}, ringPointLess);
    }

    fn lowerBound(h: u64): usize {
        let lo: usize = 0;
        let hi: usize = _.points.items.len;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            if (_.points.items[mid].hash < h) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return if (lo == _.points.items.len) 0 else lo;
    }

    pub fn getNode(key: []const u8): ConsistentHashError![]const u8 {
        if (_.points.items.len == 0) { return ConsistentHashError.EmptyRing; }
        const idx = _.lowerBound(hashKey(key));
        return _.nodes.items[_.points.items[idx].node_idx];
    }

    /// Return up to `count` distinct replica nodes clockwise on the ring
    pub fn getReplicas(alloc: std.mem.Allocator, key: []const u8, count: usize): ![]const []const u8 {
        if (_.points.items.len == 0) { return ConsistentHashError.EmptyRing; }
        const max_n = @min(count, _.nodes.items.len);
        let out = std.ArrayList(StrSlice).empty;
        errdefer out.deinit(alloc);

        let start = _.lowerBound(hashKey(key));
        let visited: usize = 0;
        while (visited < _.points.items.len and out.items.len < max_n) : (visited += 1) {
            const pos = (start + visited) % _.points.items.len;
            const candidate = _.nodes.items[_.points.items[pos].node_idx];
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
