// Reference Only: Functional overlap with lib/ds/radix.cp | Kept for comparison/analysis
// Source: native-cp-stdlib (Compressed Radix / Patricia Trie)

import std from "std";

export type StrSlice = []const u8;

export struct PrefixMatch<V> {
    key: []const u8,
    value: V,
}

fn commonPrefixLen(a: []const u8, b: []const u8) -> usize {
    const limit = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < limit) {
        if (a[i] != b[i]) {
            break;
        }
        i += 1;
    }
    return i;
}

export struct RadixTree<V> {
    struct Node {
        edge: []u8,
        value: V?,
        children: std.ArrayList(*Node),
    }

    alloc: std.mem.Allocator,
    root: *Node,
    count: usize,

    fn createNode(alloc: std.mem.Allocator, edge: []const u8) -> !*Node {
        const copied_edge = try alloc.dupe(u8, edge);
        errdefer alloc.free(copied_edge);
        const init_val = Node{
            .edge = copied_edge,
            .value = null,
            .children = .empty,
        };
        let node = try alloc.create(Node);
        node.* = init_val;
        return node;
    }

    fn destroySubtree(alloc: std.mem.Allocator, node: *Node) {
        for (child of node.children.items) {
            destroySubtree(alloc, child);
        }
        node.children.deinit(alloc);
        if (node.edge.len > 0) {
            alloc.free(node.edge);
        }
        alloc.destroy(node);
    }

