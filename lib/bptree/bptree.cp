// Source: native-cp-stdlib (Ordered B+ Tree Index)
import std from "std";

const MAX_KEYS: usize = 16;
const MIN_KEYS: usize = 8;
const NODE_KEYS_CAP: usize = 17;
const NODE_CHILDREN_CAP: usize = 18;

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

        pub fn deinit(*self) {
            if (r := self.root) {
                self.freeNode(r);
                self.root = null;
            }
            self.count = 0;
        }

        fn freeNode(*self, node: *Node) {
            if (!node.is_leaf) {
                let i: usize = 0;
                while (i <= node.num_keys) {
                    self.freeNode(node.children[i]);
                    i += 1;
                }
            }
            self.alloc.destroy(node);
        }

        fn createNode(*self, is_leaf: bool): !*Node {
            let node = try self.alloc.create(Node);
            node.is_leaf = is_leaf;
            node.num_keys = 0;
            node.next = null;
            return node;
        }

        pub fn len(self): usize {
            return self.count;
        }

        pub fn get(self, key: K): V? {
            let cur_opt = self.root;
            while (cur := cur_opt) {
                if (cur.is_leaf) {
                    let lo: usize = 0;
                    let hi: usize = cur.num_keys;
                    while (lo < hi) {
                        const mid = lo + (hi - lo) / 2;
                        if (cur.keys[mid] < key) {
                            lo = mid + 1;
                        } else {
                            hi = mid;
                        }
                    }
                    if (lo < cur.num_keys and cur.keys[lo] == key) {
                        return cur.values[lo];
                    }
                    return null;
                } else {
                    let ci: usize = 0;
                    while (ci < cur.num_keys and key >= cur.keys[ci]) {
                        ci += 1;
                    }
                    cur_opt = cur.children[ci];
                }
            }
            return null;
        }

        pub fn insert(*self, key: K, val: V): !V? {
            if (self.root == null) {
                let leaf = try self.createNode(true);
                leaf.keys[0] = key;
                leaf.values[0] = val;
                leaf.num_keys = 1;
                self.root = leaf;
                self.count = 1;
                return null;
            }

            const root_node = self.root.?;
            const res = try self.insertIntoNode(root_node, key, val);
            if (res.old_value == null) {
                self.count += 1;
            }
            if (sp := res.split) {
                let new_root = try self.createNode(false);
                new_root.keys[0] = sp.promoted_key;
                new_root.children[0] = root_node;
                new_root.children[1] = sp.right_node;
                new_root.num_keys = 1;
                self.root = new_root;
            }
            return res.old_value;
        }

        fn insertIntoNode(*self, node: *Node, key: K, val: V): !InsertResult {
            if (node.is_leaf) {
                let pos: usize = 0;
                while (pos < node.num_keys and node.keys[pos] < key) {
                    pos += 1;
                }
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
                let right = try self.createNode(true);
                const right_count = node.num_keys - mid;
                let k: usize = 0;
                while (k < right_count) {
                    right.keys[k] = node.keys[mid + k];
                    right.values[k] = node.values[mid + k];
                    k += 1;
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

            let ci: usize = 0;
            while (ci < node.num_keys and key >= node.keys[ci]) {
                ci += 1;
            }
            const child = node.children[ci];
            const child_res = try self.insertIntoNode(child, key, val);
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
            let right = try self.createNode(false);
            const right_keys = node.num_keys - mid - 1;
            let k: usize = 0;
            while (k < right_keys) {
                right.keys[k] = node.keys[mid + 1 + k];
                right.children[k] = node.children[mid + 1 + k];
                k += 1;
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

        pub fn remove(*self, key: K): V? {
            if (self.root == null) {
                return null;
            }
            const root_node = self.root.?;
            const res = self.removeFromNode(root_node, key);
            if (res.removed == null) {
                return null;
            }
            self.count -= 1;

            if (root_node.is_leaf) {
                if (root_node.num_keys == 0) {
                    self.alloc.destroy(root_node);
                    self.root = null;
                }
            } else {
                if (root_node.num_keys == 0) {
                    self.root = root_node.children[0];
                    self.alloc.destroy(root_node);
                }
            }
            return res.removed;
        }

        fn removeFromNode(*self, node: *Node, key: K): RemoveResult {
            if (node.is_leaf) {
                let pos: usize = 0;
                while (pos < node.num_keys and node.keys[pos] < key) {
                    pos += 1;
                }
                if (pos >= node.num_keys or node.keys[pos] != key) {
                    return RemoveResult{ .removed = null, .underflow = false };
                }
                const old_val = node.values[pos];
                let j: usize = pos;
                while (j + 1 < node.num_keys) {
                    node.keys[j] = node.keys[j + 1];
                    node.values[j] = node.values[j + 1];
                    j += 1;
                }
                node.num_keys -= 1;
                return RemoveResult{
                    .removed = old_val,
                    .underflow = node.num_keys < MIN_KEYS,
                };
            }

            let ci: usize = 0;
            while (ci < node.num_keys and key >= node.keys[ci]) {
                ci += 1;
            }
            const child = node.children[ci];
            const res = self.removeFromNode(child, key);
            if (res.removed == null or !res.underflow) {
                return res;
            }

            if (ci > 0) {
                const left = node.children[ci - 1];
                if (left.num_keys > MIN_KEYS) {
                    self.borrowFromLeft(node, ci, left, child);
                    return RemoveResult{ .removed = res.removed, .underflow = false };
                }
            }

            if (ci < node.num_keys) {
                const right = node.children[ci + 1];
                if (right.num_keys > MIN_KEYS) {
                    self.borrowFromRight(node, ci, child, right);
                    return RemoveResult{ .removed = res.removed, .underflow = false };
                }
            }

            if (ci > 0) {
                const left = node.children[ci - 1];
                self.mergeChildren(node, ci - 1, left, child);
            } else {
                const right = node.children[ci + 1];
                self.mergeChildren(node, ci, child, right);
            }

            return RemoveResult{
                .removed = res.removed,
                .underflow = node.num_keys < MIN_KEYS,
            };
        }

        fn borrowFromLeft(*self, parent: *Node, ci: usize, left: *Node, child: *Node) {
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

        fn borrowFromRight(*self, parent: *Node, ci: usize, child: *Node, right: *Node) {
            if (child.is_leaf) {
                child.keys[child.num_keys] = right.keys[0];
                child.values[child.num_keys] = right.values[0];
                child.num_keys += 1;
                let j: usize = 0;
                while (j + 1 < right.num_keys) {
                    right.keys[j] = right.keys[j + 1];
                    right.values[j] = right.values[j + 1];
                    j += 1;
                }
                right.num_keys -= 1;
                parent.keys[ci] = right.keys[0];
            } else {
                child.keys[child.num_keys] = parent.keys[ci];
                child.children[child.num_keys + 1] = right.children[0];
                child.num_keys += 1;
                parent.keys[ci] = right.keys[0];
                let j: usize = 0;
                while (j + 1 < right.num_keys) {
                    right.keys[j] = right.keys[j + 1];
                    j += 1;
                }
                let k: usize = 0;
                while (k < right.num_keys) {
                    right.children[k] = right.children[k + 1];
                    k += 1;
                }
                right.num_keys -= 1;
            }
        }

        fn mergeChildren(*self, parent: *Node, sep_idx: usize, left: *Node, right: *Node) {
            if (left.is_leaf) {
                let k: usize = 0;
                while (k < right.num_keys) {
                    left.keys[left.num_keys + k] = right.keys[k];
                    left.values[left.num_keys + k] = right.values[k];
                    k += 1;
                }
                left.num_keys += right.num_keys;
                left.next = right.next;
            } else {
                left.keys[left.num_keys] = parent.keys[sep_idx];
                left.num_keys += 1;
                let k: usize = 0;
                while (k < right.num_keys) {
                    left.keys[left.num_keys + k] = right.keys[k];
                    left.children[left.num_keys + k] = right.children[k];
                    k += 1;
                }
                left.children[left.num_keys + right.num_keys] = right.children[right.num_keys];
                left.num_keys += right.num_keys;
            }
            self.alloc.destroy(right);

            let j: usize = sep_idx;
            while (j + 1 < parent.num_keys) {
                parent.keys[j] = parent.keys[j + 1];
                parent.children[j + 1] = parent.children[j + 2];
                j += 1;
            }
            parent.num_keys -= 1;
        }

        fn findLeaf(self, key: K): *Node {
            let cur = self.root.?;
            while (!cur.is_leaf) {
                let ci: usize = 0;
                while (ci < cur.num_keys and key >= cur.keys[ci]) {
                    ci += 1;
                }
                cur = cur.children[ci];
            }
            return cur;
        }

        pub fn rangeScan(self, alloc: std.mem.Allocator, min_key: K, max_key: K): ![]Entry {
            let list = std.ArrayList(Entry).empty;
            errdefer list.deinit(alloc);

            if (min_key > max_key or self.root == null) {
                return try list.toOwnedSlice(alloc);
            }

            let leaf_opt: (*Node)? = self.findLeaf(min_key);
            let first_leaf = true;
            while (leaf := leaf_opt) {
                let i: usize = 0;
                if (first_leaf) {
                    while (i < leaf.num_keys and leaf.keys[i] < min_key) {
                        i += 1;
                    }
                    first_leaf = false;
                }
                while (i < leaf.num_keys) {
                    const k = leaf.keys[i];
                    if (k > max_key) {
                        return try list.toOwnedSlice(alloc);
                    }
                    try list.append(alloc, Entry{
                        .key = k,
                        .value = leaf.values[i],
                    });
                    i += 1;
                }
                leaf_opt = leaf.next;
            }

            return try list.toOwnedSlice(alloc);
        }
    };
}
