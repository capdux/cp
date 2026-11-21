// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Randomized Treap with Order Statistics, SplitMix64 PRNG & Free-List Pool)

/// Sentinel index representing a null child or empty free-list link.
pub const TREAP_NIL: u32 = 0xFFFFFFFF;

/// Single node stored inside the caller-owned fixed-capacity Treap pool.
pub struct TreapNode {
    key: i64;
    value: i64;
    priority: u64;
    left: u32;
    right: u32;
    size: u32;
}

/// Key-value entry returned by order-statistic and bound queries.
pub struct TreapEntry {
    key: i64;
    value: i64;
}

/// Zero-allocation randomized balanced BST (Treap) with O(log n) order statistics
/// (`kth`, `rank`, `lowerBound`, `upperBound`, `rangeCount`) backed by a caller-owned node pool
/// and deterministic SplitMix64 priority generator.
pub struct Treap {
    nodes: []TreapNode;
    root: u32;
    free_head: u32;
    count: usize;
    rng_state: u64;

    /// Initializes an empty `Treap` over caller-provided `nodes` storage with deterministic `seed`.
    static fn init(nodes: []TreapNode, seed: u64): Treap {
        const cap = nodes.len;
        let i: usize = 0;
        while (i < cap) {
            nodes[i] = TreapNode {
                key: 0,
                value: 0,
                priority: 0,
                left: TREAP_NIL,
                right: if (i + 1 < cap) ((i + 1) as u32) else TREAP_NIL,
                size: 0,
            };
            i += 1;
        }
        return Treap {
            nodes: nodes,
            root: TREAP_NIL,
            free_head: if (cap > 0) (0 as u32) else TREAP_NIL,
            count: 0,
            rng_state: if (seed == 0) 0x9e3779b97f4a7c15 else seed,
        };
    }

    /// Returns the number of live key-value pairs stored in the Treap.
    fn len(): usize {
        return _.count;
    }

    /// Returns `true` if the Treap contains no elements.
    fn isEmpty(): bool {
        return _.count == 0;
    }

    /// Returns the maximum number of nodes supported by the backing pool.
    fn capacity(): usize {
        return _.nodes.len;
    }

    /// Removes all elements and resets the entire backing pool into the free list.
    fn clear(*_) {
        const cap = _.nodes.len;
        let i: usize = 0;
        while (i < cap) {
            _.nodes[i] = TreapNode {
                key: 0,
                value: 0,
                priority: 0,
                left: TREAP_NIL,
                right: if (i + 1 < cap) ((i + 1) as u32) else TREAP_NIL,
                size: 0,
            };
            i += 1;
        }
        _.root = TREAP_NIL;
        _.free_head = if (cap > 0) (0 as u32) else TREAP_NIL;
        _.count = 0;
    }

    fn nextPriority(*_): u64 {
        _.rng_state = _.rng_state +% 0x9e3779b97f4a7c15;
        let z = _.rng_state;
        z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
        z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
        return z ^ (z >> 31);
    }

    fn nodeSize(idx: u32): u32 {
        if (idx == TREAP_NIL) {
            return 0;
        }
        return _.nodes[idx as usize].size;
    }

    fn updateSize(*_, idx: u32) {
        if (idx == TREAP_NIL) {
            return;
        }
        const l = _.nodes[idx as usize].left;
        const r = _.nodes[idx as usize].right;
        _.nodes[idx as usize].size = 1 + _.nodeSize(l) + _.nodeSize(r);
    }

    fn rotateRight(*_, y: u32): u32 {
        const x = _.nodes[y as usize].left;
        const t2 = _.nodes[x as usize].right;
        _.nodes[x as usize].right = y;
        _.nodes[y as usize].left = t2;
        _.updateSize(y);
        _.updateSize(x);
        return x;
    }

