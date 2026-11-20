// Source: cp/lib/bplus_tree/tree.cp (Variable-order B+ Tree engine with split/borrow/merge and invariant checker)

const std = @import("std");
import { BPlusError, KvEntry, compareKeys } from "./types.cp";

pub const NULL_NODE: u32 = 0xFFFFFFFF;
pub const MIN_ORDER: usize = 4;

struct Node {
    is_leaf: bool,
    keys: std.ArrayList([]u8),
    vals: std.ArrayList([]u8),
    children: std.ArrayList(u32),
    prev_leaf: u32,
    next_leaf: u32,

    fn init(is_leaf: bool): Node {
        return Node{
            .is_leaf = is_leaf,
            .keys = std.ArrayList([]u8).empty,
            .vals = std.ArrayList([]u8).empty,
            .children = std.ArrayList(u32).empty,
            .prev_leaf = NULL_NODE,
            .next_leaf = NULL_NODE,
        };
    }

    fn deinit(*_, alloc: std.mem.Allocator) {
        for (k of _.keys.items) {
            alloc.free(k);
        }
        for (v of _.vals.items) {
            alloc.free(v);
        }
        _.keys.deinit(alloc);
        _.vals.deinit(alloc);
        _.children.deinit(alloc);
    }
}

pub struct RangeIter {
    tree: *const BPlusTree,
    curr_leaf: u32,
    curr_idx: usize,
    end_key: ([]const u8)?,
    inclusive_end: bool,
    done: bool,

    pub fn next(*_): KvEntry? {
        if (_.done or _.curr_leaf == NULL_NODE) {
            _.done = true;
            return null;
        }
        const leaf = &_.tree.nodes.items[_.curr_leaf];
        if (_.curr_idx >= leaf.keys.items.len) {
            _.curr_leaf = leaf.next_leaf;
            _.curr_idx = 0;
            if (_.curr_leaf == NULL_NODE) {
                _.done = true;
                return null;
            }
            return _.next();
        }

        const k = leaf.keys.items[_.curr_idx];
        if (ek := _.end_key) {
            const ord = compareKeys(k, ek);
            if (_.inclusive_end) {
                if (ord == .gt) {
                    _.done = true;
                    return null;
                }
            } else {
                if (ord != .lt) {
                    _.done = true;
                    return null;
                }
            }
        }

        const v = leaf.vals.items[_.curr_idx];
        _.curr_idx += 1;
        return KvEntry{
            .key = k,
            .val = v,
        };
    }
}

pub struct BPlusTree {
    alloc: std.mem.Allocator,
    order: usize,
    root: u32,
    head_leaf: u32,
    tail_leaf: u32,
    count: usize,
    tree_height: usize,
    nodes: std.ArrayList(Node),
    free_nodes: std.ArrayList(u32),

    pub fn init(alloc: std.mem.Allocator, order: usize): BPlusError!BPlusTree {
        if (order < MIN_ORDER) {
            return BPlusError.InvalidOrder;
        }
        return BPlusTree{
            .alloc = alloc,
            .order = order,
            .root = NULL_NODE,
            .head_leaf = NULL_NODE,
            .tail_leaf = NULL_NODE,
            .count = 0,
            .tree_height = 0,
            .nodes = std.ArrayList(Node).empty,
            .free_nodes = std.ArrayList(u32).empty,
        };
    }

    pub fn deinit(*_) {
        let i: usize = 0;
        while (i < _.nodes.items.len) {
            _.nodes.items[i].deinit(_.alloc);
            i += 1;
        }
        _.nodes.deinit(_.alloc);
        _.free_nodes.deinit(_.alloc);
    }

    pub fn len(): usize {
        return _.count;
    }

    pub fn height(): usize {
        return _.tree_height;
    }

    fn maxKeys(): usize {
        return _.order - 1;
    }

    fn minLeafKeys(): usize {
        return _.order / 2;
    }

