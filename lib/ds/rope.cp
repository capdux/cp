// Reference-hardened against cessen/ropey (MIT) & crossbeam-rs/crossbeam (MIT OR Apache-2.0)
// Source: native-cp-stdlib (Rope Balanced Text Buffer + 16-Lane SIMD Newline Engine)

import std from "std";

type RopeSkipVec16u8 = @Vector(16, u8);

const MAX_LEAF_LEN: usize = 64;

pub err RopeError {
    OutOfBounds,
    InvalidRange,
}

pub type NodePtr = *Node;

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

// ============================================================================
// 16-Lane @Vector(16, u8) SIMD Primitives + Scalar Differential References
// ============================================================================

fn ropeLoadVec16(data: []const u8, off: usize): RopeSkipVec16u8 {
    const v: RopeSkipVec16u8 = [
        data[off], data[off + 1], data[off + 2], data[off + 3],
        data[off + 4], data[off + 5], data[off + 6], data[off + 7],
        data[off + 8], data[off + 9], data[off + 10], data[off + 11],
        data[off + 12], data[off + 13], data[off + 14], data[off + 15],
    ];
    return v;
}

/// 16-lane `@Vector(16, u8)` SIMD newline (`\n`) counter (hot path for Rope leaf metadata maintenance).
pub fn simdRopeCountNewlines(bytes: []const u8): usize {
    const v_nl: RopeSkipVec16u8 = @splat(0x0A);
    const v_one: RopeSkipVec16u8 = @splat(1);
    const v_zero: RopeSkipVec16u8 = @splat(0);
    let count: usize = 0;
    let i: usize = 0;
    while (i + 16 <= bytes.len) {
        const v = ropeLoadVec16(bytes, i);
        const mask = @select(u8, v == v_nl, v_one, v_zero);
        count += @as(usize, @reduce(.Add, mask));
        i += 16;
    }
    while (i < bytes.len) : (i += 1) {
        if (bytes[i] == '\n') {
            count += 1;
        }
    }
    return count;
}

/// Scalar reference for `simdRopeCountNewlines` (differential verification).
pub fn scalarRopeCountNewlines(bytes: []const u8): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        if (bytes[i] == '\n') {
            count += 1;
        }
    }
    return count;
}

/// 16-lane `@Vector(16, u8)` SIMD scanner locating the first `\n` byte (hot path for line-indexed lookup).
pub fn simdRopeFindNewline(bytes: []const u8): usize? {
    const v_nl: RopeSkipVec16u8 = @splat(0x0A);
    let i: usize = 0;
    while (i + 16 <= bytes.len) {
        const v = ropeLoadVec16(bytes, i);
        if (@reduce(.Or, v == v_nl)) {
            let j: usize = 0;
            while (j < 16) : (j += 1) {
                if (bytes[i + j] == '\n') {
                    return i + j;
                }
            }
        }
        i += 16;
    }
    while (i < bytes.len) : (i += 1) {
        if (bytes[i] == '\n') {
            return i;
        }
    }
    return null;
}

/// Scalar reference for `simdRopeFindNewline` (differential verification).
pub fn scalarRopeFindNewline(bytes: []const u8): usize? {
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        if (bytes[i] == '\n') {
            return i;
        }
    }
    return null;
}

// ============================================================================
// AVL-Balanced Rope Tree Internals
// ============================================================================

fn ropeUpdateLeafMeta(node: *Node) {
    node.is_leaf = true;
    node.left = null;
    node.right = null;
    node.left_weight = 0;
    node.left_newlines = 0;
    node.total_len = node.leaf_len;
    node.total_newlines = simdRopeCountNewlines(node.leaf_buf[0..node.leaf_len]);
    node.height = 1;
}

