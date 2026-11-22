// Source: native-cp-stdlib (Rope Balanced Text Buffer)

import std from "std";

const MAX_LEAF_LEN: usize = 64;

pub const RopeError = error {
    OutOfBounds,
    InvalidRange,
};

pub const NodePtr = *Node;

pub const Node = struct {
    is_leaf: bool,
    left: NodePtr?,
    right: NodePtr?,
    left_weight: usize,
    left_newlines: usize,
    total_len: usize,
    total_newlines: usize,
    height: usize,
    leaf_len: usize,
    leaf_buf: [64]u8,
};

fn countNewlines(bytes: []const u8) -> usize {
    let count: usize = 0;
    for (b of bytes) {
        if (b == '\n') {
            count = count + 1;
        }
    }
    return count;
}

fn updateLeafMeta(node: *Node) {
    node.is_leaf = true;
    node.left = null;
    node.right = null;
    node.left_weight = 0;
    node.left_newlines = 0;
    node.total_len = node.leaf_len;
    node.total_newlines = countNewlines(node.leaf_buf[0..node.leaf_len]);
    node.height = 1;
}

fn updateInternalMeta(node: *Node) {
    const l = node.left.?;
    const r = node.right.?;
    node.is_leaf = false;
    node.left_weight = l.total_len;
    node.left_newlines = l.total_newlines;
    node.total_len = l.total_len + r.total_len;
    node.total_newlines = l.total_newlines + r.total_newlines;
    const max_h = if (l.height > r.height) l.height else r.height;
    node.height = 1 + max_h;
}

fn nodeHeight(opt: NodePtr?) -> usize {
    if (n := opt) {
        return n.height;
    }
    return 0;
}

fn rotateRight(y: *Node) -> NodePtr {
    let x = y.left.?;
    const t2 = x.right;
    y.left = t2;
    x.right = y;
    updateInternalMeta(y);
    updateInternalMeta(x);
    return x;
}

fn rotateLeft(x: *Node) -> NodePtr {
    let y = x.right.?;
    const t2 = y.left;
    x.right = t2;
    y.left = x;
    updateInternalMeta(x);
    updateInternalMeta(y);
    return y;
}

fn rebalance(node: *Node) -> NodePtr {
    if (node.is_leaf) {
        return node;
    }
    const lh = nodeHeight(node.left);
    const rh = nodeHeight(node.right);
    if (lh > rh + 1) {
        let l = node.left.?;
        if (nodeHeight(l.right) > nodeHeight(l.left)) {
            node.left = rotateLeft(l);
        }
        return rotateRight(node);
    }
    if (rh > lh + 1) {
        let r = node.right.?;
        if (nodeHeight(r.left) > nodeHeight(r.right)) {
            node.right = rotateRight(r);
        }
        return rotateLeft(node);
    }
    return node;
}

fn createLeaf(alloc: std.mem.Allocator, data: []const u8) -> !NodePtr {
    let node = try alloc.create(Node);
    node.leaf_len = data.len;
    let i: usize = 0;
    while (i < data.len) {
        node.leaf_buf[i] = data[i];
        i = i + 1;
    }
    updateLeafMeta(node);
    return node;
}

fn createInternal(alloc: std.mem.Allocator, left: NodePtr, right: NodePtr) -> !NodePtr {
    let node = try alloc.create(Node);
    node.is_leaf = false;
    node.left = left;
    node.right = right;
    node.leaf_len = 0;
    updateInternalMeta(node);
    return node;
}

fn destroySubtree(alloc: std.mem.Allocator, node: *Node) {
    if (!node.is_leaf) {
        if (l := node.left) {
            destroySubtree(alloc, l);
        }
        if (r := node.right) {
            destroySubtree(alloc, r);
        }
    }
    alloc.destroy(node);
}

fn buildBalancedFromSlice(alloc: std.mem.Allocator, text: []const u8) -> !NodePtr {
    if (text.len <= MAX_LEAF_LEN) {
        return try createLeaf(alloc, text);
    }
    const mid = text.len / 2;
    const left = try buildBalancedFromSlice(alloc, text[0..mid]);
    errdefer destroySubtree(alloc, left);
    const right = try buildBalancedFromSlice(alloc, text[mid..text.len]);
    errdefer destroySubtree(alloc, right);
    return try createInternal(alloc, left, right);
}

