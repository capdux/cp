// Reference-hardened against rust-lang/rust BTree & rust-bio IntervalTree (MIT OR Apache-2.0)
// Source: native-cp-stdlib (Ordered B+ Tree Index with 4-Lane SIMD Node Search & Invariant Verification)
import std from "std";

const MAX_KEYS: usize = 16;
const MIN_KEYS: usize = 8;
const NODE_KEYS_CAP: usize = 17;
const NODE_CHILDREN_CAP: usize = 18;

type BptItVec4i64 = @Vector(4, i64);

/// Scalar reference: counts how many elements in `keys` are strictly less than `target`.
/// On a sorted B+Tree leaf key slice, this equals the exact lower_bound slot index.
pub fn scalarBptCountLessI64(keys: []const i64, target: i64): usize {
    let cnt: usize = 0;
    let i: usize = 0;
    while (i < keys.len) : (i += 1) {
        if (keys[i] < target) cnt += 1;
    }
    return cnt;
}

/// 4-lane `@Vector(4, i64)` SIMD batch count of elements in `keys` strictly less than `target`.
/// Matches `scalarBptCountLessI64` with 0 discrepancy across all lengths.
pub fn simdBptCountLessI64(keys: []const i64, target: i64): usize {
    const n: usize = keys.len;
    const v_target: BptItVec4i64 = @splat(target);
    let cnt: usize = 0;
    let i: usize = 0;
    while (i + 4 <= n) : (i += 4) {
        const vk: BptItVec4i64 = keys[i .. i + 4][0..4].*;
        const m0: usize = @intFromBool(vk[0] < v_target[0]);
        const m1: usize = @intFromBool(vk[1] < v_target[1]);
        const m2: usize = @intFromBool(vk[2] < v_target[2]);
        const m3: usize = @intFromBool(vk[3] < v_target[3]);
        cnt += m0 + m1 + m2 + m3;
    }
    while (i < n) : (i += 1) {
        if (keys[i] < target) cnt += 1;
    }
    return cnt;
}

fn simdBptCountLeI64(keys: []const i64, target: i64): usize {
    const n: usize = keys.len;
    const v_target: BptItVec4i64 = @splat(target);
    let cnt: usize = 0;
    let i: usize = 0;
    while (i + 4 <= n) : (i += 4) {
        const vk: BptItVec4i64 = keys[i .. i + 4][0..4].*;
        const m0: usize = @intFromBool(vk[0] <= v_target[0]);
        const m1: usize = @intFromBool(vk[1] <= v_target[1]);
        const m2: usize = @intFromBool(vk[2] <= v_target[2]);
        const m3: usize = @intFromBool(vk[3] <= v_target[3]);
        cnt += m0 + m1 + m2 + m3;
    }
    while (i < n) : (i += 1) {
        if (keys[i] <= target) cnt += 1;
    }
    return cnt;
}

