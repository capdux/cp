// Source: cp/lib/geo_quadtree/quadtree.cp (Pure CP 2D Spatial Quadtree Index)
const std = @import("std");

pub const QuadError = error{
    InvalidCapacity,
    InvalidBounds,
};

pub const Rect = struct {
    cx: f64,
    cy: f64,
    half_w: f64,
    half_h: f64,

    pub fn contains(x: f64, y: f64): bool {
        return x >= _.cx - _.half_w and x <= _.cx + _.half_w and
            y >= _.cy - _.half_h and y <= _.cy + _.half_h;
    }

    pub fn intersects(other: Rect): bool {
        const dx = @abs(_.cx - other.cx);
        const dy = @abs(_.cy - other.cy);
        return dx <= (_.half_w + other.half_w) and dy <= (_.half_h + other.half_h);
    }
};

pub fn PointItem(comptime V: type): type {
    return struct {
        x: f64,
        y: f64,
        value: V,
    };
}

pub fn QuadTree(comptime V: type): type {
    const Entry = PointItem(V);

    const Node = struct {
        bounds: Rect,
        points: std.ArrayList(Entry),
        divided: bool,
        nw: (*@This())?,
        ne: (*@This())?,
        sw: (*@This())?,
        se: (*@This())?,
    };

    return struct {
        root: *Node,
        capacity: usize,
        total_count: usize,
        alloc: std.mem.Allocator,

        const Self = @This();
        pub const Item = Entry;

        static fn createNode(alloc: std.mem.Allocator, bounds: Rect): !*Node {
            let n = try alloc.create(Node);
            n.* = Node{
                .bounds = bounds,
                .points = std.ArrayList(Entry).empty,
                .divided = false,
                .nw = null,
                .ne = null,
                .sw = null,
                .se = null,
            };
            return n;
        }

        static fn destroyNode(alloc: std.mem.Allocator, node: *Node) {
            if (c := node.nw) { destroyNode(alloc, c); }
            if (c := node.ne) { destroyNode(alloc, c); }
            if (c := node.sw) { destroyNode(alloc, c); }
            if (c := node.se) { destroyNode(alloc, c); }
            node.points.deinit(alloc);
            alloc.destroy(node);
        }

        pub static fn init(alloc: std.mem.Allocator, bounds: Rect, capacity: usize): !Self {
            if (capacity == 0) { return QuadError.InvalidCapacity; }
            if (bounds.half_w <= 0.0 or bounds.half_h <= 0.0) { return QuadError.InvalidBounds; }
            const root = try createNode(alloc, bounds);
            return Self{
                .root = root,
                .capacity = capacity,
                .total_count = 0,
                .alloc = alloc,
            };
        }

        pub fn deinit(*_) {
            destroyNode(_.alloc, _.root);
            _.total_count = 0;
        }

        pub fn len(): usize {
            return _.total_count;
        }

        static fn subdivide(alloc: std.mem.Allocator, node: *Node): !void {
            const hw = node.bounds.half_w * 0.5;
            const hh = node.bounds.half_h * 0.5;
            const cx = node.bounds.cx;
            const cy = node.bounds.cy;

            node.nw = try createNode(alloc, Rect{ .cx = cx - hw, .cy = cy + hh, .half_w = hw, .half_h = hh });
            node.ne = try createNode(alloc, Rect{ .cx = cx + hw, .cy = cy + hh, .half_w = hw, .half_h = hh });
            node.sw = try createNode(alloc, Rect{ .cx = cx - hw, .cy = cy - hh, .half_w = hw, .half_h = hh });
            node.se = try createNode(alloc, Rect{ .cx = cx + hw, .cy = cy - hh, .half_w = hw, .half_h = hh });
            node.divided = true;
        }

        static fn insertInto(alloc: std.mem.Allocator, cap_limit: usize, node: *Node, item: Entry, depth: usize): !bool {
            if (!node.bounds.contains(item.x, item.y)) { return false; }

            if (!node.divided and (node.points.items.len < cap_limit or depth >= 24)) {
                try node.points.append(alloc, item);
                return true;
            }
            if (!node.divided) {
                try subdivide(alloc, node);
            }
            if (nw := node.nw) {
                if (try insertInto(alloc, cap_limit, nw, item, depth + 1)) { return true; }
            }
            if (ne := node.ne) {
                if (try insertInto(alloc, cap_limit, ne, item, depth + 1)) { return true; }
            }
            if (sw := node.sw) {
                if (try insertInto(alloc, cap_limit, sw, item, depth + 1)) { return true; }
            }
            if (se := node.se) {
                if (try insertInto(alloc, cap_limit, se, item, depth + 1)) { return true; }
            }
            return false;
        }

        pub fn insert(*_, x: f64, y: f64, value: V): !bool {
            const ok = try insertInto(_.alloc, _.capacity, _.root, Entry{ .x = x, .y = y, .value = value }, 0);
            if (ok) { _.total_count += 1; }
            return ok;
        }

        static fn queryRectNode(node: *Node, range: Rect, alloc: std.mem.Allocator, out: *std.ArrayList(Entry)): !void {
            if (!node.bounds.intersects(range)) { return; }
            for (pt of node.points.items) {
                if (range.contains(pt.x, pt.y)) {
                    try out.append(alloc, pt);
                }
            }
            if (node.divided) {
                if (nw := node.nw) { try queryRectNode(nw, range, alloc, out); }
                if (ne := node.ne) { try queryRectNode(ne, range, alloc, out); }
                if (sw := node.sw) { try queryRectNode(sw, range, alloc, out); }
                if (se := node.se) { try queryRectNode(se, range, alloc, out); }
            }
        }

        pub fn queryRect(alloc: std.mem.Allocator, range: Rect): ![]Entry {
            let out = std.ArrayList(Entry).empty;
            errdefer out.deinit(alloc);
            try queryRectNode(_.root, range, alloc, &out);
            return out.toOwnedSlice(alloc);
        }

        pub fn queryRadius(alloc: std.mem.Allocator, cx: f64, cy: f64, radius: f64): ![]Entry {
            const bbox = Rect{ .cx = cx, .cy = cy, .half_w = radius, .half_h = radius };
            const candidates = try _.queryRect(alloc, bbox);
            defer alloc.free(candidates);

            let out = std.ArrayList(Entry).empty;
            errdefer out.deinit(alloc);
            const r2 = radius * radius;
            for (pt of candidates) {
                const dx = pt.x - cx;
                const dy = pt.y - cy;
                if (dx * dx + dy * dy <= r2) {
                    try out.append(alloc, pt);
                }
            }
            return out.toOwnedSlice(alloc);
        }
    };
}