fn joinTrees(alloc: std.mem.Allocator, left: *Node, right: *Node) -> !NodePtr {
    if (left.is_leaf && right.is_leaf && left.leaf_len + right.leaf_len <= MAX_LEAF_LEN) {
        let i: usize = 0;
        while (i < right.leaf_len) {
            left.leaf_buf[left.leaf_len + i] = right.leaf_buf[i];
            i = i + 1;
        }
        left.leaf_len = left.leaf_len + right.leaf_len;
        updateLeafMeta(left);
        alloc.destroy(right);
        return left;
    }
    if (left.height > right.height + 1 && !left.is_leaf) {
        left.right = try joinTrees(alloc, left.right.?, right);
        updateInternalMeta(left);
        return rebalance(left);
    }
    if (right.height > left.height + 1 && !right.is_leaf) {
        right.left = try joinTrees(alloc, left, right.left.?);
        updateInternalMeta(right);
        return rebalance(right);
    }
    let parent = try createInternal(alloc, left, right);
    return rebalance(parent);
}

fn insertNode(alloc: std.mem.Allocator, node: *Node, pos: usize, text: []const u8) -> !NodePtr {
    if (node.is_leaf) {
        if (node.leaf_len + text.len <= MAX_LEAF_LEN) {
            let tail = node.leaf_len;
            while (tail > pos) {
                const dst_idx: usize = tail - 1 + text.len;
                const src_idx: usize = tail - 1;
                node.leaf_buf[dst_idx] = node.leaf_buf[src_idx];
                tail = tail - 1;
            }
            let i: usize = 0;
            while (i < text.len) {
                node.leaf_buf[pos + i] = text[i];
                i = i + 1;
            }
            node.leaf_len = node.leaf_len + text.len;
            updateLeafMeta(node);
            return node;
        }
        if (pos == 0) {
            const new_left = try buildBalancedFromSlice(alloc, text);
            return try joinTrees(alloc, new_left, node);
        }
        if (pos == node.leaf_len) {
            const new_right = try buildBalancedFromSlice(alloc, text);
            return try joinTrees(alloc, node, new_right);
        }
        const left_leaf = try createLeaf(alloc, node.leaf_buf[0..pos]);
        errdefer destroySubtree(alloc, left_leaf);
        const mid_tree = try buildBalancedFromSlice(alloc, text);
        errdefer destroySubtree(alloc, mid_tree);
        const right_leaf = try createLeaf(alloc, node.leaf_buf[pos..node.leaf_len]);
        errdefer destroySubtree(alloc, right_leaf);
        alloc.destroy(node);
        let lm = try joinTrees(alloc, left_leaf, mid_tree);
        return try joinTrees(alloc, lm, right_leaf);
    }
    if (pos <= node.left_weight) {
        node.left = try insertNode(alloc, node.left.?, pos, text);
    } else {
        node.right = try insertNode(alloc, node.right.?, pos - node.left_weight, text);
    }
    updateInternalMeta(node);
    return rebalance(node);
}

fn deleteNode(alloc: std.mem.Allocator, node: *Node, start: usize, end: usize) -> NodePtr? {
    if (start == 0 && end == node.total_len) {
        destroySubtree(alloc, node);
        return null;
    }
    if (node.is_leaf) {
        const del_len = end - start;
        let i = end;
        while (i < node.leaf_len) {
            node.leaf_buf[i - del_len] = node.leaf_buf[i];
            i = i + 1;
        }
        node.leaf_len = node.leaf_len - del_len;
        updateLeafMeta(node);
        return node;
    }
    const lw = node.left_weight;
    if (end <= lw) {
        const new_left = deleteNode(alloc, node.left.?, start, end);
        if (new_left == null) {
            const surviving = node.right.?;
            alloc.destroy(node);
            return surviving;
        }
        node.left = new_left;
    } else if (start >= lw) {
        const new_right = deleteNode(alloc, node.right.?, start - lw, end - lw);
        if (new_right == null) {
            const surviving = node.left.?;
            alloc.destroy(node);
            return surviving;
        }
        node.right = new_right;
    } else {
        const new_left = deleteNode(alloc, node.left.?, start, lw);
        const new_right = deleteNode(alloc, node.right.?, 0, end - lw);
        if (new_left == null && new_right == null) {
            alloc.destroy(node);
            return null;
        }
        if (new_left == null) {
            alloc.destroy(node);
            return new_right;
        }
        if (new_right == null) {
            alloc.destroy(node);
            return new_left;
        }
        node.left = new_left;
        node.right = new_right;
    }
    updateInternalMeta(node);
    return rebalance(node);
}

fn charAtNode(node: NodePtr, idx: usize) -> u8 {
    let cur = node;
    let rem = idx;
    while (!cur.is_leaf) {
        if (rem < cur.left_weight) {
            cur = cur.left.?;
        } else {
            rem = rem - cur.left_weight;
            cur = cur.right.?;
        }
    }
    return cur.leaf_buf[rem];
}

