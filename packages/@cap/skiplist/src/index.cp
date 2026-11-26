// @cap/skiplist — Zero-allocation static-pool deterministic skip list for CP
// Clean-room MIT implementation inspired by Rust `skiplist` and `crossbeam-skiplist`.

pub const MAX_NODES: usize = 128;
pub const MAX_LEVEL: usize = 8;

const POOL_SIZE: usize = 129; // index 0 = head sentinel, indices 1..=128 = data nodes
const NONE_IDX: u16 = 0xFFFF;
const DEFAULT_SEED: u64 = ((0x243F6A88 as u64) << 32) | (0x85A308D3 as u64);

pub struct SkipEntry {
    pub key: i64,
    pub value: i64,
    pub found: bool,

    pub static fn none() -> SkipEntry {
        return SkipEntry{
            key: 0,
            value: 0,
            found: false,
        };
    }
}

struct SkipNode {
    key: i64,
    value: i64,
    level: u8,
    next: [8]u16,
}

fn emptyNode() -> SkipNode {
    return SkipNode{
        key: 0,
        value: 0,
        level: 0,
        next: [NONE_IDX, NONE_IDX, NONE_IDX, NONE_IDX, NONE_IDX, NONE_IDX, NONE_IDX, NONE_IDX],
    };
}

pub struct SkipList {
    nodes: [129]SkipNode,
    free_stack: [128]u16,
    free_top: usize,
    cur_level: u8,
    count: usize,
    rng_state: u64,

    /// Initializes an empty deterministic skip list with 128 reusable node slots.
    pub static fn init() -> SkipList {
        let nodes: [129]SkipNode = undefined;
        let i: usize = 0;
        while (i < POOL_SIZE) {
            nodes[i] = emptyNode();
            i += 1;
        }
        let free_stack: [128]u16 = undefined;
        let f: usize = 0;
        while (f < MAX_NODES) {
            free_stack[f] = (MAX_NODES - f) as u16; // indices 1..=128
            f += 1;
        }
        return SkipList{
            nodes: nodes,
            free_stack: free_stack,
            free_top: MAX_NODES,
            cur_level: 1,
            count: 0,
            rng_state: DEFAULT_SEED,
        };
    }

    pub fn len() -> usize {
        return _.count;
    }