fn ropeUpdateInternalMeta(node: *Node) {
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

fn ropeNodeHeight(opt: NodePtr?) -> usize {
    if (n := opt) {
        return n.height;
    }
    return 0;
}

fn ropeRotateRight(y: *Node) -> NodePtr {
    let x = y.left.?;
    const t2 = x.right;
    y.left = t2;
    x.right = y;
    ropeUpdateInternalMeta(y);
    ropeUpdateInternalMeta(x);
    return x;
}

fn ropeRotateLeft(x: *Node) -> NodePtr {
    let y = x.right.?;
    const t2 = y.left;
    x.right = t2;
    y.left = x;
    ropeUpdateInternalMeta(x);
    ropeUpdateInternalMeta(y);
    return y;
}

fn ropeRebalance(node: *Node) -> NodePtr {
    if (node.is_leaf) {
        return node;
    }
    const lh = ropeNodeHeight(node.left);
    const rh = ropeNodeHeight(node.right);
    if (lh > rh + 1) {
        let l = node.left.?;
        if (ropeNodeHeight(l.right) > ropeNodeHeight(l.left)) {
            node.left = ropeRotateLeft(l);
        }
        return ropeRotateRight(node);
    }
    if (rh > lh + 1) {
        let r = node.right.?;
        if (ropeNodeHeight(r.left) > ropeNodeHeight(r.right)) {
            node.right = ropeRotateRight(r);
        }
        return ropeRotateLeft(node);
    }
    return node;
}

fn ropeCreateLeaf(alloc: std.mem.Allocator, data: []const u8) -> !NodePtr {
    let node = try alloc.create(Node);
    node.leaf_len = data.len;
    let i: usize = 0;
    while (i < data.len) {
        node.leaf_buf[i] = data[i];
        i = i + 1;
    }
    ropeUpdateLeafMeta(node);
    return node;
}

fn ropeCreateInternal(alloc: std.mem.Allocator, left: NodePtr, right: NodePtr) -> !NodePtr {
    let node = try alloc.create(Node);
    node.is_leaf = false;
    node.left = left;
    node.right = right;
    node.leaf_len = 0;
    ropeUpdateInternalMeta(node);
    return node;
}

fn ropeDestroySubtree(alloc: std.mem.Allocator, node: *Node) {
    if (!node.is_leaf) {
        if (l := node.left) {
            ropeDestroySubtree(alloc, l);
        }
        if (r := node.right) {
            ropeDestroySubtree(alloc, r);
        }
    }
    alloc.destroy(node);
}

fn ropeBuildBalancedFromSlice(alloc: std.mem.Allocator, text: []const u8) -> !NodePtr {
    if (text.len <= MAX_LEAF_LEN) {
        return try ropeCreateLeaf(alloc, text);
    }
    const mid = text.len / 2;
    const left = try ropeBuildBalancedFromSlice(alloc, text[0..mid]);
    errdefer ropeDestroySubtree(alloc, left);
    const right = try ropeBuildBalancedFromSlice(alloc, text[mid..text.len]);
    errdefer ropeDestroySubtree(alloc, right);
    return try ropeCreateInternal(alloc, left, right);
}

fn ropeJoinTrees(alloc: std.mem.Allocator, left: *Node, right: *Node) -> !NodePtr {
    if (left.is_leaf && right.is_leaf && left.leaf_len + right.leaf_len <= MAX_LEAF_LEN) {
        let i: usize = 0;
        while (i < right.leaf_len) {
            left.leaf_buf[left.leaf_len + i] = right.leaf_buf[i];
            i = i + 1;
        }
        left.leaf_len = left.leaf_len + right.leaf_len;
        ropeUpdateLeafMeta(left);
        alloc.destroy(right);
        return left;
    }
    if (left.height > right.height + 1 && !left.is_leaf) {
        left.right = try ropeJoinTrees(alloc, left.right.?, right);
        ropeUpdateInternalMeta(left);
        return ropeRebalance(left);
    }
    if (right.height > left.height + 1 && !right.is_leaf) {
        right.left = try ropeJoinTrees(alloc, left, right.left.?);
        ropeUpdateInternalMeta(right);
        return ropeRebalance(right);
    }
    let parent = try ropeCreateInternal(alloc, left, right);
    return ropeRebalance(parent);
}

/// Zero-alloc AVL join reusing an existing internal `parent` node (used during deletion).
fn ropeJoinWithReusedParent(alloc: std.mem.Allocator, parent: *Node, left: *Node, right: *Node) -> NodePtr {
    if (left.is_leaf && right.is_leaf && left.leaf_len + right.leaf_len <= MAX_LEAF_LEN) {
        let i: usize = 0;
        while (i < right.leaf_len) {
            left.leaf_buf[left.leaf_len + i] = right.leaf_buf[i];
            i = i + 1;
        }
        left.leaf_len = left.leaf_len + right.leaf_len;
        ropeUpdateLeafMeta(left);
        alloc.destroy(right);
        alloc.destroy(parent);
        return left;
    }
    if (left.height > right.height + 1 && !left.is_leaf) {
        left.right = ropeJoinWithReusedParent(alloc, parent, left.right.?, right);
        ropeUpdateInternalMeta(left);
        return ropeRebalance(left);
    }
    if (right.height > left.height + 1 && !right.is_leaf) {
        right.left = ropeJoinWithReusedParent(alloc, parent, left, right.left.?);
        ropeUpdateInternalMeta(right);
        return ropeRebalance(right);
    }
    parent.is_leaf = false;
    parent.left = left;
    parent.right = right;
    parent.leaf_len = 0;
    ropeUpdateInternalMeta(parent);
    return ropeRebalance(parent);
}

fn ropeInsertNode(alloc: std.mem.Allocator, node: *Node, pos: usize, text: []const u8) -> !NodePtr {
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
            ropeUpdateLeafMeta(node);
            return node;
        }
        if (pos == 0) {
            const new_left = try ropeBuildBalancedFromSlice(alloc, text);
            return try ropeJoinTrees(alloc, new_left, node);
        }
        if (pos == node.leaf_len) {
            const new_right = try ropeBuildBalancedFromSlice(alloc, text);
            return try ropeJoinTrees(alloc, node, new_right);
        }
        const left_leaf = try ropeCreateLeaf(alloc, node.leaf_buf[0..pos]);
        errdefer ropeDestroySubtree(alloc, left_leaf);
        const mid_tree = try ropeBuildBalancedFromSlice(alloc, text);
        errdefer ropeDestroySubtree(alloc, mid_tree);
        const right_leaf = try ropeCreateLeaf(alloc, node.leaf_buf[pos..node.leaf_len]);
        errdefer ropeDestroySubtree(alloc, right_leaf);
        alloc.destroy(node);
        let lm = try ropeJoinTrees(alloc, left_leaf, mid_tree);
        return try ropeJoinTrees(alloc, lm, right_leaf);
    }
    if (pos <= node.left_weight) {
        node.left = try ropeInsertNode(alloc, node.left.?, pos, text);
    } else {
        node.right = try ropeInsertNode(alloc, node.right.?, pos - node.left_weight, text);
    }
    ropeUpdateInternalMeta(node);
    return ropeRebalance(node);
}