    fn minInternalKeys(): usize {
        return (_.order - 1) / 2;
    }

    fn allocNode(*_, is_leaf: bool): BPlusError!u32 {
        if (_.free_nodes.items.len > 0) {
            const idx = _.free_nodes.pop().?;
            _.nodes.items[idx] = Node.init(is_leaf);
            return idx;
        }
        const idx: u32 = @intCast(_.nodes.items.len);
        _.nodes.append(_.alloc, Node.init(is_leaf)) catch return BPlusError.OutOfMemory;
        return idx;
    }

    fn releaseNode(*_, idx: u32) {
        _.nodes.items[idx].deinit(_.alloc);
        _.nodes.items[idx] = Node.init(true);
        _.free_nodes.append(_.alloc, idx) catch {};
    }

    fn findLeaf(key: []const u8): u32 {
        if (_.root == NULL_NODE) {
            return NULL_NODE;
        }
        let curr = _.root;
        while (!_.nodes.items[curr].is_leaf) {
            const node = &_.nodes.items[curr];
            let child_idx: usize = 0;
            while (child_idx < node.keys.items.len) {
                if (compareKeys(key, node.keys.items[child_idx]) == .lt) {
                    break;
                }
                child_idx += 1;
            }
            curr = node.children.items[child_idx];
        }
        return curr;
    }

    fn lowerBoundInLeaf(node: *const Node, key: []const u8): usize {
        let lo: usize = 0;
        let hi: usize = node.keys.items.len;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            if (compareKeys(node.keys.items[mid], key) == .lt) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    }

    pub fn get(key: []const u8): ([]const u8)? {
        const leaf_idx = _.findLeaf(key);
        if (leaf_idx == NULL_NODE) {
            return null;
        }
        const leaf = &_.nodes.items[leaf_idx];
        const pos = lowerBoundInLeaf(leaf, key);
        if (pos < leaf.keys.items.len and compareKeys(leaf.keys.items[pos], key) == .eq) {
            return leaf.vals.items[pos];
        }
        return null;
    }

    pub fn first(): KvEntry? {
        if (_.head_leaf == NULL_NODE) {
            return null;
        }
        const leaf = &_.nodes.items[_.head_leaf];
        if (leaf.keys.items.len == 0) {
            return null;
        }
        return KvEntry{
            .key = leaf.keys.items[0],
            .val = leaf.vals.items[0],
        };
    }

    pub fn last(): KvEntry? {
        if (_.tail_leaf == NULL_NODE) {
            return null;
        }
        const leaf = &_.nodes.items[_.tail_leaf];
        if (leaf.keys.items.len == 0) {
            return null;
        }
        const last_i = leaf.keys.items.len - 1;
        return KvEntry{
            .key = leaf.keys.items[last_i],
            .val = leaf.vals.items[last_i],
        };
    }

    pub fn rangeIter(start_key: ([]const u8)?, end_key: ([]const u8)?, inclusive_end: bool): RangeIter {
        if (_.head_leaf == NULL_NODE) {
            return RangeIter{
                .tree = &_,
                .curr_leaf = NULL_NODE,
                .curr_idx = 0,
                .end_key = end_key,
                .inclusive_end = inclusive_end,
                .done = true,
            };
        }
        let leaf_idx = _.head_leaf;
        let idx: usize = 0;
        if (sk := start_key) {
            leaf_idx = _.findLeaf(sk);
            if (leaf_idx != NULL_NODE) {
                idx = lowerBoundInLeaf(&_.nodes.items[leaf_idx], sk);
            }
        }
        return RangeIter{
            .tree = &_,
            .curr_leaf = leaf_idx,
            .curr_idx = idx,
            .end_key = end_key,
            .inclusive_end = inclusive_end,
            .done = false,
        };
    }

    pub fn rangeScan(alloc: std.mem.Allocator, start_key: ([]const u8)?, end_key: ([]const u8)?): BPlusError![]KvEntry {
        return _.rangeScanEx(alloc, start_key, end_key, true);
    }