    fn findChildIndex(node: *Node, first_byte: u8) -> usize? {
        let i: usize = 0;
        while (i < node.children.items.len) {
            const child = node.children.items[i];
            if (child.edge.len > 0 and child.edge[0] == first_byte) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    fn collectKeys(
        alloc: std.mem.Allocator,
        node: *Node,
        buf: *std.ArrayList(u8),
        out: *std.ArrayList(StrSlice),
    ) -> !void {
        if (node.value != null) {
            const copy = try alloc.dupe(u8, buf.items);
            errdefer alloc.free(copy);
            try out.append(alloc, copy);
        }
        for (child of node.children.items) {
            const saved_len = buf.items.len;
            try buf.appendSlice(alloc, child.edge);
            try collectKeys(alloc, child, buf, out);
            buf.items.len = saved_len;
        }
    }

    pub static fn init(alloc: std.mem.Allocator) -> !RadixTree<V> {
        const root = try createNode(alloc, "");
        return RadixTree<V>{
            .alloc = alloc,
            .root = root,
            .count = 0,
        };
    }

    pub fn deinit(*_) {
        destroySubtree(_.alloc, _.root);
    }

    pub fn len() -> usize {
        return _.count;
    }

    pub fn insert(*_, key: []const u8, val: V) -> !V? {
        let curr: *Node = _.root;
        let rem: []const u8 = key;

        while (true) {
            if (rem.len == 0) {
                const old = curr.value;
                curr.value = val;
                if (old == null) {
                    _.count += 1;
                }
                return old;
            }

            const maybe_idx = findChildIndex(curr, rem[0]);
            if (maybe_idx == null) {
                let new_child = try createNode(_.alloc, rem);
                new_child.value = val;
                errdefer destroySubtree(_.alloc, new_child);
                try curr.children.append(_.alloc, new_child);
                _.count += 1;
                return null;
            }

            const idx = maybe_idx.?;
            let child = curr.children.items[idx];
            const cpl = commonPrefixLen(child.edge, rem);

            if (cpl == child.edge.len) {
                curr = child;
                rem = rem[cpl..];
                continue;
            }

            const prefix = child.edge[0..cpl];
            const child_suffix = child.edge[cpl..];
            const key_suffix = rem[cpl..];

            const split_val: V? = if (key_suffix.len == 0) val else null;
            let split_node = try createNode(_.alloc, prefix);
            split_node.value = split_val;
            errdefer destroySubtree(_.alloc, split_node);
            try split_node.children.ensureTotalCapacity(_.alloc, 2);

            const new_child_edge = try _.alloc.dupe(u8, child_suffix);
            errdefer _.alloc.free(new_child_edge);

            if (key_suffix.len > 0) {
                let leaf = try createNode(_.alloc, key_suffix);
                leaf.value = val;
                if (child.edge.len > 0) {
                    _.alloc.free(child.edge);
                }
                child.edge = new_child_edge;
                split_node.children.appendAssumeCapacity(child);
                split_node.children.appendAssumeCapacity(leaf);
                curr.children.items[idx] = split_node;
                _.count += 1;
                return null;
            } else {
                if (child.edge.len > 0) {
                    _.alloc.free(child.edge);
                }
                child.edge = new_child_edge;
                split_node.children.appendAssumeCapacity(child);
                curr.children.items[idx] = split_node;
                _.count += 1;
                return null;
            }
        }
    }

    pub fn get(key: []const u8) -> V? {
        let curr: *Node = _.root;
        let rem: []const u8 = key;

        while (true) {
            if (rem.len == 0) {
                return curr.value;
            }
            const maybe_idx = findChildIndex(curr, rem[0]);
            if (maybe_idx == null) {
                return null;
            }
            const child = curr.children.items[maybe_idx.?];
            if (rem.len < child.edge.len) {
                return null;
            }
            if (!std.mem.eql(u8, rem[0..child.edge.len], child.edge)) {
                return null;
            }
            curr = child;
            rem = rem[child.edge.len..];
        }
    }

    pub fn longestPrefixMatch(query: []const u8) -> PrefixMatch<V>? {
        let curr: *Node = _.root;
        let matched_len: usize = 0;
        let best: PrefixMatch<V>? = null;

        if (v := curr.value) {
            best = PrefixMatch<V>{
                .key = query[0..0],
                .value = v,
            };
        }

        let rem: []const u8 = query;
        while (rem.len > 0) {
            const maybe_idx = findChildIndex(curr, rem[0]);
            if (maybe_idx == null) {
                break;
            }
            const child = curr.children.items[maybe_idx.?];
            if (rem.len < child.edge.len) {
                break;
            }
            if (!std.mem.eql(u8, rem[0..child.edge.len], child.edge)) {
                break;
            }
            matched_len += child.edge.len;
            curr = child;
            rem = rem[child.edge.len..];
            if (v := curr.value) {
                best = PrefixMatch<V>{
                    .key = query[0..matched_len],
                    .value = v,
                };
            }
        }

        return best;
    }

    fn removeAt(*_, node: *Node, rem: []const u8) -> V? {
        if (rem.len == 0) {
            const old = node.value;
            if (old == null) {
                return null;
            }
            node.value = null;
            _.count -= 1;
            return old;
        }

        const maybe_idx = findChildIndex(node, rem[0]);
        if (maybe_idx == null) {
            return null;
        }
        const idx = maybe_idx.?;
        const child = node.children.items[idx];
        if (rem.len < child.edge.len) {
            return null;
        }
        if (!std.mem.eql(u8, rem[0..child.edge.len], child.edge)) {
            return null;
        }

        const removed = _.removeAt(child, rem[child.edge.len..]);
        if (removed == null) {
            return null;
        }

        if (child.value == null and child.children.items.len == 0) {
            * = node.children.swapRemove(idx);
            child.children.deinit(_.alloc);
            if (child.edge.len > 0) {
                _.alloc.free(child.edge);
            }
            _.alloc.destroy(child);
        } else if (child.value == null and child.children.items.len == 1) {
            let grand = child.children.items[0];
            const total_len = child.edge.len + grand.edge.len;
            const merged = _.alloc.alloc(u8, total_len) catch null;
            if (m := merged) {
                std.mem.copyForwards(u8, m[0..child.edge.len], child.edge);
                std.mem.copyForwards(u8, m[child.edge.len..], grand.edge);
                if (child.edge.len > 0) {
                    _.alloc.free(child.edge);
                }
                if (grand.edge.len > 0) {
                    _.alloc.free(grand.edge);
                }
                grand.edge = m;
                node.children.items[idx] = grand;
                child.children.deinit(_.alloc);
                _.alloc.destroy(child);
            }
        }

        return removed;
    }

    pub fn remove(*_, key: []const u8) -> V? {
        return _.removeAt(_.root, key);
    }

    pub fn keysWithPrefix(alloc: std.mem.Allocator, prefix: []const u8) -> ![]StrSlice {
        let curr: *Node = _.root;
        let buf = std.ArrayList(u8).empty;
        errdefer buf.deinit(alloc);

        let rem: []const u8 = prefix;
        while (rem.len > 0) {
            const maybe_idx = findChildIndex(curr, rem[0]);
            if (maybe_idx == null) {
                buf.deinit(alloc);
                return try alloc.alloc(StrSlice, 0);
            }
            const child = curr.children.items[maybe_idx.?];
            const cpl = commonPrefixLen(child.edge, rem);
            if (cpl < rem.len and cpl < child.edge.len) {
                buf.deinit(alloc);
                return try alloc.alloc(StrSlice, 0);
            }
            try buf.appendSlice(alloc, child.edge);
            curr = child;
            if (rem.len <= child.edge.len) {
                rem = rem[rem.len..];
            } else {
                rem = rem[child.edge.len..];
            }
        }

        let out = std.ArrayList(StrSlice).empty;
        errdefer {
            for (s of out.items) {
                alloc.free(s);
            }
            out.deinit(alloc);
        }

        try collectKeys(alloc, curr, &buf, &out);
        buf.deinit(alloc);
        return try out.toOwnedSlice(alloc);
    }
}