fn collectSlice(node: NodePtr, start: usize, end: usize, out: *std.ArrayList(u8), alloc: std.mem.Allocator) -> !void {
    if (start >= end) {
        return;
    }
    if (node.is_leaf) {
        try out.appendSlice(alloc, node.leaf_buf[start..end]);
        return;
    }
    const lw = node.left_weight;
    if (start < lw) {
        const left_end = if (end < lw) end else lw;
        try collectSlice(node.left.?, start, left_end, out, alloc);
    }
    if (end > lw) {
        const right_start = if (start > lw) start - lw else 0;
        const right_end = end - lw;
        try collectSlice(node.right.?, right_start, right_end, out, alloc);
    }
}

fn findNthNewlineOffset(node: NodePtr, target_nl: usize) -> usize {
    let cur = node;
    let rem_nl = target_nl;
    let acc_bytes: usize = 0;
    while (!cur.is_leaf) {
        if (rem_nl <= cur.left_newlines) {
            cur = cur.left.?;
        } else {
            acc_bytes = acc_bytes + cur.left_weight;
            rem_nl = rem_nl - cur.left_newlines;
            cur = cur.right.?;
        }
    }
    let seen: usize = 0;
    let i: usize = 0;
    while (i < cur.leaf_len) {
        if (cur.leaf_buf[i] == '\n') {
            seen = seen + 1;
            if (seen == rem_nl) {
                return acc_bytes + i;
            }
        }
        i = i + 1;
    }
    return acc_bytes + cur.leaf_len;
}

pub struct Rope {
    alloc: std.mem.Allocator,
    root: NodePtr?,

    pub static fn init(alloc: std.mem.Allocator) -> Rope {
        return Rope {
            alloc: alloc,
            root: null,
        };
    }

    pub static fn fromSlice(alloc: std.mem.Allocator, text: []const u8) -> !Rope {
        if (text.len == 0) {
            return Rope.init(alloc);
        }
        const root = try buildBalancedFromSlice(alloc, text);
        return Rope {
            alloc: alloc,
            root: root,
        };
    }

    pub fn deinit(*_) {
        if (r := _.root) {
            destroySubtree(_.alloc, r);
            _.root = null;
        }
    }

    pub fn len() -> usize {
        if (r := _.root) {
            return r.total_len;
        }
        return 0;
    }

    pub fn lineCount() -> usize {
        if (r := _.root) {
            return r.total_newlines + 1;
        }
        return 1;
    }

    pub fn charAt(idx: usize) -> RopeError!u8 {
        if (r := _.root) {
            if (idx >= r.total_len) {
                return RopeError.OutOfBounds;
            }
            return charAtNode(r, idx);
        }
        return RopeError.OutOfBounds;
    }

    pub fn insert(*_, pos: usize, text: []const u8) -> !void {
        const cur_len = _.len();
        if (pos > cur_len) {
            return RopeError.OutOfBounds;
        }
        if (text.len == 0) {
            return;
        }
        if (r := _.root) {
            _.root = try insertNode(_.alloc, r, pos, text);
        } else {
            _.root = try buildBalancedFromSlice(_.alloc, text);
        }
    }

    pub fn delete(*_, start: usize, end: usize) -> RopeError!void {
        if (start > end) {
            return RopeError.InvalidRange;
        }
        const cur_len = _.len();
        if (end > cur_len) {
            return RopeError.OutOfBounds;
        }
        if (start == end) {
            return;
        }
        if (r := _.root) {
            _.root = deleteNode(_.alloc, r, start, end);
        }
    }

    pub fn sliceRange(alloc: std.mem.Allocator, start: usize, end: usize) -> ![]u8 {
        if (start > end) {
            return RopeError.InvalidRange;
        }
        const cur_len = _.len();
        if (end > cur_len) {
            return RopeError.OutOfBounds;
        }
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(alloc);
        if (start < end) {
            if (r := _.root) {
                try collectSlice(r, start, end, &out, alloc);
            }
        }
        return try out.toOwnedSlice(alloc);
    }

    pub fn lineAt(alloc: std.mem.Allocator, line_idx: usize) -> ![]u8 {
        const total_lines = _.lineCount();
        if (line_idx >= total_lines) {
            return RopeError.OutOfBounds;
        }
        if (r := _.root) {
            const start_byte: usize = if (line_idx == 0) 0 else findNthNewlineOffset(r, line_idx) + 1;
            const end_byte: usize = if (line_idx == r.total_newlines) r.total_len else findNthNewlineOffset(r, line_idx + 1);
            return try _.sliceRange(alloc, start_byte, end_byte);
        }
        return try alloc.alloc(u8, 0);
    }

    pub fn toString(alloc: std.mem.Allocator) -> ![]u8 {
        return try _.sliceRange(alloc, 0, _.len());
    }
}
