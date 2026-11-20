// Source: cp/lib/bplus/bplus.cp (Pure CP B+ Tree Ordered KV & Range Scan Engine)
// Zero-C-dependency B+ Tree with doubly-linked leaf chain, multi-level splitting,
// borrow/merge rebalancing, root height contraction, rangeScan, prefixScan, and lowerBound.

const std = @import("std");

/// Error set for B+ Tree operations.
pub err BPlusError {
    InvalidOrder,
    InvalidRange,
    OutOfMemory,
}

/// Read-only view of a key-value entry stored in the B+ Tree.
pub struct BPlusEntry {
    key: string = "";
    value: string = "";
}

const NULL_IDX: usize = std.math.maxInt(usize);
const MAX_TREE_DEPTH: usize = 64;

struct SearchResult {
    found: bool = false;
    idx: usize = 0;
}

struct BPlusNode {
    is_leaf: bool = true;
    keys: std.ArrayList([]u8) = .empty;
    values: std.ArrayList([]u8) = .empty;
    children: std.ArrayList(usize) = .empty;
    next_leaf: usize = NULL_IDX;
    prev_leaf: usize = NULL_IDX;
}

fn lowerBoundKey(keys: []const []u8, target: string): SearchResult {
    let lo: usize = 0;
    let hi: usize = keys.len;
    while (lo < hi) {
        const mid = lo + (hi - lo) / 2;
        const ord = std.mem.order(u8, keys[mid], target);
        if (ord == .lt) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    if (lo < keys.len && std.mem.eql(u8, keys[lo], target)) {
        return SearchResult{ found: true, idx: lo };
    }
    return SearchResult{ found: false, idx: lo };
}

fn upperBoundKey(keys: []const []u8, target: string): usize {
    let lo: usize = 0;
    let hi: usize = keys.len;
    while (lo < hi) {
        const mid = lo + (hi - lo) / 2;
        const ord = std.mem.order(u8, keys[mid], target);
        if (ord != .gt) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

/// In-memory B+ Tree ordered lexicographically by `key: string`.
/// Uses an index-pool of nodes and a doubly-linked leaf chain for fast range/prefix scans.
pub struct BPlusTree {
    alloc: std.mem.Allocator;
    order: usize;
    nodes: std.ArrayList(BPlusNode) = .empty;
    free_slots: std.ArrayList(usize) = .empty;
    root: usize = NULL_IDX;
    head_leaf: usize = NULL_IDX;
    tail_leaf: usize = NULL_IDX;
    length: usize = 0;
    tree_height: usize = 0;

    /// Initialize an empty B+ Tree with branching factor `order >= 3`.
    pub static fn init(alloc: std.mem.Allocator, order: usize): BPlusTree ! BPlusError {
        if (order < 3) {
            return BPlusError.InvalidOrder;
        }
        return BPlusTree{
            alloc: alloc,
            order: order,
            nodes: .empty,
            free_slots: .empty,
            root: NULL_IDX,
            head_leaf: NULL_IDX,
            tail_leaf: NULL_IDX,
            length: 0,
            tree_height: 0,
        };
    }

    /// Release all nodes, owned keys, owned values, and internal pool storage.
    pub fn deinit(*_) {
        for (*node of _.nodes.items) {
            for (k of node.keys.items) {
                _.alloc.free(k);
            }
            node.keys.deinit(_.alloc);
            for (v of node.values.items) {
                _.alloc.free(v);
            }
            node.values.deinit(_.alloc);
            node.children.deinit(_.alloc);
        }
        _.nodes.deinit(_.alloc);
        _.free_slots.deinit(_.alloc);
        _.root = NULL_IDX;
        _.head_leaf = NULL_IDX;
        _.tail_leaf = NULL_IDX;
        _.length = 0;
        _.tree_height = 0;
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

    fn allocNode(*_, is_leaf: bool): usize ! BPlusError {
        const node = BPlusNode{ is_leaf: is_leaf };
        if (_.free_slots.items.len > 0) {
            const idx = _.free_slots.pop().?;
            _.nodes.items[idx] = node;
            return idx;
        }
        const idx = _.nodes.items.len;
        _.nodes.append(_.alloc, node) catch return BPlusError.OutOfMemory;
        return idx;
    }

    fn freeNode(*_, idx: usize) {
        const node = &_.nodes.items[idx];
        for (k of node.keys.items) {
            _.alloc.free(k);
        }
        node.keys.deinit(_.alloc);
        for (v of node.values.items) {
            _.alloc.free(v);
        }
        node.values.deinit(_.alloc);
        node.children.deinit(_.alloc);
        _.nodes.items[idx] = BPlusNode{};
        _.free_slots.append(_.alloc, idx) catch {};
    }

    fn findLeaf(key: string): usize {
        if (_.root == NULL_IDX) return NULL_IDX;
        let cur = _.root;
        while (!_.nodes.items[cur].is_leaf) {
            const pos = upperBoundKey(_.nodes.items[cur].keys.items, key);
            cur = _.nodes.items[cur].children.items[pos];
        }
        return cur;
    }

    /// Return the number of key-value pairs in the tree.
    pub fn len(): usize {
        return _.length;
    }

    /// Return the current height (number of levels) of the tree.
    pub fn height(): usize {
        return _.tree_height;
    }

    /// Look up the value associated with `key`, or `null` if absent.
    pub fn get(key: string): ([]const u8)? {
        const leaf_idx = _.findLeaf(key);
        if (leaf_idx == NULL_IDX) return null;
        const leaf = _.nodes.items[leaf_idx];
        const res = lowerBoundKey(leaf.keys.items, key);
        if (res.found) {
            return leaf.values.items[res.idx];
        }
        return null;
    }

    /// Return the first entry whose key is >= `key`, or `null` if none exists.
    pub fn lowerBound(key: string): BPlusEntry? {
        const leaf_idx = _.findLeaf(key);
        if (leaf_idx == NULL_IDX) return null;
        const leaf = _.nodes.items[leaf_idx];
        const res = lowerBoundKey(leaf.keys.items, key);
        if (res.idx < leaf.keys.items.len) {
            return BPlusEntry{
                key: leaf.keys.items[res.idx],
                value: leaf.values.items[res.idx],
            };
        }
        if (leaf.next_leaf != NULL_IDX) {
            const next = _.nodes.items[leaf.next_leaf];
            if (next.keys.items.len > 0) {
                return BPlusEntry{
                    key: next.keys.items[0],
                    value: next.values.items[0],
                };
            }
        }
        return null;
    }

    /// Return the minimum entry in the tree, or `null` if empty.
    pub fn minEntry(): BPlusEntry? {
        if (_.head_leaf == NULL_IDX) return null;
        const leaf = _.nodes.items[_.head_leaf];
        if (leaf.keys.items.len == 0) return null;
        return BPlusEntry{
            key: leaf.keys.items[0],
            value: leaf.values.items[0],
        };
    }

    /// Return the maximum entry in the tree, or `null` if empty.
    pub fn maxEntry(): BPlusEntry? {
        if (_.tail_leaf == NULL_IDX) return null;
        const leaf = _.nodes.items[_.tail_leaf];
        if (leaf.keys.items.len == 0) return null;
        const last = leaf.keys.items.len - 1;
        return BPlusEntry{
            key: leaf.keys.items[last],
            value: leaf.values.items[last],
        };
    }

    /// Insert or update `(key, value)`.
    /// Returns `true` if a new key was inserted, or `false` if an existing key was overwritten.
    pub fn put(*_, key: string, value: string): bool ! BPlusError {
        if (_.root == NULL_IDX) {
            const owned_k = _.alloc.dupe(u8, key) catch return BPlusError.OutOfMemory;
            errdefer _.alloc.free(owned_k);
            const owned_v = _.alloc.dupe(u8, value) catch return BPlusError.OutOfMemory;
            errdefer _.alloc.free(owned_v);

            const leaf_idx = _.allocNode(true)?;
            _.nodes.items[leaf_idx].keys.append(_.alloc, owned_k) catch return BPlusError.OutOfMemory;
            _.nodes.items[leaf_idx].values.append(_.alloc, owned_v) catch return BPlusError.OutOfMemory;
            _.root = leaf_idx;
            _.head_leaf = leaf_idx;
            _.tail_leaf = leaf_idx;
            _.length = 1;
            _.tree_height = 1;
            return true;
        }

        let path_nodes: [MAX_TREE_DEPTH]usize = [0; MAX_TREE_DEPTH];
        let path_pos: [MAX_TREE_DEPTH]usize = [0; MAX_TREE_DEPTH];
        let depth: usize = 0;

        let cur = _.root;
        while (!_.nodes.items[cur].is_leaf) {
            const pos = upperBoundKey(_.nodes.items[cur].keys.items, key);
            path_nodes[depth] = cur;
            path_pos[depth] = pos;
            depth += 1;
            cur = _.nodes.items[cur].children.items[pos];
        }

        const leaf_idx = cur;
        const res = lowerBoundKey(_.nodes.items[leaf_idx].keys.items, key);
        if (res.found) {
            const owned_v = _.alloc.dupe(u8, value) catch return BPlusError.OutOfMemory;
            _.alloc.free(_.nodes.items[leaf_idx].values.items[res.idx]);
            _.nodes.items[leaf_idx].values.items[res.idx] = owned_v;
            return false;
        }

        const owned_k = _.alloc.dupe(u8, key) catch return BPlusError.OutOfMemory;
        errdefer _.alloc.free(owned_k);
        const owned_v = _.alloc.dupe(u8, value) catch return BPlusError.OutOfMemory;
        errdefer _.alloc.free(owned_v);

        _.nodes.items[leaf_idx].keys.insert(_.alloc, res.idx, owned_k) catch return BPlusError.OutOfMemory;
        _.nodes.items[leaf_idx].values.insert(_.alloc, res.idx, owned_v) catch return BPlusError.OutOfMemory;
        _.length += 1;

        if (_.nodes.items[leaf_idx].keys.items.len <= _.maxKeys()) {
            return true;
        }

        // Split leaf
        const total = _.nodes.items[leaf_idx].keys.items.len;
        const mid = total / 2;
        const right_idx = _.allocNode(true)?;

        _.nodes.items[right_idx].keys.appendSlice(_.alloc, _.nodes.items[leaf_idx].keys.items[mid..total]) catch return BPlusError.OutOfMemory;
        _.nodes.items[right_idx].values.appendSlice(_.alloc, _.nodes.items[leaf_idx].values.items[mid..total]) catch return BPlusError.OutOfMemory;
        _.nodes.items[leaf_idx].keys.items.len = mid;
        _.nodes.items[leaf_idx].values.items.len = mid;

        // Link right_idx into doubly-linked leaf chain after leaf_idx
        const old_next = _.nodes.items[leaf_idx].next_leaf;
        _.nodes.items[right_idx].prev_leaf = leaf_idx;
        _.nodes.items[right_idx].next_leaf = old_next;
        if (old_next != NULL_IDX) {
            _.nodes.items[old_next].prev_leaf = right_idx;
        } else {
            _.tail_leaf = right_idx;
        }
        _.nodes.items[leaf_idx].next_leaf = right_idx;

        let sep_key = _.alloc.dupe(u8, _.nodes.items[right_idx].keys.items[0]) catch return BPlusError.OutOfMemory;
        let right_child = right_idx;

        // Propagate split up the ancestor path
        while (depth > 0) {
            depth -= 1;
            const parent_idx = path_nodes[depth];
            const child_pos = path_pos[depth];

            _.nodes.items[parent_idx].keys.insert(_.alloc, child_pos, sep_key) catch return BPlusError.OutOfMemory;
            _.nodes.items[parent_idx].children.insert(_.alloc, child_pos + 1, right_child) catch return BPlusError.OutOfMemory;

            if (_.nodes.items[parent_idx].keys.items.len <= _.maxKeys()) {
                return true;
            }

            // Split internal node
            const p_total = _.nodes.items[parent_idx].keys.items.len;
            const p_mid = p_total / 2;
            const up_key = _.nodes.items[parent_idx].keys.items[p_mid];
            const new_right = _.allocNode(false)?;

            _.nodes.items[new_right].keys.appendSlice(_.alloc, _.nodes.items[parent_idx].keys.items[p_mid + 1 .. p_total]) catch return BPlusError.OutOfMemory;
            _.nodes.items[new_right].children.appendSlice(_.alloc, _.nodes.items[parent_idx].children.items[p_mid + 1 .. p_total + 1]) catch return BPlusError.OutOfMemory;
            _.nodes.items[parent_idx].keys.items.len = p_mid;
            _.nodes.items[parent_idx].children.items.len = p_mid + 1;

            sep_key = up_key;
            right_child = new_right;
        }

        // Root split: create a new root and increase tree height
        const new_root = _.allocNode(false)?;
        _.nodes.items[new_root].keys.append(_.alloc, sep_key) catch return BPlusError.OutOfMemory;
        _.nodes.items[new_root].children.append(_.alloc, _.root) catch return BPlusError.OutOfMemory;
        _.nodes.items[new_root].children.append(_.alloc, right_child) catch return BPlusError.OutOfMemory;
        _.root = new_root;
        _.tree_height += 1;
        return true;
    }

    /// Remove `key` from the tree, rebalancing via borrow/merge and contracting root if needed.
    /// Returns `true` if the key existed and was removed, `false` otherwise.
    pub fn remove(*_, key: string): bool {
        if (_.root == NULL_IDX) return false;

        let path_nodes: [MAX_TREE_DEPTH]usize = [0; MAX_TREE_DEPTH];
        let path_pos: [MAX_TREE_DEPTH]usize = [0; MAX_TREE_DEPTH];
        let depth: usize = 0;

        let cur = _.root;
        while (!_.nodes.items[cur].is_leaf) {
            const pos = upperBoundKey(_.nodes.items[cur].keys.items, key);
            path_nodes[depth] = cur;
            path_pos[depth] = pos;
            depth += 1;
            cur = _.nodes.items[cur].children.items[pos];
        }

        const leaf_idx = cur;
        const res = lowerBoundKey(_.nodes.items[leaf_idx].keys.items, key);
        if (!res.found) return false;

        _.alloc.free(_.nodes.items[leaf_idx].keys.items[res.idx]);
        _.alloc.free(_.nodes.items[leaf_idx].values.items[res.idx]);
        * = _.nodes.items[leaf_idx].keys.orderedRemove(res.idx);
        * = _.nodes.items[leaf_idx].values.orderedRemove(res.idx);
        _.length -= 1;

        if (leaf_idx == _.root) {
            if (_.nodes.items[leaf_idx].keys.items.len == 0) {
                _.freeNode(leaf_idx);
                _.root = NULL_IDX;
                _.head_leaf = NULL_IDX;
                _.tail_leaf = NULL_IDX;
                _.tree_height = 0;
            }
            return true;
        }

        if (_.nodes.items[leaf_idx].keys.items.len >= _.minLeafKeys()) {
            return true;
        }

        // Rebalance leaf against siblings
        const parent_idx = path_nodes[depth - 1];
        const pos = path_pos[depth - 1];

        // 1. Try borrow from left leaf sibling
        if (pos > 0) {
            const left_idx = _.nodes.items[parent_idx].children.items[pos - 1];
            if (_.nodes.items[left_idx].keys.items.len > _.minLeafKeys()) {
                const borrowed_k = _.nodes.items[left_idx].keys.pop().?;
                const borrowed_v = _.nodes.items[left_idx].values.pop().?;
                _.nodes.items[leaf_idx].keys.insert(_.alloc, 0, borrowed_k) catch {};
                _.nodes.items[leaf_idx].values.insert(_.alloc, 0, borrowed_v) catch {};

                const new_sep = _.alloc.dupe(u8, borrowed_k) catch borrowed_k;
                _.alloc.free(_.nodes.items[parent_idx].keys.items[pos - 1]);
                _.nodes.items[parent_idx].keys.items[pos - 1] = new_sep;
                return true;
            }
        }

        // 2. Try borrow from right leaf sibling
        if (pos + 1 < _.nodes.items[parent_idx].children.items.len) {
            const right_idx = _.nodes.items[parent_idx].children.items[pos + 1];
            if (_.nodes.items[right_idx].keys.items.len > _.minLeafKeys()) {
                const borrowed_k = _.nodes.items[right_idx].keys.orderedRemove(0);
                const borrowed_v = _.nodes.items[right_idx].values.orderedRemove(0);
                _.nodes.items[leaf_idx].keys.append(_.alloc, borrowed_k) catch {};
                _.nodes.items[leaf_idx].values.append(_.alloc, borrowed_v) catch {};

                const new_sep = _.alloc.dupe(u8, _.nodes.items[right_idx].keys.items[0]) catch borrowed_k;
                _.alloc.free(_.nodes.items[parent_idx].keys.items[pos]);
                _.nodes.items[parent_idx].keys.items[pos] = new_sep;
                return true;
            }
        }

        // 3. Merge leaf with sibling
        if (pos > 0) {
            const left_idx = _.nodes.items[parent_idx].children.items[pos - 1];
            _.nodes.items[left_idx].keys.appendSlice(_.alloc, _.nodes.items[leaf_idx].keys.items) catch {};
            _.nodes.items[left_idx].values.appendSlice(_.alloc, _.nodes.items[leaf_idx].values.items) catch {};
            _.nodes.items[leaf_idx].keys.items.len = 0;
            _.nodes.items[leaf_idx].values.items.len = 0;

            const next_idx = _.nodes.items[leaf_idx].next_leaf;
            _.nodes.items[left_idx].next_leaf = next_idx;
            if (next_idx != NULL_IDX) {
                _.nodes.items[next_idx].prev_leaf = left_idx;
            } else {
                _.tail_leaf = left_idx;
            }

            _.freeNode(leaf_idx);
            _.alloc.free(_.nodes.items[parent_idx].keys.orderedRemove(pos - 1));
            * = _.nodes.items[parent_idx].children.orderedRemove(pos);
        } else {
            const right_idx = _.nodes.items[parent_idx].children.items[pos + 1];
            _.nodes.items[leaf_idx].keys.appendSlice(_.alloc, _.nodes.items[right_idx].keys.items) catch {};
            _.nodes.items[leaf_idx].values.appendSlice(_.alloc, _.nodes.items[right_idx].values.items) catch {};
            _.nodes.items[right_idx].keys.items.len = 0;
            _.nodes.items[right_idx].values.items.len = 0;

            const next_idx = _.nodes.items[right_idx].next_leaf;
            _.nodes.items[leaf_idx].next_leaf = next_idx;
            if (next_idx != NULL_IDX) {
                _.nodes.items[next_idx].prev_leaf = leaf_idx;
            } else {
                _.tail_leaf = leaf_idx;
            }

            _.freeNode(right_idx);
            _.alloc.free(_.nodes.items[parent_idx].keys.orderedRemove(pos));
            * = _.nodes.items[parent_idx].children.orderedRemove(pos + 1);
        }

        // Propagate underflow up internal nodes
        let d = depth;
        while (d > 0) {
            d -= 1;
            const cur_idx = path_nodes[d];
            if (cur_idx == _.root) {
                if (_.nodes.items[cur_idx].keys.items.len == 0) {
                    const new_root = _.nodes.items[cur_idx].children.items[0];
                    _.freeNode(cur_idx);
                    _.root = new_root;
                    _.tree_height -= 1;
                }
                break;
            }

            if (_.nodes.items[cur_idx].keys.items.len >= _.minInternalKeys()) {
                break;
            }

            const p_idx = path_nodes[d - 1];
            const c_pos = path_pos[d - 1];

            // Try borrow from left internal sibling
            if (c_pos > 0) {
                const l_idx = _.nodes.items[p_idx].children.items[c_pos - 1];
                if (_.nodes.items[l_idx].keys.items.len > _.minInternalKeys()) {
                    const down_k = _.nodes.items[p_idx].keys.items[c_pos - 1];
                    const up_k = _.nodes.items[l_idx].keys.pop().?;
                    const moved_child = _.nodes.items[l_idx].children.pop().?;

                    _.nodes.items[cur_idx].keys.insert(_.alloc, 0, down_k) catch {};
                    _.nodes.items[cur_idx].children.insert(_.alloc, 0, moved_child) catch {};
                    _.nodes.items[p_idx].keys.items[c_pos - 1] = up_k;
                    break;
                }
            }

            // Try borrow from right internal sibling
            if (c_pos + 1 < _.nodes.items[p_idx].children.items.len) {
                const r_idx = _.nodes.items[p_idx].children.items[c_pos + 1];
                if (_.nodes.items[r_idx].keys.items.len > _.minInternalKeys()) {
                    const down_k = _.nodes.items[p_idx].keys.items[c_pos];
                    const up_k = _.nodes.items[r_idx].keys.orderedRemove(0);
                    const moved_child = _.nodes.items[r_idx].children.orderedRemove(0);

                    _.nodes.items[cur_idx].keys.append(_.alloc, down_k) catch {};
                    _.nodes.items[cur_idx].children.append(_.alloc, moved_child) catch {};
                    _.nodes.items[p_idx].keys.items[c_pos] = up_k;
                    break;
                }
            }

            // Merge internal node with sibling
            if (c_pos > 0) {
                const l_idx = _.nodes.items[p_idx].children.items[c_pos - 1];
                const down_k = _.nodes.items[p_idx].keys.orderedRemove(c_pos - 1);
                _.nodes.items[l_idx].keys.append(_.alloc, down_k) catch {};
                _.nodes.items[l_idx].keys.appendSlice(_.alloc, _.nodes.items[cur_idx].keys.items) catch {};
                _.nodes.items[l_idx].children.appendSlice(_.alloc, _.nodes.items[cur_idx].children.items) catch {};
                _.nodes.items[cur_idx].keys.items.len = 0;
                _.nodes.items[cur_idx].children.items.len = 0;
                * = _.nodes.items[p_idx].children.orderedRemove(c_pos);
                _.freeNode(cur_idx);
            } else {
                const r_idx = _.nodes.items[p_idx].children.items[c_pos + 1];
                const down_k = _.nodes.items[p_idx].keys.orderedRemove(c_pos);
                _.nodes.items[cur_idx].keys.append(_.alloc, down_k) catch {};
                _.nodes.items[cur_idx].keys.appendSlice(_.alloc, _.nodes.items[r_idx].keys.items) catch {};
                _.nodes.items[cur_idx].children.appendSlice(_.alloc, _.nodes.items[r_idx].children.items) catch {};
                _.nodes.items[r_idx].keys.items.len = 0;
                _.nodes.items[r_idx].children.items.len = 0;
                * = _.nodes.items[p_idx].children.orderedRemove(c_pos + 1);
                _.freeNode(r_idx);
            }
        }

        return true;
    }

    /// Scan all entries with `start_key <= key <= end_key` (if `include_end == true`)
    /// or `start_key <= key < end_key` (if `include_end == false`).
    /// Caller owns the returned slice and must free it with `alloc.free(entries)`.
    pub fn rangeScan(alloc: std.mem.Allocator, start_key: string, end_key: string, include_end: bool): []BPlusEntry ! BPlusError {
        if (std.mem.order(u8, start_key, end_key) == .gt) {
            return BPlusError.InvalidRange;
        }
        if (_.root == NULL_IDX || (std.mem.eql(u8, start_key, end_key) && !include_end)) {
            return alloc.alloc(BPlusEntry, 0) catch return BPlusError.OutOfMemory;
        }

        let results: std.ArrayList(BPlusEntry) = .empty;
        errdefer results.deinit(alloc);

        let leaf_idx = _.findLeaf(start_key);
        let start_pos = lowerBoundKey(_.nodes.items[leaf_idx].keys.items, start_key).idx;

        while (leaf_idx != NULL_IDX) {
            const leaf = _.nodes.items[leaf_idx];
            for (i of start_pos..leaf.keys.items.len) {
                const k = leaf.keys.items[i];
                const ord = std.mem.order(u8, k, end_key);
                if (ord == .gt || (ord == .eq && !include_end)) {
                    return results.toOwnedSlice(alloc) catch return BPlusError.OutOfMemory;
                }
                results.append(alloc, BPlusEntry{
                    key: k,
                    value: leaf.values.items[i],
                }) catch return BPlusError.OutOfMemory;
            }
            leaf_idx = leaf.next_leaf;
            start_pos = 0;
        }

        return results.toOwnedSlice(alloc) catch return BPlusError.OutOfMemory;
    }

    /// Scan all entries whose keys start with `prefix` in sorted ascending order.
    /// Caller owns the returned slice and must free it with `alloc.free(entries)`.
    pub fn prefixScan(alloc: std.mem.Allocator, prefix: string): []BPlusEntry ! BPlusError {
        if (_.root == NULL_IDX) {
            return alloc.alloc(BPlusEntry, 0) catch return BPlusError.OutOfMemory;
        }

        let results: std.ArrayList(BPlusEntry) = .empty;
        errdefer results.deinit(alloc);

        let leaf_idx = _.findLeaf(prefix);
        let start_pos = lowerBoundKey(_.nodes.items[leaf_idx].keys.items, prefix).idx;

        while (leaf_idx != NULL_IDX) {
            const leaf = _.nodes.items[leaf_idx];
            for (i of start_pos..leaf.keys.items.len) {
                const k = leaf.keys.items[i];
                if (!std.mem.startsWith(u8, k, prefix)) {
                    return results.toOwnedSlice(alloc) catch return BPlusError.OutOfMemory;
                }
                results.append(alloc, BPlusEntry{
                    key: k,
                    value: leaf.values.items[i],
                }) catch return BPlusError.OutOfMemory;
            }
            leaf_idx = leaf.next_leaf;
            start_pos = 0;
        }

        return results.toOwnedSlice(alloc) catch return BPlusError.OutOfMemory;
    }

    fn verifyNode(idx: usize, depth: usize, min_bound: string?, max_bound: string?, expected_depth: usize, leaf_count: *usize): bool {
        if (idx >= _.nodes.items.len) return false;
        const node = _.nodes.items[idx];
        const is_root = (idx == _.root);

        // Verify keys are strictly increasing and within [min_bound, max_bound)
        for (i of 0..node.keys.items.len) {
            const k = node.keys.items[i];
            if (min_bound != null && std.mem.order(u8, k, min_bound.?) == .lt) return false;
            if (max_bound != null && std.mem.order(u8, k, max_bound.?) != .lt) return false;
            if (i + 1 < node.keys.items.len) {
                if (std.mem.order(u8, k, node.keys.items[i + 1]) != .lt) return false;
            }
        }

        if (node.keys.items.len > _.maxKeys()) return false;

        if (node.is_leaf) {
            if (depth != expected_depth) return false;
            if (node.values.items.len != node.keys.items.len) return false;
            if (node.children.items.len != 0) return false;
            if (is_root) {
                if (node.keys.items.len < 1) return false;
            } else {
                if (node.keys.items.len < _.minLeafKeys()) return false;
            }
            leaf_count.* += 1;
            return true;
        }

        // Internal node checks
        if (depth >= expected_depth) return false;
        if (node.values.items.len != 0) return false;
        if (node.children.items.len != node.keys.items.len + 1) return false;
        if (is_root) {
            if (node.keys.items.len < 1) return false;
        } else {
            if (node.keys.items.len < _.minInternalKeys()) return false;
        }

        for (i of 0..node.children.items.len) {
            const c_min: string? = if (i == 0) min_bound else node.keys.items[i - 1];
            const c_max: string? = if (i == node.keys.items.len) max_bound else node.keys.items[i];
            if (!_.verifyNode(node.children.items[i], depth + 1, c_min, c_max, expected_depth, leaf_count)) {
                return false;
            }
        }
        return true;
    }

    /// Verify all B+ Tree structural invariants:
    /// - Uniform leaf depth == `tree_height`
    /// - Node key counts within `[min, max]` bounds
    /// - Internal router keys strictly partition child key ranges
    /// - Doubly-linked leaf chain is strictly sorted, consistent with `head_leaf`/`tail_leaf`, and totals `length`
    pub fn verifyInvariants(): bool {
        if (_.length == 0) {
            return _.root == NULL_IDX &&
                _.head_leaf == NULL_IDX &&
                _.tail_leaf == NULL_IDX &&
                _.tree_height == 0;
        }
        if (_.root == NULL_IDX || _.head_leaf == NULL_IDX || _.tail_leaf == NULL_IDX || _.tree_height == 0) {
            return false;
        }

        let dfs_leaves: usize = 0;
        if (!_.verifyNode(_.root, 1, null, null, _.tree_height, *dfs_leaves)) {
            return false;
        }

        // Walk leaf linked list forward and verify prev_leaf, tail_leaf, strict ordering, and total count
        let chain_leaves: usize = 0;
        let chain_keys: usize = 0;
        let prev_idx: usize = NULL_IDX;
        let cur = _.head_leaf;
        let last_key: string? = null;

        while (cur != NULL_IDX) {
            if (cur >= _.nodes.items.len) return false;
            const leaf = _.nodes.items[cur];
            if (!leaf.is_leaf) return false;
            if (leaf.prev_leaf != prev_idx) return false;

            for (k of leaf.keys.items) {
                if (last_key != null) {
                    if (std.mem.order(u8, last_key.?, k) != .lt) return false;
                }
                last_key = k;
                chain_keys += 1;
            }

            chain_leaves += 1;
            if (leaf.next_leaf == NULL_IDX) {
                if (cur != _.tail_leaf) return false;
            }
            prev_idx = cur;
            cur = leaf.next_leaf;
        }

        return chain_leaves == dfs_leaves && chain_keys == _.length;
    }
}
