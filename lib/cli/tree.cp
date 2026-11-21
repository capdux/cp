// Source: Absorbed from /data/dev/bun/src/cli/ (Zero-Alloc CLI Tree Renderer)
// Pure-CP zero-allocation CLI tree & dependency-graph renderer supporting Unicode, ASCII, and Rounded styles.

/// Tree branch and connector drawing style.
pub enum TreeStyle {
    unicode,
    ascii,
    rounded,
}

/// Zero-allocation stack-resident bitmask tracking `is_last` status for up to 64 ancestor levels.
pub struct TreePrefix {
    depth: u8;
    last_mask: u64;

    /// Initialize an empty root-level prefix (depth 0).
    static fn init(): TreePrefix {
        return TreePrefix { depth: 0, last_mask: 0 };
    }

    /// Push one ancestor level onto the prefix stack.
    /// Returns `null` if nesting already reached the 64-level limit.
    fn push(is_last: bool): TreePrefix? {
        if (_.depth >= 64) return null;
        const shift: u6 = _.depth as u6;
        const bit: u64 = (1 as u64) << shift;
        const next_mask: u64 = if (is_last) (_.last_mask | bit) else (_.last_mask & (~bit));
        return TreePrefix { depth: _.depth + 1, last_mask: next_mask };
    }

    /// Pop the deepest ancestor level (no-op when already at depth 0).
    fn pop(): TreePrefix {
        if (_.depth == 0) return TreePrefix { depth: 0, last_mask: 0 };
        const new_depth: u8 = _.depth - 1;
        const shift: u6 = new_depth as u6;
        const bit: u64 = (1 as u64) << shift;
        return TreePrefix { depth: new_depth, last_mask: _.last_mask & (~bit) };
    }

    /// Check whether the ancestor at `level` (`0 <= level < depth`) was the last sibling.
    fn isAncestorLast(level: u8): bool {
        if (level >= _.depth) return false;
        const shift: u6 = level as u6;
        return ((_.last_mask >> shift) & 1) != 0;
    }

    /// Format ancestor indentation columns followed by the current node's branch connector.
    /// - `unicode`: `"│   "` / `"    "` + `"├── "` / `"└── "`
    /// - `ascii`:   `"|   "` / `"    "` + `"|-- "` / `"+-- "`
    /// - `rounded`: `"│   "` / `"    "` + `"├── "` / `"╰── "`
    /// Returns `null` if `out` is too small.
    fn formatPrefix(style: TreeStyle, is_last: bool, out: []u8): ([]const u8)? {
        const pipe_seg: []const u8 = if (style == TreeStyle.ascii) "|   " else "│   ";
        const blank_seg: []const u8 = "    ";

        let branch_mid: []const u8 = "├── ";
        let branch_last: []const u8 = "└── ";
        if (style == TreeStyle.ascii) {
            branch_mid = "|-- ";
            branch_last = "+-- ";
        } else if (style == TreeStyle.rounded) {
            branch_mid = "├── ";
            branch_last = "╰── ";
        }

        let w: usize = 0;
        let level: u8 = 0;
        while (level < _.depth) {
            const seg = if (_.isAncestorLast(level)) blank_seg else pipe_seg;
            w = pushBytes(out, w, seg) orelse return null;
            level += 1;
        }

        const branch = if (is_last) branch_last else branch_mid;
        w = pushBytes(out, w, branch) orelse return null;
        return out[0..w];
    }
}

/// Flat pre-order tree node descriptor (`depth == 0` for root, `depth >= 1` for nested children).
pub struct TreeNode {
    depth: u8;
    label: []const u8;
}

/// Copy `src` into `out` starting at `offset`, returning new offset or `null` on overflow.
fn pushBytes(out: []u8, offset: usize, src: []const u8): usize? {
    if (offset + src.len > out.len) return null;
    let i: usize = 0;
    while (i < src.len) {
        out[offset + i] = src[i];
        i += 1;
    }
    return offset + src.len;
}

/// Look ahead in a pre-order flat node list to determine whether `nodes[idx]`
/// is the last sibling at its depth under the same parent.
fn isLastSiblingAt(nodes: []const TreeNode, idx: usize): bool {
    const target_depth = nodes[idx].depth;
    let j: usize = idx + 1;
    while (j < nodes.len) {
        const d = nodes[j].depth;
        if (d < target_depth) return true;
        if (d == target_depth) return false;
        j += 1;
    }
    return true;
}

/// Render a flat pre-order list of `(depth, label)` nodes into a complete tree string in `out`.
/// Automatically detects last siblings at every depth with zero heap allocations.
/// Supports depths `0..=64` (up to 64 ancestor levels). Returns `null` if depth > 64 or `out` is too small.
pub fn renderFlatTree(nodes: []const TreeNode, style: TreeStyle, out: []u8): ([]const u8)? {
    if (nodes.len == 0) return out[0..0];

    // Track whether the active ancestor at each depth (1..=64) is the last sibling
    let last_at_depth: [65]bool = [false; 65];
    let w: usize = 0;
    let i: usize = 0;

    while (i < nodes.len) {
        if (i > 0) {
            w = pushBytes(out, w, "\n") orelse return null;
        }

        const node = nodes[i];
        if (node.depth > 64) return null;

        if (node.depth == 0) {
            w = pushBytes(out, w, node.label) orelse return null;
            // Reset ancestor tracking for a new root
            let k: usize = 0;
            while (k < 65) {
                last_at_depth[k] = false;
                k += 1;
            }
        } else {
            const cur_is_last = isLastSiblingAt(nodes, i);
            last_at_depth[node.depth as usize] = cur_is_last;

            let prefix = TreePrefix.init();
            let anc: u8 = 1;
            while (anc < node.depth) {
                prefix = prefix.push(last_at_depth[anc as usize]) orelse return null;
                anc += 1;
            }

            const pref_slice = prefix.formatPrefix(style, cur_is_last, out[w..]) orelse return null;
            w += pref_slice.len;
            w = pushBytes(out, w, node.label) orelse return null;
        }

        i += 1;
    }

    return out[0..w];
}