    pub fn rangeScanEx(alloc: std.mem.Allocator, start_key: ([]const u8)?, end_key: ([]const u8)?, inclusive_end: bool): BPlusError![]KvEntry {
        let out = std.ArrayList(KvEntry).empty;
        errdefer out.deinit(alloc);

        let iter = _.rangeIter(start_key, end_key, inclusive_end);
        while (entry := iter.next()) {
            out.append(alloc, entry) catch return BPlusError.OutOfMemory;
        }
        return out.toOwnedSlice(alloc) catch return BPlusError.OutOfMemory;
    }

    pub fn insert(*_, key: []const u8, val: []const u8): BPlusError!void {
        if (_.root == NULL_NODE) {
            const leaf_idx = try _.allocNode(true);
            const k_dup = _.alloc.dupe(u8, key) catch return BPlusError.OutOfMemory;
            errdefer _.alloc.free(k_dup);
            const v_dup = _.alloc.dupe(u8, val) catch return BPlusError.OutOfMemory;
            errdefer _.alloc.free(v_dup);

            _.nodes.items[leaf_idx].keys.append(_.alloc, k_dup) catch return BPlusError.OutOfMemory;
            _.nodes.items[leaf_idx].vals.append(_.alloc, v_dup) catch return BPlusError.OutOfMemory;
            _.root = leaf_idx;
            _.head_leaf = leaf_idx;
            _.tail_leaf = leaf_idx;
            _.count = 1;
            _.tree_height = 1;
            return;
        }

        let path = std.ArrayList(u32).empty;
        defer path.deinit(_.alloc);
        let child_indices = std.ArrayList(usize).empty;
        defer child_indices.deinit(_.alloc);

        let curr = _.root;
        while (!_.nodes.items[curr].is_leaf) {
            const node = &_.nodes.items[curr];
            let ci: usize = 0;
            while (ci < node.keys.items.len) {
                if (compareKeys(key, node.keys.items[ci]) == .lt) {
                    break;
                }
                ci += 1;
            }
            path.append(_.alloc, curr) catch return BPlusError.OutOfMemory;
            child_indices.append(_.alloc, ci) catch return BPlusError.OutOfMemory;
            curr = node.children.items[ci];
        }

        const pos = lowerBoundInLeaf(&_.nodes.items[curr], key);
        if (pos < _.nodes.items[curr].keys.items.len and compareKeys(_.nodes.items[curr].keys.items[pos], key) == .eq) {
            const v_dup = _.alloc.dupe(u8, val) catch return BPlusError.OutOfMemory;
            _.alloc.free(_.nodes.items[curr].vals.items[pos]);
            _.nodes.items[curr].vals.items[pos] = v_dup;
            return;
        }

        const k_dup = _.alloc.dupe(u8, key) catch return BPlusError.OutOfMemory;
        errdefer _.alloc.free(k_dup);
        const v_dup = _.alloc.dupe(u8, val) catch return BPlusError.OutOfMemory;
        errdefer _.alloc.free(v_dup);

        _.nodes.items[curr].keys.insert(_.alloc, pos, k_dup) catch return BPlusError.OutOfMemory;
        _.nodes.items[curr].vals.insert(_.alloc, pos, v_dup) catch return BPlusError.OutOfMemory;
        _.count += 1;

        if (_.nodes.items[curr].keys.items.len <= _.maxKeys()) {
            return;
        }

        const right_leaf = try _.allocNode(true);
        const split_idx = _.order / 2;
        const old_next = _.nodes.items[curr].next_leaf;

        {
            let left = &_.nodes.items[curr];
            let right = &_.nodes.items[right_leaf];
            let i: usize = split_idx;
            while (i < left.keys.items.len) {
                right.keys.append(_.alloc, left.keys.items[i]) catch return BPlusError.OutOfMemory;
                right.vals.append(_.alloc, left.vals.items[i]) catch return BPlusError.OutOfMemory;
                i += 1;
            }
            left.keys.shrinkRetainingCapacity(split_idx);
            left.vals.shrinkRetainingCapacity(split_idx);

            right.prev_leaf = curr;
            right.next_leaf = old_next;
            left.next_leaf = right_leaf;
        }
        if (old_next != NULL_NODE) {
            _.nodes.items[old_next].prev_leaf = right_leaf;
        } else {
            _.tail_leaf = right_leaf;
        }

        let promoted_key = _.alloc.dupe(u8, _.nodes.items[right_leaf].keys.items[0]) catch return BPlusError.OutOfMemory;
        let right_child = right_leaf;

        while (path.items.len > 0) {
            const parent_idx = path.pop().?;
            const ci = child_indices.pop().?;

            {
                let parent = &_.nodes.items[parent_idx];
                parent.keys.insert(_.alloc, ci, promoted_key) catch return BPlusError.OutOfMemory;
                parent.children.insert(_.alloc, ci + 1, right_child) catch return BPlusError.OutOfMemory;
                if (parent.keys.items.len <= _.maxKeys()) {
                    return;
                }
            }

            const new_right = try _.allocNode(false);
            const mid = _.order / 2;
            let parent = &_.nodes.items[parent_idx];
            const next_promoted = parent.keys.items[mid];

            {
                let rnode = &_.nodes.items[new_right];
                let ki: usize = mid + 1;
                while (ki < parent.keys.items.len) {
                    rnode.keys.append(_.alloc, parent.keys.items[ki]) catch return BPlusError.OutOfMemory;
                    ki += 1;
                }
                let chi: usize = mid + 1;
                while (chi < parent.children.items.len) {
                    rnode.children.append(_.alloc, parent.children.items[chi]) catch return BPlusError.OutOfMemory;
                    chi += 1;
                }
                parent.keys.shrinkRetainingCapacity(mid);
                parent.children.shrinkRetainingCapacity(mid + 1);
            }

            promoted_key = next_promoted;
            right_child = new_right;
        }

        const new_root = try _.allocNode(false);
        _.nodes.items[new_root].keys.append(_.alloc, promoted_key) catch return BPlusError.OutOfMemory;
        _.nodes.items[new_root].children.append(_.alloc, _.root) catch return BPlusError.OutOfMemory;
        _.nodes.items[new_root].children.append(_.alloc, right_child) catch return BPlusError.OutOfMemory;
        _.root = new_root;
        _.tree_height += 1;
    }