fn ropeDeleteNode(alloc: std.mem.Allocator, node: *Node, start: usize, end: usize) -> NodePtr? {
    if (start == 0 && end == node.total_len) {
        ropeDestroySubtree(alloc, node);
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
        ropeUpdateLeafMeta(node);
        return node;
    }
    const lw = node.left_weight;
    if (end <= lw) {
        const new_left = ropeDeleteNode(alloc, node.left.?, start, end);
        if (new_left == null) {
            const surviving = node.right.?;
            alloc.destroy(node);
            return surviving;
        }
        return ropeJoinWithReusedParent(alloc, node, new_left.?, node.right.?);
    } else if (start >= lw) {
        const new_right = ropeDeleteNode(alloc, node.right.?, start - lw, end - lw);
        if (new_right == null) {
            const surviving = node.left.?;
            alloc.destroy(node);
            return surviving;
        }
        return ropeJoinWithReusedParent(alloc, node, node.left.?, new_right.?);
    } else {
        const new_left = ropeDeleteNode(alloc, node.left.?, start, lw);
        const new_right = ropeDeleteNode(alloc, node.right.?, 0, end - lw);
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
        return ropeJoinWithReusedParent(alloc, node, new_left.?, new_right.?);
    }
}

struct RopeSplitPair {
    left: NodePtr?,
    right: NodePtr?,
}

fn ropeSplitNode(alloc: std.mem.Allocator, node: *Node, pos: usize) -> !RopeSplitPair {
    if (pos == 0) {
        return RopeSplitPair { left: null, right: node };
    }
    if (pos >= node.total_len) {
        return RopeSplitPair { left: node, right: null };
    }
    if (node.is_leaf) {
        const right_leaf = try ropeCreateLeaf(alloc, node.leaf_buf[pos..node.leaf_len]);
        node.leaf_len = pos;
        ropeUpdateLeafMeta(node);
        return RopeSplitPair { left: node, right: right_leaf };
    }
    const lw = node.left_weight;
    const orig_left = node.left.?;
    const orig_right = node.right.?;
    if (pos == lw) {
        alloc.destroy(node);
        return RopeSplitPair { left: orig_left, right: orig_right };
    }
    if (pos < lw) {
        const sp = try ropeSplitNode(alloc, orig_left, pos);
        if (sp_r := sp.right) {
            const merged_r = ropeJoinWithReusedParent(alloc, node, sp_r, orig_right);
            return RopeSplitPair { left: sp.left, right: merged_r };
        }
        alloc.destroy(node);
        return RopeSplitPair { left: sp.left, right: orig_right };
    } else {
        const sp = try ropeSplitNode(alloc, orig_right, pos - lw);
        if (sp_l := sp.left) {
            const merged_l = ropeJoinWithReusedParent(alloc, node, orig_left, sp_l);
            return RopeSplitPair { left: merged_l, right: sp.right };
        }
        alloc.destroy(node);
        return RopeSplitPair { left: orig_left, right: sp.right };
    }
}