    pub fn isEmpty() -> bool {
        return _.count == 0;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < POOL_SIZE) {
            _.nodes[i] = emptyNode();
            i += 1;
        }
        let f: usize = 0;
        while (f < MAX_NODES) {
            _.free_stack[f] = (MAX_NODES - f) as u16;
            f += 1;
        }
        _.free_top = MAX_NODES;
        _.cur_level = 1;
        _.count = 0;
        _.rng_state = DEFAULT_SEED;
    }

    fn randomLevel(*_) -> u8 {
        let x = _.rng_state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.rng_state = x;

        let lvl: u8 = 1;
        let bits = x;
        while ((bits & 1) != 0 && (lvl as usize) < MAX_LEVEL) {
            lvl += 1;
            bits >>= 1;
        }
        return lvl;
    }

    /// Inserts `(key, value)` or updates `value` in-place if `key` already exists.
    /// Returns `false` if the 128-node static pool is full.
    pub fn insert(*_, key: i64, value: i64) -> bool {
        let update: [8]u16 = [0, 0, 0, 0, 0, 0, 0, 0];
        let cur: u16 = 0;
        let lvl = _.cur_level as usize;
        while (lvl > 0) {
            lvl -= 1;
            while (true) {
                const nxt = _.nodes[cur as usize].next[lvl];
                if (nxt == NONE_IDX || _.nodes[nxt as usize].key >= key) {
                    break;
                }
                cur = nxt;
            }
            update[lvl] = cur;
        }

        const cand = _.nodes[update[0] as usize].next[0];
        if (cand != NONE_IDX && _.nodes[cand as usize].key == key) {
            _.nodes[cand as usize].value = value;
            return true;
        }

        if (_.free_top == 0) {
            return false;
        }
        _.free_top -= 1;
        const new_idx = _.free_stack[_.free_top];
        const new_lvl = _.randomLevel();
        if (new_lvl > _.cur_level) {
            let l = _.cur_level as usize;
            while (l < (new_lvl as usize)) {
                update[l] = 0;
                l += 1;
            }
            _.cur_level = new_lvl;
        }

        let node = emptyNode();
        node.key = key;
        node.value = value;
        node.level = new_lvl;
        _.nodes[new_idx as usize] = node;

        let l: usize = 0;
        while (l < (new_lvl as usize)) {
            _.nodes[new_idx as usize].next[l] = _.nodes[update[l] as usize].next[l];
            _.nodes[update[l] as usize].next[l] = new_idx;
            l += 1;
        }

        _.count += 1;
        return true;
    }

    fn findNodeGE(key: i64) -> u16 {
        let cur: u16 = 0;
        let lvl = _.cur_level as usize;
        while (lvl > 0) {
            lvl -= 1;
            while (true) {
                const nxt = _.nodes[cur as usize].next[lvl];
                if (nxt == NONE_IDX || _.nodes[nxt as usize].key >= key) {
                    break;
                }
                cur = nxt;
            }
        }
        return _.nodes[cur as usize].next[0];
    }

    /// Looks up exact `key` in O(log N) time.
    pub fn get(key: i64) -> SkipEntry {
        const idx = _.findNodeGE(key);
        if (idx != NONE_IDX && _.nodes[idx as usize].key == key) {
            return SkipEntry{
                key: _.nodes[idx as usize].key,
                value: _.nodes[idx as usize].value,
                found: true,
            };
        }
        return SkipEntry.none();
    }

    pub fn contains(key: i64) -> bool {
        return _.get(key).found;
    }

    /// Removes `key` and returns its node slot to the freelist for immediate reuse.
    pub fn remove(*_, key: i64) -> bool {
        let update: [8]u16 = [0, 0, 0, 0, 0, 0, 0, 0];
        let cur: u16 = 0;
        let lvl = _.cur_level as usize;
        while (lvl > 0) {
            lvl -= 1;
            while (true) {
                const nxt = _.nodes[cur as usize].next[lvl];
                if (nxt == NONE_IDX || _.nodes[nxt as usize].key >= key) {
                    break;
                }
                cur = nxt;
            }
            update[lvl] = cur;
        }

        const target = _.nodes[update[0] as usize].next[0];
        if (target == NONE_IDX || _.nodes[target as usize].key != key) {
            return false;
        }

        let l: usize = 0;
        while (l < (_.cur_level as usize)) {
            if (_.nodes[update[l] as usize].next[l] != target) {
                break;
            }
            _.nodes[update[l] as usize].next[l] = _.nodes[target as usize].next[l];
            l += 1;
        }

        _.nodes[target as usize] = emptyNode();
        _.free_stack[_.free_top] = target;
        _.free_top += 1;
        _.count -= 1;

        while (_.cur_level > 1 && _.nodes[0].next[(_.cur_level as usize) - 1] == NONE_IDX) {
            _.cur_level -= 1;
        }
        return true;
    }

    /// Returns the smallest-key entry in O(1) time.
    pub fn first() -> SkipEntry {
        const idx = _.nodes[0].next[0];
        if (idx == NONE_IDX) {
            return SkipEntry.none();
        }
        return SkipEntry{
            key: _.nodes[idx as usize].key,
            value: _.nodes[idx as usize].value,
            found: true,
        };
    }

    /// Returns the largest-key entry in O(log N) time.
    pub fn last() -> SkipEntry {
        if (_.count == 0) {
            return SkipEntry.none();
        }
        let cur: u16 = 0;
        let lvl = _.cur_level as usize;
        while (lvl > 0) {
            lvl -= 1;
            while (_.nodes[cur as usize].next[lvl] != NONE_IDX) {
                cur = _.nodes[cur as usize].next[lvl];
            }
        }
        if (cur == 0) {
            return SkipEntry.none();
        }
        return SkipEntry{
            key: _.nodes[cur as usize].key,
            value: _.nodes[cur as usize].value,
            found: true,
        };
    }

    /// Returns the first entry with `entry.key >= key` in O(log N) time.
    pub fn lowerBound(key: i64) -> SkipEntry {
        const idx = _.findNodeGE(key);
        if (idx == NONE_IDX) {
            return SkipEntry.none();
        }
        return SkipEntry{
            key: _.nodes[idx as usize].key,
            value: _.nodes[idx as usize].value,
            found: true,
        };
    }

    /// Counts entries whose keys fall within the inclusive interval `[min_key, max_key]`.
    pub fn rangeCount(min_key: i64, max_key: i64) -> usize {
        if (min_key > max_key || _.count == 0) {
            return 0;
        }
        let idx = _.findNodeGE(min_key);
        let total: usize = 0;
        while (idx != NONE_IDX && _.nodes[idx as usize].key <= max_key) {
            total += 1;
            idx = _.nodes[idx as usize].next[0];
        }
        return total;
    }
}