    pub fn remove(*_, key: []const u8): bool {
        if (_.root == NULL_NODE) {
            return false;
        }

        let path = std.ArrayList(u32).empty;
        defer path.deinit(_.alloc);
        let child_indices = std.ArrayList(usize).empty;
        defer child_indices.deinit(_.alloc);

        let curr = _.root;
        while (!_.nodes.items[curr].is_leaf) {
            const node = &_.nodes.items[curr];
            let ci: usize = 0;
            while (ci < node.keys.items.len) {
                if (compareKeys(key, node.keys.items[ci]) == .lt) {
                    break;
                }
                ci += 1;
            }
            path.append(_.alloc, curr) catch return false;
            child_indices.append(_.alloc, ci) catch return false;
            curr = node.children.items[ci];
        }

        const pos = lowerBoundInLeaf(&_.nodes.items[curr], key);
        if (pos >= _.nodes.items[curr].keys.items.len or compareKeys(_.nodes.items[curr].keys.items[pos], key) != .eq) {
            return false;
        }

        const removed_k = _.nodes.items[curr].keys.orderedRemove(pos);
        const removed_v = _.nodes.items[curr].vals.orderedRemove(pos);
        _.alloc.free(removed_k);
        _.alloc.free(removed_v);
        _.count -= 1;

        if (curr == _.root) {
            if (_.nodes.items[curr].keys.items.len == 0) {
                _.releaseNode(curr);
                _.root = NULL_NODE;
                _.head_leaf = NULL_NODE;
                _.tail_leaf = NULL_NODE;
                _.tree_height = 0;
            }
            return true;
        }

        if (_.nodes.items[curr].keys.items.len >= _.minLeafKeys()) {
            return true;
        }

        _.rebalanceLeaf(curr, &path, &child_indices);
        return true;
    }