pub fn BPlusTree(comptime K: type, comptime V: type): type {
    return struct {
        const Self = @This();

        pub const Entry = struct {
            key: K,
            value: V,
        };

        const Node = struct {
            is_leaf: bool,
            num_keys: usize,
            keys: [NODE_KEYS_CAP]K,
            values: [NODE_KEYS_CAP]V,
            children: [NODE_CHILDREN_CAP]*@This(),
            next: (*@This())?,
        };

        const SplitResult = struct {
            promoted_key: K,
            right_node: *Node,
        };

        const InsertResult = struct {
            old_value: V?,
            split: SplitResult?,
        };

        const RemoveResult = struct {
            removed: V?,
            underflow: bool,
        };

        const SubtreeCheck = struct {
            ok: bool,
            leaf_depth: usize,
            leaf_count: usize,
        };

        alloc: std.mem.Allocator,
        root: (*Node)?,
        count: usize,

        pub fn init(alloc: std.mem.Allocator): Self {
            return Self{
                .alloc = alloc,
                .root = null,
                .count = 0,
            };
        }

        pub fn deinit(*_) {
            if (r := _.root) {
                _.freeNode(r);
                _.root = null;
            }
            _.count = 0;
        }

        fn freeNode(node: *Node) {
            if (!node.is_leaf) {
                let i: usize = 0;
                while (i <= node.num_keys) : (i += 1) {
                    _.freeNode(node.children[i]);
                }
            }
            _.alloc.destroy(node);
        }

        fn createNode(is_leaf: bool): !*Node {
            let node = try _.alloc.create(Node);
            node.is_leaf = is_leaf;
            node.num_keys = 0;
            node.next = null;
            return node;
        }

        static fn findLeafSlot(keys: []const K, key: K): usize {
            if (comptime K == i64) {
                return simdBptCountLessI64(keys, key);
            }
            let lo: usize = 0;
            let hi: usize = keys.len;
            while (lo < hi) {
                const mid = lo + (hi - lo) / 2;
                if (keys[mid] < key) {
                    lo = mid + 1;
                } else {
                    hi = mid;
                }
            }
            return lo;
        }

        static fn findChildSlot(keys: []const K, key: K): usize {
            if (comptime K == i64) {
                return simdBptCountLeI64(keys, key);
            }
            let lo: usize = 0;
            let hi: usize = keys.len;
            while (lo < hi) {
                const mid = lo + (hi - lo) / 2;
                if (keys[mid] <= key) {
                    lo = mid + 1;
                } else {
                    hi = mid;
                }
            }
            return lo;
        }

        pub fn len(): usize {
            return _.count;
        }

        pub fn isEmpty(): bool {
            return _.count == 0;
        }

        pub fn contains(key: K): bool {
            return _.get(key) != null;
        }

        pub fn get(key: K): V? {
            let cur_opt = _.root;
            while (cur := cur_opt) {
                if (cur.is_leaf) {
                    const pos = findLeafSlot(cur.keys[0..cur.num_keys], key);
                    if (pos < cur.num_keys and cur.keys[pos] == key) {
                        return cur.values[pos];
                    }
                    return null;
                } else {
                    const ci = findChildSlot(cur.keys[0..cur.num_keys], key);
                    cur_opt = cur.children[ci];
                }
            }
            return null;
        }

        pub fn insert(*_, key: K, val: V): !V? {
            if (_.root == null) {
                let leaf = try _.createNode(true);
                leaf.keys[0] = key;
                leaf.values[0] = val;
                leaf.num_keys = 1;
                _.root = leaf;
                _.count = 1;
                return null;
            }

            const root_node = _.root.?;
            const res = try _.insertIntoNode(root_node, key, val);
            if (res.old_value == null) {
                _.count += 1;
            }
            if (sp := res.split) {
                let new_root = try _.createNode(false);
                new_root.keys[0] = sp.promoted_key;
                new_root.children[0] = root_node;
                new_root.children[1] = sp.right_node;
                new_root.num_keys = 1;
                _.root = new_root;
            }
            return res.old_value;
        }

        fn insertIntoNode(node: *Node, key: K, val: V): !InsertResult {
            if (node.is_leaf) {
                const pos = findLeafSlot(node.keys[0..node.num_keys], key);
                if (pos < node.num_keys and node.keys[pos] == key) {
                    const prev = node.values[pos];
                    node.values[pos] = val;
                    return InsertResult{ .old_value = prev, .split = null };
                }

                let j: usize = node.num_keys;
                while (j > pos) {
                    node.keys[j] = node.keys[j - 1];
                    node.values[j] = node.values[j - 1];
                    j -= 1;
                }
                node.keys[pos] = key;
                node.values[pos] = val;
                node.num_keys += 1;

                if (node.num_keys <= MAX_KEYS) {
                    return InsertResult{ .old_value = null, .split = null };
                }

                const mid: usize = node.num_keys / 2;
                let right = try _.createNode(true);
                const right_count = node.num_keys - mid;
                let k: usize = 0;
                while (k < right_count) : (k += 1) {
                    right.keys[k] = node.keys[mid + k];
                    right.values[k] = node.values[mid + k];
                }
                right.num_keys = right_count;
                node.num_keys = mid;
                right.next = node.next;
                node.next = right;

                return InsertResult{
                    .old_value = null,
                    .split = SplitResult{
                        .promoted_key = right.keys[0],
                        .right_node = right,
                    },
                };
            }

            const ci = findChildSlot(node.keys[0..node.num_keys], key);
            const child = node.children[ci];
            const child_res = try _.insertIntoNode(child, key, val);
            if (child_res.split == null) {
                return child_res;
            }

            const sp = child_res.split.?;
            let j: usize = node.num_keys;
            while (j > ci) {
                node.keys[j] = node.keys[j - 1];
                node.children[j + 1] = node.children[j];
                j -= 1;
            }
            node.keys[ci] = sp.promoted_key;
            node.children[ci + 1] = sp.right_node;
            node.num_keys += 1;

            if (node.num_keys <= MAX_KEYS) {
                return InsertResult{ .old_value = child_res.old_value, .split = null };
            }

            const mid: usize = node.num_keys / 2;
            const up_key = node.keys[mid];
            let right = try _.createNode(false);
            const right_keys = node.num_keys - mid - 1;
            let k: usize = 0;
            while (k < right_keys) : (k += 1) {
                right.keys[k] = node.keys[mid + 1 + k];
                right.children[k] = node.children[mid + 1 + k];
            }
            right.children[right_keys] = node.children[node.num_keys];
            right.num_keys = right_keys;
            node.num_keys = mid;

            return InsertResult{
                .old_value = child_res.old_value,
                .split = SplitResult{
                    .promoted_key = up_key,
                    .right_node = right,
                },
            };
        }

        pub fn remove(*_, key: K): V? {
            if (_.root == null) {
                return null;
            }
            const root_node = _.root.?;
            const res = _.removeFromNode(root_node, key);
            if (res.removed == null) {
                return null;
            }
            _.count -= 1;

            if (root_node.is_leaf) {
                if (root_node.num_keys == 0) {
                    _.alloc.destroy(root_node);
                    _.root = null;
                }
            } else {
                if (root_node.num_keys == 0) {
                    _.root = root_node.children[0];
                    _.alloc.destroy(root_node);
                }
            }
            return res.removed;
        }

        fn removeFromNode(node: *Node, key: K): RemoveResult {
            if (node.is_leaf) {
                const pos = findLeafSlot(node.keys[0..node.num_keys], key);
                if (pos >= node.num_keys or node.keys[pos] != key) {
                    return RemoveResult{ .removed = null, .underflow = false };
                }
                const old_val = node.values[pos];
                let j: usize = pos;
                while (j + 1 < node.num_keys) : (j += 1) {
                    node.keys[j] = node.keys[j + 1];
                    node.values[j] = node.values[j + 1];
                }
                node.num_keys -= 1;
                return RemoveResult{
                    .removed = old_val,
                    .underflow = node.num_keys < MIN_KEYS,
                };
            }

            const ci = findChildSlot(node.keys[0..node.num_keys], key);
            const child = node.children[ci];
            const res = _.removeFromNode(child, key);
            if (res.removed == null or !res.underflow) {
                return res;
            }

            if (ci > 0) {
                const left = node.children[ci - 1];
                if (left.num_keys > MIN_KEYS) {
                    borrowFromLeft(node, ci, left, child);
                    return RemoveResult{ .removed = res.removed, .underflow = false };
                }
            }

            if (ci < node.num_keys) {
                const right = node.children[ci + 1];
                if (right.num_keys > MIN_KEYS) {
                    borrowFromRight(node, ci, child, right);
                    return RemoveResult{ .removed = res.removed, .underflow = false };
                }
            }

            if (ci > 0) {
                const left = node.children[ci - 1];
                _.mergeChildren(node, ci - 1, left, child);
            } else {
                const right = node.children[ci + 1];
                _.mergeChildren(node, ci, child, right);
            }

            return RemoveResult{
                .removed = res.removed,
                .underflow = node.num_keys < MIN_KEYS,
            };
        }

        static fn borrowFromLeft(parent: *Node, ci: usize, left: *Node, child: *Node) {
            if (child.is_leaf) {
                let j: usize = child.num_keys;
                while (j > 0) {
                    child.keys[j] = child.keys[j - 1];
                    child.values[j] = child.values[j - 1];
                    j -= 1;
                }
                child.keys[0] = left.keys[left.num_keys - 1];
                child.values[0] = left.values[left.num_keys - 1];
                left.num_keys -= 1;
                child.num_keys += 1;
                parent.keys[ci - 1] = child.keys[0];
            } else {
                let j: usize = child.num_keys;
                while (j > 0) {
                    child.keys[j] = child.keys[j - 1];
                    j -= 1;
                }
                let k: usize = child.num_keys + 1;
                while (k > 0) {
                    child.children[k] = child.children[k - 1];
                    k -= 1;
                }
                child.keys[0] = parent.keys[ci - 1];
                child.children[0] = left.children[left.num_keys];
                parent.keys[ci - 1] = left.keys[left.num_keys - 1];
                left.num_keys -= 1;
                child.num_keys += 1;
            }
        }

        static fn borrowFromRight(parent: *Node, ci: usize, child: *Node, right: *Node) {
            if (child.is_leaf) {
                child.keys[child.num_keys] = right.keys[0];
                child.values[child.num_keys] = right.values[0];
                child.num_keys += 1;
                let j: usize = 0;
                while (j + 1 < right.num_keys) : (j += 1) {
                    right.keys[j] = right.keys[j + 1];
                    right.values[j] = right.values[j + 1];
                }
                right.num_keys -= 1;
                parent.keys[ci] = right.keys[0];
            } else {
                child.keys[child.num_keys] = parent.keys[ci];
                child.children[child.num_keys + 1] = right.children[0];
                child.num_keys += 1;
                parent.keys[ci] = right.keys[0];
                let j: usize = 0;
                while (j + 1 < right.num_keys) : (j += 1) {
                    right.keys[j] = right.keys[j + 1];
                }
                let k: usize = 0;
                while (k < right.num_keys) : (k += 1) {
                    right.children[k] = right.children[k + 1];
                }
                right.num_keys -= 1;
            }
        }

        fn mergeChildren(parent: *Node, sep_idx: usize, left: *Node, right: *Node) {
            if (left.is_leaf) {
                let k: usize = 0;
                while (k < right.num_keys) : (k += 1) {
                    left.keys[left.num_keys + k] = right.keys[k];
                    left.values[left.num_keys + k] = right.values[k];
                }
                left.num_keys += right.num_keys;
                left.next = right.next;
            } else {
                left.keys[left.num_keys] = parent.keys[sep_idx];
                left.num_keys += 1;
                let k: usize = 0;
                while (k < right.num_keys) : (k += 1) {
                    left.keys[left.num_keys + k] = right.keys[k];
                    left.children[left.num_keys + k] = right.children[k];
                }
                left.children[left.num_keys + right.num_keys] = right.children[right.num_keys];
                left.num_keys += right.num_keys;
            }
            _.alloc.destroy(right);

            let j: usize = sep_idx;
            while (j + 1 < parent.num_keys) : (j += 1) {
                parent.keys[j] = parent.keys[j + 1];
                parent.children[j + 1] = parent.children[j + 2];
            }
            parent.num_keys -= 1;
        }

        fn findLeaf(key: K): *Node {
            let cur = _.root.?;
            while (!cur.is_leaf) {
                const ci = findChildSlot(cur.keys[0..cur.num_keys], key);
                cur = cur.children[ci];
            }
            return cur;
        }

        pub fn rangeScan(alloc: std.mem.Allocator, min_key: K, max_key: K): ![]Entry {
            let list = std.ArrayList(Entry).empty;
            errdefer list.deinit(alloc);

            if (min_key > max_key or _.root == null) {
                return try list.toOwnedSlice(alloc);
            }

            let leaf_opt: (*Node)? = _.findLeaf(min_key);
            let first_leaf = true;
            while (leaf := leaf_opt) {
                let i: usize = 0;
                if (first_leaf) {
                    i = findLeafSlot(leaf.keys[0..leaf.num_keys], min_key);
                    first_leaf = false;
                }
                while (i < leaf.num_keys) : (i += 1) {
                    const k = leaf.keys[i];
                    if (k > max_key) {
                        return try list.toOwnedSlice(alloc);
                    }
                    try list.append(alloc, Entry{
                        .key = k,
                        .value = leaf.values[i],
                    });
                }
                leaf_opt = leaf.next;
            }

            return try list.toOwnedSlice(alloc);
        }

        static fn checkNodeInvariants(
            node: *Node,
            is_root: bool,
            depth: usize,
            has_lo: bool,
            lo_bound: K,
            has_hi: bool,
            hi_bound: K,
        ): SubtreeCheck {
            if (node.num_keys > MAX_KEYS) {
                return SubtreeCheck{ .ok = false, .leaf_depth = 0, .leaf_count = 0 };
            }
            if (is_root) {
                if (node.num_keys == 0) {
                    return SubtreeCheck{ .ok = false, .leaf_depth = 0, .leaf_count = 0 };
                }
            } else {
                if (node.num_keys < MIN_KEYS) {
                    return SubtreeCheck{ .ok = false, .leaf_depth = 0, .leaf_count = 0 };
                }
            }

            // Strictly increasing keys + subtree range bounds
            let i: usize = 0;
            while (i < node.num_keys) : (i += 1) {
                const k = node.keys[i];
                if (i > 0 and !(node.keys[i - 1] < k)) {
                    return SubtreeCheck{ .ok = false, .leaf_depth = 0, .leaf_count = 0 };
                }
                if (has_lo and k < lo_bound) {
                    return SubtreeCheck{ .ok = false, .leaf_depth = 0, .leaf_count = 0 };
                }
                if (has_hi and !(k < hi_bound)) {
                    return SubtreeCheck{ .ok = false, .leaf_depth = 0, .leaf_count = 0 };
                }
            }

            if (node.is_leaf) {
                return SubtreeCheck{ .ok = true, .leaf_depth = depth, .leaf_count = node.num_keys };
            }

            let expected_depth: usize = 0;
            let total_leaves: usize = 0;
            let ci: usize = 0;
            while (ci <= node.num_keys) : (ci += 1) {
                const child_has_lo: bool = if (ci == 0) has_lo else true;
                const child_lo: K = if (ci == 0) lo_bound else node.keys[ci - 1];
                const child_has_hi: bool = if (ci == node.num_keys) has_hi else true;
                const child_hi: K = if (ci == node.num_keys) hi_bound else node.keys[ci];

                const sub = checkNodeInvariants(
                    node.children[ci],
                    false,
                    depth + 1,
                    child_has_lo,
                    child_lo,
                    child_has_hi,
                    child_hi,
                );
                if (!sub.ok) {
                    return SubtreeCheck{ .ok = false, .leaf_depth = 0, .leaf_count = 0 };
                }
                if (ci == 0) {
                    expected_depth = sub.leaf_depth;
                } else if (sub.leaf_depth != expected_depth) {
                    return SubtreeCheck{ .ok = false, .leaf_depth = 0, .leaf_count = 0 };
                }
                total_leaves += sub.leaf_count;
            }
            return SubtreeCheck{ .ok = true, .leaf_depth = expected_depth, .leaf_count = total_leaves };
        }

        /// Verifies all B+Tree structural & ordering invariants:
        /// equal leaf depth, occupancy bounds, strict monotonicity, separator bounds, and leaf chain integrity.
        pub fn verifyInvariants(): bool {
            if (_.root == null) {
                return _.count == 0;
            }
            const r = _.root.?;
            const chk = checkNodeInvariants(r, true, 0, false, r.keys[0], false, r.keys[0]);
            if (!chk.ok or chk.leaf_count != _.count) {
                return false;
            }

            // Walk leaf linked list from leftmost leaf and verify strict ordering & exact count.
            let leftmost = r;
            while (!leftmost.is_leaf) {
                leftmost = leftmost.children[0];
            }
            let cur_opt: (*Node)? = leftmost;
            let seen: usize = 0;
            let has_prev: bool = false;
            let prev_key: K = leftmost.keys[0];
            while (leaf := cur_opt) {
                if (!leaf.is_leaf) return false;
                let i: usize = 0;
                while (i < leaf.num_keys) : (i += 1) {
                    const k = leaf.keys[i];
                    if (has_prev and !(prev_key < k)) return false;
                    prev_key = k;
                    has_prev = true;
                    seen += 1;
                }
                cur_opt = leaf.next;
            }
            return seen == _.count;
        }
    };
}