fn ropeCharAtNode(node: NodePtr, idx: usize) -> u8 {
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

fn ropeCollectSlice(node: NodePtr, start: usize, end: usize, out: *std.ArrayList(u8), alloc: std.mem.Allocator) -> !void {
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
        try ropeCollectSlice(node.left.?, start, left_end, out, alloc);
    }
    if (end > lw) {
        const right_start = if (start > lw) start - lw else 0;
        const right_end = end - lw;
        try ropeCollectSlice(node.right.?, right_start, right_end, out, alloc);
    }
}

fn ropeFindNthNewlineOffset(node: NodePtr, target_nl: usize) -> usize {
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
    let pos: usize = 0;
    while (pos < cur.leaf_len) {
        if (hit := simdRopeFindNewline(cur.leaf_buf[pos..cur.leaf_len])) {
            seen += 1;
            if (seen == rem_nl) {
                return acc_bytes + pos + hit;
            }
            pos += hit + 1;
        } else {
            break;
        }
    }
    return acc_bytes + cur.leaf_len;
}

fn ropeVerifyNode(node: NodePtr) -> bool {
    if (node.is_leaf) {
        if (node.left != null || node.right != null) return false;
        if (node.leaf_len == 0 || node.leaf_len > MAX_LEAF_LEN) return false;
        if (node.total_len != node.leaf_len) return false;
        if (node.height != 1) return false;
        if (node.left_weight != 0 || node.left_newlines != 0) return false;
        if (node.total_newlines != scalarRopeCountNewlines(node.leaf_buf[0..node.leaf_len])) return false;
        return true;
    }
    const l = node.left orelse return false;
    const r = node.right orelse return false;
    if (!ropeVerifyNode(l) || !ropeVerifyNode(r)) return false;
    if (node.left_weight != l.total_len) return false;
    if (node.left_newlines != l.total_newlines) return false;
    if (node.total_len != l.total_len + r.total_len) return false;
    if (node.total_newlines != l.total_newlines + r.total_newlines) return false;
    const max_h = if (l.height > r.height) l.height else r.height;
    if (node.height != 1 + max_h) return false;
    if (l.height > r.height + 1 || r.height > l.height + 1) return false;
    return true;
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
        const root = try ropeBuildBalancedFromSlice(alloc, text);
        return Rope {
            alloc: alloc,
            root: root,
        };
    }

    pub fn deinit(*_) {
        if (r := _.root) {
            ropeDestroySubtree(_.alloc, r);
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
            return ropeCharAtNode(r, idx);
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
            _.root = try ropeInsertNode(_.alloc, r, pos, text);
        } else {
            _.root = try ropeBuildBalancedFromSlice(_.alloc, text);
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
            _.root = ropeDeleteNode(_.alloc, r, start, end);
        }
    }

    /// Concatenates `other` onto the end of this Rope, consuming `other` (leaving `other` empty).
    pub fn concat(*_, other: *Rope) -> !void {
        const rhs = other.root orelse return;
        other.root = null;
        if (lhs := _.root) {
            _.root = try ropeJoinTrees(_.alloc, lhs, rhs);
        } else {
            _.root = rhs;
        }
    }

    /// Splits this Rope at byte offset `pos`, keeping `[0..pos)` in `self` and returning `[pos..len)`.
    pub fn splitAt(*_, pos: usize) -> !Rope {
        const cur_len = _.len();
        if (pos > cur_len) {
            return RopeError.OutOfBounds;
        }
        if (pos == cur_len) {
            return Rope.init(_.alloc);
        }
        if (pos == 0) {
            const moved = _.root;
            _.root = null;
            return Rope { alloc: _.alloc, root: moved };
        }
        const r = _.root.?;
        const sp = try ropeSplitNode(_.alloc, r, pos);
        _.root = sp.left;
        return Rope { alloc: _.alloc, root: sp.right };
    }

    /// Verifies all AVL balance, byte-weight, and newline-count invariants across the entire tree.
    pub fn verifyInvariants() -> bool {
        if (r := _.root) {
            return ropeVerifyNode(r);
        }
        return true;
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
                try ropeCollectSlice(r, start, end, &out, alloc);
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
            const start_byte: usize = if (line_idx == 0) 0 else ropeFindNthNewlineOffset(r, line_idx) + 1;
            const end_byte: usize = if (line_idx == r.total_newlines) r.total_len else ropeFindNthNewlineOffset(r, line_idx + 1);
            return try _.sliceRange(alloc, start_byte, end_byte);
        }
        return try alloc.alloc(u8, 0);
    }

    pub fn toString(alloc: std.mem.Allocator) -> ![]u8 {
        return try _.sliceRange(alloc, 0, _.len());
    }
}