    fn rebalanceLeaf(*_, leaf_idx: u32, path: *std.ArrayList(u32), child_indices: *std.ArrayList(usize)) {
        const parent_idx = path.items[path.items.len - 1];
        const ci = child_indices.items[child_indices.items.len - 1];
        const min_k = _.minLeafKeys();

        if (ci > 0) {
            const left_idx = _.nodes.items[parent_idx].children.items[ci - 1];
            if (_.nodes.items[left_idx].keys.items.len > min_k) {
                const k = _.nodes.items[left_idx].keys.pop().?;
                const v = _.nodes.items[left_idx].vals.pop().?;
                _.nodes.items[leaf_idx].keys.insert(_.alloc, 0, k) catch return;
                _.nodes.items[leaf_idx].vals.insert(_.alloc, 0, v) catch return;

                const new_sep = _.alloc.dupe(u8, k) catch return;
                _.alloc.free(_.nodes.items[parent_idx].keys.items[ci - 1]);
                _.nodes.items[parent_idx].keys.items[ci - 1] = new_sep;
                return;
            }
        }

        if (ci + 1 < _.nodes.items[parent_idx].children.items.len) {
            const right_idx = _.nodes.items[parent_idx].children.items[ci + 1];
            if (_.nodes.items[right_idx].keys.items.len > min_k) {
                const k = _.nodes.items[right_idx].keys.orderedRemove(0);
                const v = _.nodes.items[right_idx].vals.orderedRemove(0);
                _.nodes.items[leaf_idx].keys.append(_.alloc, k) catch return;
                _.nodes.items[leaf_idx].vals.append(_.alloc, v) catch return;

                const new_sep = _.alloc.dupe(u8, _.nodes.items[right_idx].keys.items[0]) catch return;
                _.alloc.free(_.nodes.items[parent_idx].keys.items[ci]);
                _.nodes.items[parent_idx].keys.items[ci] = new_sep;
                return;
            }
        }

        if (ci > 0) {
            const left_idx = _.nodes.items[parent_idx].children.items[ci - 1];
            _.mergeLeaves(left_idx, leaf_idx, parent_idx, ci - 1);
        } else {
            const right_idx = _.nodes.items[parent_idx].children.items[ci + 1];
            _.mergeLeaves(leaf_idx, right_idx, parent_idx, ci);
        }

        * = path.pop();
        * = child_indices.pop();
        _.rebalanceInternal(parent_idx, path, child_indices);
    }

    fn mergeLeaves(*_, left_idx: u32, right_idx: u32, parent_idx: u32, sep_key_idx: usize) {
        {
            let left = &_.nodes.items[left_idx];
            let right = &_.nodes.items[right_idx];
            let i: usize = 0;
            while (i < right.keys.items.len) {
                left.keys.append(_.alloc, right.keys.items[i]) catch return;
                left.vals.append(_.alloc, right.vals.items[i]) catch return;
                i += 1;
            }
            right.keys.clearRetainingCapacity();
            right.vals.clearRetainingCapacity();

            const after_right = right.next_leaf;
            left.next_leaf = after_right;
            if (after_right != NULL_NODE) {
                _.nodes.items[after_right].prev_leaf = left_idx;
            } else {
                _.tail_leaf = left_idx;
            }
        }

        _.releaseNode(right_idx);
        const sep = _.nodes.items[parent_idx].keys.orderedRemove(sep_key_idx);
        _.alloc.free(sep);
        * = _.nodes.items[parent_idx].children.orderedRemove(sep_key_idx + 1);
    }