    fn rotateLeft(*_, x: u32): u32 {
        const y = _.nodes[x as usize].right;
        const t2 = _.nodes[y as usize].left;
        _.nodes[y as usize].left = x;
        _.nodes[x as usize].right = t2;
        _.updateSize(x);
        _.updateSize(y);
        return y;
    }

    fn allocNode(*_, key: i64, value: i64, prio: u64): u32 {
        if (_.free_head == TREAP_NIL) {
            return TREAP_NIL;
        }
        const idx = _.free_head;
        _.free_head = _.nodes[idx as usize].right;
        _.nodes[idx as usize] = TreapNode {
            key: key,
            value: value,
            priority: prio,
            left: TREAP_NIL,
            right: TREAP_NIL,
            size: 1,
        };
        return idx;
    }

    fn freeNode(*_, idx: u32) {
        _.nodes[idx as usize].left = TREAP_NIL;
        _.nodes[idx as usize].right = _.free_head;
        _.nodes[idx as usize].size = 0;
        _.free_head = idx;
    }

    fn insertRec(*_, cur: u32, new_idx: u32): u32 {
        if (cur == TREAP_NIL) {
            return new_idx;
        }
        const k = _.nodes[new_idx as usize].key;
        if (k < _.nodes[cur as usize].key) {
            const new_left = _.insertRec(_.nodes[cur as usize].left, new_idx);
            _.nodes[cur as usize].left = new_left;
            _.updateSize(cur);
            if (_.nodes[new_left as usize].priority > _.nodes[cur as usize].priority) {
                return _.rotateRight(cur);
            }
            return cur;
        } else {
            const new_right = _.insertRec(_.nodes[cur as usize].right, new_idx);
            _.nodes[cur as usize].right = new_right;
            _.updateSize(cur);
            if (_.nodes[new_right as usize].priority > _.nodes[cur as usize].priority) {
                return _.rotateLeft(cur);
            }
            return cur;
        }
    }

    /// Inserts `(key, value)` into the Treap, or updates `value` in-place if `key` already exists.
    /// Returns `false` only if `key` is new and the backing node pool is full.
    fn insert(*_, key: i64, value: i64): bool {
        // Fast path: update existing key without allocating a new node
        let cur = _.root;
        while (cur != TREAP_NIL) {
            const ck = _.nodes[cur as usize].key;
            if (key == ck) {
                _.nodes[cur as usize].value = value;
                return true;
            } else if (key < ck) {
                cur = _.nodes[cur as usize].left;
            } else {
                cur = _.nodes[cur as usize].right;
            }
        }

        if (_.free_head == TREAP_NIL) {
            return false;
        }
        const prio = _.nextPriority();
        const new_idx = _.allocNode(key, value, prio);
        _.root = _.insertRec(_.root, new_idx);
        _.count += 1;
        return true;
    }

    fn mergeSubtrees(*_, a: u32, b: u32): u32 {
        if (a == TREAP_NIL) {
            return b;
        }
        if (b == TREAP_NIL) {
            return a;
        }
        if (_.nodes[a as usize].priority >= _.nodes[b as usize].priority) {
            _.nodes[a as usize].right = _.mergeSubtrees(_.nodes[a as usize].right, b);
            _.updateSize(a);
            return a;
        } else {
            _.nodes[b as usize].left = _.mergeSubtrees(a, _.nodes[b as usize].left);
            _.updateSize(b);
            return b;
        }
    }

    fn removeRec(*_, cur: u32, key: i64, removed: *bool): u32 {
        if (cur == TREAP_NIL) {
            return TREAP_NIL;
        }
        const ck = _.nodes[cur as usize].key;
        if (key < ck) {
            _.nodes[cur as usize].left = _.removeRec(_.nodes[cur as usize].left, key, removed);
            _.updateSize(cur);
            return cur;
        } else if (key > ck) {
            _.nodes[cur as usize].right = _.removeRec(_.nodes[cur as usize].right, key, removed);
            _.updateSize(cur);
            return cur;
        } else {
            const l = _.nodes[cur as usize].left;
            const r = _.nodes[cur as usize].right;
            _.freeNode(cur);
            removed.* = true;
            return _.mergeSubtrees(l, r);
        }
    }

