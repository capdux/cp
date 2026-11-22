// Source: Absorbed from /data/dev/bun/src/collections/ | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/col/treap.cp — 零堆碎片平衡树堆 (Treap: Randomized Binary Search Tree)
// 兼具二叉搜索树 (BST 按 Key 有序) 与最大堆 (按 SplitMix64 伪随机优先级平衡) 特性
// ============================================================================

pub struct TreapNode<K, V> {
    pub key: K = undefined;
    pub value: V = undefined;
    pub priority: u64 = 0;
    pub left: usize = 0;   // 1-based index (0 represents null child)
    pub right: usize = 0;  // 1-based index (0 represents null child)
}

pub struct Treap<K, V> {
    nodes: [256]TreapNode<K, V> = undefined;
    free_list: [256]usize = undefined;
    free_len: usize = 0;
    allocated: usize = 0;
    root: usize = 0;
    count: usize = 0;
    rng_state: u64 = 0x517CC1B727220A95;

    pub static fn init(): Treap<K, V> {
        return Treap<K, V> {
            nodes: undefined,
            free_list: undefined,
            free_len: 0,
            allocated: 0,
            root: 0,
            count: 0,
            rng_state: 0x517CC1B727220A95,
        };
    }

    pub fn len(*_): usize {
        return _.count;
    }

    pub fn isEmpty(*_): bool {
        return _.count == 0;
    }

    pub fn capacity(*_): usize {
        return 256;
    }

    pub fn clear(*_) {
        _.free_len = 0;
        _.allocated = 0;
        _.root = 0;
        _.count = 0;
    }

    pub fn deinit(*_) {
        _.clear();
    }

    fn nextPriority(*_): u64 {
        // XorShift64 deterministic high-entropy priority generator
        let x = _.rng_state;
        x = x ^ (x << 13);
        x = x ^ (x >> 7);
        x = x ^ (x << 17);
        _.rng_state = x;
        return x;
    }

    fn allocNode(*_, key: K, value: V): usize {
        let idx1: usize = 0;
        if (_.free_len > 0) {
            _.free_len -= 1;
            idx1 = _.free_list[_.free_len];
        } else {
            if (_.allocated >= 256) {
                return 0;
            }
            _.allocated += 1;
            idx1 = _.allocated;
        }
        const prio = _.nextPriority();
        _.nodes[idx1 - 1] = TreapNode<K, V> {
            key: key,
            value: value,
            priority: prio,
            left: 0,
            right: 0,
        };
        return idx1;
    }

    fn freeNode(*_, idx1: usize) {
        if (idx1 > 0 && _.free_len < 256) {
            _.free_list[_.free_len] = idx1;
            _.free_len += 1;
        }
    }

    fn rotateRight(*_, y: usize): usize {
        const x = _.nodes[y - 1].left;
        const t2 = _.nodes[x - 1].right;
        _.nodes[x - 1].right = y;
        _.nodes[y - 1].left = t2;
        return x;
    }

    fn rotateLeft(*_, x: usize): usize {
        const y = _.nodes[x - 1].right;
        const t2 = _.nodes[y - 1].left;
        _.nodes[y - 1].left = x;
        _.nodes[x - 1].right = t2;
        return y;
    }

    fn insertNode(*_, curr: usize, key: K, value: V): usize {
        if (curr == 0) {
            const created = _.allocNode(key, value);
            if (created != 0) {
                _.count += 1;
            }
            return created;
        }

        const curr_key = _.nodes[curr - 1].key;
        if (key == curr_key) {
            _.nodes[curr - 1].value = value;
            return curr;
        } else if (key < curr_key) {
            const left_child = _.insertNode(_.nodes[curr - 1].left, key, value);
            _.nodes[curr - 1].left = left_child;
            if (left_child != 0 && _.nodes[left_child - 1].priority > _.nodes[curr - 1].priority) {
                return _.rotateRight(curr);
            }
        } else {
            const right_child = _.insertNode(_.nodes[curr - 1].right, key, value);
            _.nodes[curr - 1].right = right_child;
            if (right_child != 0 && _.nodes[right_child - 1].priority > _.nodes[curr - 1].priority) {
                return _.rotateLeft(curr);
            }
        }
        return curr;
    }

    /// 插入或更新键值对，平均时间复杂度 O(log N)
    pub fn insert(*_, key: K, value: V) {
        _.root = _.insertNode(_.root, key, value);
    }

    /// 查找键对应的值，平均时间复杂度 O(log N)
    pub fn get(*_, key: K): V? {
        let curr = _.root;
        while (curr != 0) {
            const node = _.nodes[curr - 1];
            if (key == node.key) {
                return node.value;
            } else if (key < node.key) {
                curr = node.left;
            } else {
                curr = node.right;
            }
        }
        return null;
    }

    /// 判断指定键是否存在
    pub fn contains(*_, key: K): bool {
        return _.get(key) != null;
    }

    fn removeNode(*_, curr: usize, key: K): usize {
        if (curr == 0) {
            return 0;
        }
        const curr_key = _.nodes[curr - 1].key;
        if (key < curr_key) {
            _.nodes[curr - 1].left = _.removeNode(_.nodes[curr - 1].left, key);
            return curr;
        } else if (key > curr_key) {
            _.nodes[curr - 1].right = _.removeNode(_.nodes[curr - 1].right, key);
            return curr;
        }

        // Found target node
        const left = _.nodes[curr - 1].left;
        const right = _.nodes[curr - 1].right;
        if (left == 0) {
            _.count -= 1;
            _.freeNode(curr);
            return right;
        } else if (right == 0) {
            _.count -= 1;
            _.freeNode(curr);
            return left;
        } else {
            if (_.nodes[left - 1].priority > _.nodes[right - 1].priority) {
                const new_root = _.rotateRight(curr);
                _.nodes[new_root - 1].right = _.removeNode(_.nodes[new_root - 1].right, key);
                return new_root;
            } else {
                const new_root = _.rotateLeft(curr);
                _.nodes[new_root - 1].left = _.removeNode(_.nodes[new_root - 1].left, key);
                return new_root;
            }
        }
    }

    /// 删除指定键，若存在并成功移除返回 true，平均时间复杂度 O(log N)
    pub fn remove(*_, key: K): bool {
        const before = _.count;
        _.root = _.removeNode(_.root, key);
        return _.count < before;
    }

    /// 获取树中最小键 (BST 最左节点)
    pub fn minKey(*_): K? {
        if (_.root == 0) {
            return null;
        }
        let curr = _.root;
        while (_.nodes[curr - 1].left != 0) {
            curr = _.nodes[curr - 1].left;
        }
        return _.nodes[curr - 1].key;
    }

    /// 获取树中最大键 (BST 最右节点)
    pub fn maxKey(*_): K? {
        if (_.root == 0) {
            return null;
        }
        let curr = _.root;
        while (_.nodes[curr - 1].right != 0) {
            curr = _.nodes[curr - 1].right;
        }
        return _.nodes[curr - 1].key;
    }
}