    fn rebalanceInternal(*_, node_idx: u32, path: *std.ArrayList(u32), child_indices: *std.ArrayList(usize)) {
        if (node_idx == _.root) {
            if (_.nodes.items[node_idx].keys.items.len == 0) {
                const only_child = _.nodes.items[node_idx].children.items[0];
                _.nodes.items[node_idx].children.clearRetainingCapacity();
                _.releaseNode(node_idx);
                _.root = only_child;
                _.tree_height -= 1;
            }
            return;
        }

        const min_k = _.minInternalKeys();
        if (_.nodes.items[node_idx].keys.items.len >= min_k) {
            return;
        }

        const parent_idx = path.items[path.items.len - 1];
        const ci = child_indices.items[child_indices.items.len - 1];

        if (ci > 0) {
            const left_idx = _.nodes.items[parent_idx].children.items[ci - 1];
            if (_.nodes.items[left_idx].keys.items.len > min_k) {
                const sep = _.nodes.items[parent_idx].keys.items[ci - 1];
                const borrowed_k = _.nodes.items[left_idx].keys.pop().?;
                const borrowed_c = _.nodes.items[left_idx].children.pop().?;

                _.nodes.items[node_idx].keys.insert(_.alloc, 0, sep) catch return;
                _.nodes.items[node_idx].children.insert(_.alloc, 0, borrowed_c) catch return;
                _.nodes.items[parent_idx].keys.items[ci - 1] = borrowed_k;
                return;
            }
        }

        if (ci + 1 < _.nodes.items[parent_idx].children.items.len) {
            const right_idx = _.nodes.items[parent_idx].children.items[ci + 1];
            if (_.nodes.items[right_idx].keys.items.len > min_k) {
                const sep = _.nodes.items[parent_idx].keys.items[ci];
                const borrowed_k = _.nodes.items[right_idx].keys.orderedRemove(0);
                const borrowed_c = _.nodes.items[right_idx].children.orderedRemove(0);

                _.nodes.items[node_idx].keys.append(_.alloc, sep) catch return;
                _.nodes.items[node_idx].children.append(_.alloc, borrowed_c) catch return;
                _.nodes.items[parent_idx].keys.items[ci] = borrowed_k;
                return;
            }
        }

        if (ci > 0) {
            const left_idx = _.nodes.items[parent_idx].children.items[ci - 1];
            _.mergeInternal(left_idx, node_idx, parent_idx, ci - 1);
        } else {
            const right_idx = _.nodes.items[parent_idx].children.items[ci + 1];
            _.mergeInternal(node_idx, right_idx, parent_idx, ci);
        }

        * = path.pop();
        * = child_indices.pop();
        _.rebalanceInternal(parent_idx, path, child_indices);
    }

    fn mergeInternal(*_, left_idx: u32, right_idx: u32, parent_idx: u32, sep_key_idx: usize) {
        const sep = _.nodes.items[parent_idx].keys.orderedRemove(sep_key_idx);
        * = _.nodes.items[parent_idx].children.orderedRemove(sep_key_idx + 1);

        {
            let left = &_.nodes.items[left_idx];
            let right = &_.nodes.items[right_idx];
            left.keys.append(_.alloc, sep) catch return;
            for (k of right.keys.items) {
                left.keys.append(_.alloc, k) catch return;
            }
            for (c of right.children.items) {
                left.children.append(_.alloc, c) catch return;
            }
            right.keys.clearRetainingCapacity();
            right.children.clearRetainingCapacity();
        }

        _.releaseNode(right_idx);
    }