    /// Removes `key` from the Treap and returns its node slot to the free list.
    /// Returns `true` if `key` was present and removed, `false` otherwise.
    fn remove(*_, key: i64): bool {
        let removed = false;
        _.root = _.removeRec(_.root, key, &removed);
        if (removed) {
            _.count -= 1;
        }
        return removed;
    }

    /// Looks up the value associated with `key`, or `null` if not found.
    fn get(key: i64): i64? {
        let cur = _.root;
        while (cur != TREAP_NIL) {
            const ck = _.nodes[cur as usize].key;
            if (key == ck) {
                return _.nodes[cur as usize].value;
            } else if (key < ck) {
                cur = _.nodes[cur as usize].left;
            } else {
                cur = _.nodes[cur as usize].right;
            }
        }
        return null;
    }

    /// Returns `true` if `key` exists in the Treap.
    fn contains(key: i64): bool {
        return _.get(key) != null;
    }

    /// Returns the `k`-th smallest entry (0-based) in O(log n) time, or `null` if `k >= len()`.
    fn kth(k: usize): TreapEntry? {
        if (k >= _.count) {
            return null;
        }
        let rem = k;
        let cur = _.root;
        while (cur != TREAP_NIL) {
            const left = _.nodes[cur as usize].left;
            const left_sz = _.nodeSize(left) as usize;
            if (rem < left_sz) {
                cur = left;
            } else if (rem == left_sz) {
                return TreapEntry {
                    key: _.nodes[cur as usize].key,
                    value: _.nodes[cur as usize].value,
                };
            } else {
                rem = rem - left_sz - 1;
                cur = _.nodes[cur as usize].right;
            }
        }
        return null;
    }

    /// Returns the number of elements with key strictly less than `key` in O(log n) time.
    fn rank(key: i64): usize {
        let res: usize = 0;
        let cur = _.root;
        while (cur != TREAP_NIL) {
            const ck = _.nodes[cur as usize].key;
            if (key <= ck) {
                cur = _.nodes[cur as usize].left;
            } else {
                res += (_.nodeSize(_.nodes[cur as usize].left) as usize) + 1;
                cur = _.nodes[cur as usize].right;
            }
        }
        return res;
    }

    fn countLessOrEqual(key: i64): usize {
        let res: usize = 0;
        let cur = _.root;
        while (cur != TREAP_NIL) {
            const ck = _.nodes[cur as usize].key;
            if (key < ck) {
                cur = _.nodes[cur as usize].left;
            } else {
                res += (_.nodeSize(_.nodes[cur as usize].left) as usize) + 1;
                cur = _.nodes[cur as usize].right;
            }
        }
        return res;
    }

    /// Returns the smallest entry with key `>= key`, or `null` if none exists.
    fn lowerBound(key: i64): TreapEntry? {
        let best = TREAP_NIL;
        let cur = _.root;
        while (cur != TREAP_NIL) {
            const ck = _.nodes[cur as usize].key;
            if (ck >= key) {
                best = cur;
                cur = _.nodes[cur as usize].left;
            } else {
                cur = _.nodes[cur as usize].right;
            }
        }
        if (best == TREAP_NIL) {
            return null;
        }
        return TreapEntry {
            key: _.nodes[best as usize].key,
            value: _.nodes[best as usize].value,
        };
    }

    /// Returns the smallest entry with key `> key`, or `null` if none exists.
    fn upperBound(key: i64): TreapEntry? {
        let best = TREAP_NIL;
        let cur = _.root;
        while (cur != TREAP_NIL) {
            const ck = _.nodes[cur as usize].key;
            if (ck > key) {
                best = cur;
                cur = _.nodes[cur as usize].left;
            } else {
                cur = _.nodes[cur as usize].right;
            }
        }
        if (best == TREAP_NIL) {
            return null;
        }
        return TreapEntry {
            key: _.nodes[best as usize].key,
            value: _.nodes[best as usize].value,
        };
    }

    /// Returns the minimum-key entry in the Treap, or `null` if empty.
    fn minEntry(): TreapEntry? {
        if (_.root == TREAP_NIL) {
            return null;
        }
        let cur = _.root;
        while (_.nodes[cur as usize].left != TREAP_NIL) {
            cur = _.nodes[cur as usize].left;
        }
        return TreapEntry {
            key: _.nodes[cur as usize].key,
            value: _.nodes[cur as usize].value,
        };
    }

    /// Returns the maximum-key entry in the Treap, or `null` if empty.
    fn maxEntry(): TreapEntry? {
        if (_.root == TREAP_NIL) {
            return null;
        }
        let cur = _.root;
        while (_.nodes[cur as usize].right != TREAP_NIL) {
            cur = _.nodes[cur as usize].right;
        }
        return TreapEntry {
            key: _.nodes[cur as usize].key,
            value: _.nodes[cur as usize].value,
        };
    }

    /// Returns the number of elements with key in the closed interval `[lo, hi]` in O(log n) time.
    /// Safe even when `hi == i64::MAX`.
    fn rangeCount(lo: i64, hi: i64): usize {
        if (lo > hi || _.count == 0) {
            return 0;
        }
        return _.countLessOrEqual(hi) - _.rank(lo);
    }

    fn verifySubtree(
        idx: u32,
        has_min: bool,
        min_key: i64,
        has_max: bool,
        max_key: i64,
        visited_count: *usize,
    ): bool {
        if (idx == TREAP_NIL) {
            return true;
        }
        if ((idx as usize) >= _.nodes.len) {
            return false;
        }
        visited_count.* += 1;
        if (visited_count.* > _.nodes.len) {
            return false;
        }

        const node = _.nodes[idx as usize];
        if (has_min && node.key <= min_key) {
            return false;
        }
        if (has_max && node.key >= max_key) {
            return false;
        }

        if (node.left != TREAP_NIL) {
            if ((node.left as usize) >= _.nodes.len) {
                return false;
            }
            if (_.nodes[node.left as usize].priority > node.priority) {
                return false;
            }
        }
        if (node.right != TREAP_NIL) {
            if ((node.right as usize) >= _.nodes.len) {
                return false;
            }
            if (_.nodes[node.right as usize].priority > node.priority) {
                return false;
            }
        }

        if (!_.verifySubtree(node.left, has_min, min_key, true, node.key, visited_count)) {
            return false;
        }
        if (!_.verifySubtree(node.right, true, node.key, has_max, max_key, visited_count)) {
            return false;
        }

        const expected_sz: u32 = (1 as u32) + _.nodeSize(node.left) + _.nodeSize(node.right);
        return node.size == expected_sz;
    }

    /// Verifies all structural invariants:
    /// 1. Strict BST key ordering across all subtrees
    /// 2. Max-heap priority ordering (`parent.priority >= child.priority`)
    /// 3. Exact subtree `size` maintenance
    /// 4. Exact pool conservation (`live_nodes + free_list_nodes == capacity`)
    fn verifyInvariants(): bool {
        let visited: usize = 0;
        if (!_.verifySubtree(_.root, false, 0, false, 0, &visited)) {
            return false;
        }
        if (visited != _.count) {
            return false;
        }
        if ((_.nodeSize(_.root) as usize) != _.count) {
            return false;
        }

        // Verify free-list length and cycle freedom
        let free_count: usize = 0;
        let f = _.free_head;
        while (f != TREAP_NIL) {
            if ((f as usize) >= _.nodes.len) {
                return false;
            }
            free_count += 1;
            if (free_count > _.nodes.len) {
                return false;
            }
            f = _.nodes[f as usize].right;
        }
        return _.count + free_count == _.nodes.len;
    }
}