    fn validateSubtree(
        node_idx: u32,
        depth: usize,
        leaf_depth: *usize,
        min_bound: ([]const u8)?,
        max_bound: ([]const u8)?,
        leaf_order: *std.ArrayList(u32),
        total_entries: *usize,
    ): BPlusError!void {
        if (node_idx >= _.nodes.items.len) {
            return BPlusError.CorruptedTree;
        }
        const node = &_.nodes.items[node_idx];

        let i: usize = 1;
        while (i < node.keys.items.len) {
            if (compareKeys(node.keys.items[i - 1], node.keys.items[i]) != .lt) {
                return BPlusError.CorruptedTree;
            }
            i += 1;
        }

        if (node.keys.items.len > _.maxKeys()) {
            return BPlusError.CorruptedTree;
        }

        if (mb := min_bound) {
            if (node.keys.items.len > 0 and compareKeys(node.keys.items[0], mb) == .lt) {
                return BPlusError.CorruptedTree;
            }
        }
        if (xb := max_bound) {
            if (node.keys.items.len > 0 and compareKeys(node.keys.items[node.keys.items.len - 1], xb) != .lt) {
                return BPlusError.CorruptedTree;
            }
        }

        if (node.is_leaf) {
            if (node.vals.items.len != node.keys.items.len) {
                return BPlusError.CorruptedTree;
            }
            if (node_idx != _.root and node.keys.items.len < _.minLeafKeys()) {
                return BPlusError.CorruptedTree;
            }
            if (leaf_depth.* == 0) {
                leaf_depth.* = depth;
            } else if (leaf_depth.* != depth) {
                return BPlusError.CorruptedTree;
            }
            leaf_order.append(_.alloc, node_idx) catch return BPlusError.OutOfMemory;
            total_entries.* += node.keys.items.len;
            return;
        }

        if (node_idx == _.root) {
            if (node.keys.items.len == 0) {
                return BPlusError.CorruptedTree;
            }
        } else {
            if (node.keys.items.len < _.minInternalKeys()) {
                return BPlusError.CorruptedTree;
            }
        }

        if (node.children.items.len != node.keys.items.len + 1) {
            return BPlusError.CorruptedTree;
        }

        let ci: usize = 0;
        while (ci < node.children.items.len) {
            const child_min: ([]const u8)? = if (ci == 0) min_bound else node.keys.items[ci - 1];
            const child_max: ([]const u8)? = if (ci < node.keys.items.len) node.keys.items[ci] else max_bound;
            try _.validateSubtree(node.children.items[ci], depth + 1, leaf_depth, child_min, child_max, leaf_order, total_entries);
            ci += 1;
        }
    }

    pub fn validateInvariants(): BPlusError!void {
        if (_.count == 0) {
            if (_.root != NULL_NODE or _.head_leaf != NULL_NODE or _.tail_leaf != NULL_NODE or _.tree_height != 0) {
                return BPlusError.CorruptedTree;
            }
            return;
        }
        if (_.root == NULL_NODE or _.head_leaf == NULL_NODE or _.tail_leaf == NULL_NODE) {
            return BPlusError.CorruptedTree;
        }

        let leaf_depth: usize = 0;
        let total_entries: usize = 0;
        let leaf_order = std.ArrayList(u32).empty;
        defer leaf_order.deinit(_.alloc);

        try _.validateSubtree(_.root, 1, &leaf_depth, null, null, &leaf_order, &total_entries);

        if (total_entries != _.count or leaf_depth != _.tree_height) {
            return BPlusError.CorruptedTree;
        }
        if (leaf_order.items.len == 0 or leaf_order.items[0] != _.head_leaf or leaf_order.items[leaf_order.items.len - 1] != _.tail_leaf) {
            return BPlusError.CorruptedTree;
        }

        let i: usize = 0;
        while (i < leaf_order.items.len) {
            const lid = leaf_order.items[i];
            const leaf = &_.nodes.items[lid];
            const expected_prev = if (i == 0) NULL_NODE else leaf_order.items[i - 1];
            const expected_next = if (i + 1 == leaf_order.items.len) NULL_NODE else leaf_order.items[i + 1];
            if (leaf.prev_leaf != expected_prev or leaf.next_leaf != expected_next) {
                return BPlusError.CorruptedTree;
            }
            i += 1;
        }
    }
}
