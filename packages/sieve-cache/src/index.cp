// @cap/sieve-cache — Zero-allocation NSDI'24 SIEVE cache eviction engine for CP
// Clean-room MIT implementation inspired by the NSDI'24 SIEVE paper and Rust `sieve-cache`.

pub const MAX_CAP: usize = 128;
const HASH_CAP: usize = 256;
const HASH_MASK: usize = 255;

const NONE_IDX: u16 = 0xFFFF;
const HT_EMPTY: u16 = 0xFFFF;
const HT_TOMB: u16 = 0xFFFE;

const HASH_MIX: u64 = ((0x9e3779b9 as u64) << 32) | (0x7f4a7c15 as u64);

struct SieveNode {
    key: u64,
    value: i64,
    visited: bool,
    in_use: bool,
    prev: u16,
    next: u16,
}

fn emptyNode() -> SieveNode {
    return SieveNode{
        key: 0,
        value: 0,
        visited: false,
        in_use: false,
        prev: NONE_IDX,
        next: NONE_IDX,
    };
}

fn mulWrap64(a: u64, b: u64) -> u64 {
    const a_lo = a & 0xFFFFFFFF;
    const a_hi = a >> 32;
    const b_lo = b & 0xFFFFFFFF;
    const b_hi = b >> 32;

    const p0 = a_lo * b_lo;
    const p1 = (a_lo * b_hi) & 0xFFFFFFFF;
    const p2 = (a_hi * b_lo) & 0xFFFFFFFF;
    const hi = ((p0 >> 32) + p1 + p2) & 0xFFFFFFFF;
    return (p0 & 0xFFFFFFFF) | (hi << 32);
}

fn hashSlot(key: u64) -> usize {
    const h = mulWrap64(key ^ (key >> 33), HASH_MIX);
    return ((h >> 32) as usize) & HASH_MASK;
}

pub struct SieveCache {
    nodes: [128]SieveNode,
    buckets: [256]u16,
    free_stack: [128]u16,
    free_top: usize,
    head: u16,
    tail: u16,
    hand: u16,
    cap: usize,
    count: usize,
    tomb_count: usize,
    hit_count: u64,
    miss_count: u64,
    eviction_count: u64,

    /// Initializes a zero-allocation `SieveCache` with `max_items` clamped to `1..=128`.
    pub static fn init(max_items: usize) -> SieveCache {
        let cap = max_items;
        if (cap < 1) {
            cap = 1;
        }
        if (cap > MAX_CAP) {
            cap = MAX_CAP;
        }

        let nodes: [128]SieveNode = undefined;
        let free_stack: [128]u16 = undefined;
        let i: usize = 0;
        while (i < MAX_CAP) {
            nodes[i] = emptyNode();
            free_stack[i] = (MAX_CAP - 1 - i) as u16;
            i += 1;
        }

        let buckets: [256]u16 = undefined;
        let b: usize = 0;
        while (b < HASH_CAP) {
            buckets[b] = HT_EMPTY;
            b += 1;
        }

        return SieveCache{
            nodes: nodes,
            buckets: buckets,
            free_stack: free_stack,
            free_top: MAX_CAP,
            head: NONE_IDX,
            tail: NONE_IDX,
            hand: NONE_IDX,
            cap: cap,
            count: 0,
            tomb_count: 0,
            hit_count: 0,
            miss_count: 0,
            eviction_count: 0,
        };
    }

    pub fn len() -> usize {
        return _.count;
    }

    pub fn capacity() -> usize {
        return _.cap;
    }

    pub fn isEmpty() -> bool {
        return _.count == 0;
    }

    pub fn hits() -> u64 {
        return _.hit_count;
    }

    pub fn misses() -> u64 {
        return _.miss_count;
    }

    pub fn evictions() -> u64 {
        return _.eviction_count;
    }

    fn findNodeIdx(key: u64) -> u16 {
        let slot = hashSlot(key);
        let probe: usize = 0;
        while (probe < HASH_CAP) {
            const entry = _.buckets[slot];
            if (entry == HT_EMPTY) {
                return NONE_IDX;
            }
            if (entry != HT_TOMB) {
                if (_.nodes[entry as usize].key == key) {
                    return entry;
                }
            }
            slot = (slot + 1) & HASH_MASK;
            probe += 1;
        }
        return NONE_IDX;
    }

    fn htInsert(*_, key: u64, node_idx: u16) {
        let slot = hashSlot(key);
        let probe: usize = 0;
        while (probe < HASH_CAP) {
            const entry = _.buckets[slot];
            if (entry == HT_EMPTY || entry == HT_TOMB) {
                if (entry == HT_TOMB && _.tomb_count > 0) {
                    _.tomb_count -= 1;
                }
                _.buckets[slot] = node_idx;
                return;
            }
            slot = (slot + 1) & HASH_MASK;
            probe += 1;
        }
    }

    fn htRemove(*_, key: u64) {
        let slot = hashSlot(key);
        let probe: usize = 0;
        while (probe < HASH_CAP) {
            const entry = _.buckets[slot];
            if (entry == HT_EMPTY) {
                return;
            }
            if (entry != HT_TOMB && _.nodes[entry as usize].key == key) {
                _.buckets[slot] = HT_TOMB;
                _.tomb_count += 1;
                if (_.tomb_count > 64) {
                    _.rehash();
                }
                return;
            }
            slot = (slot + 1) & HASH_MASK;
            probe += 1;
        }
    }

    fn rehash(*_) {
        let b: usize = 0;
        while (b < HASH_CAP) {
            _.buckets[b] = HT_EMPTY;
            b += 1;
        }
        _.tomb_count = 0;
        let cur = _.head;
        while (cur != NONE_IDX) {
            _.htInsert(_.nodes[cur as usize].key, cur);
            cur = _.nodes[cur as usize].next;
        }
    }

    fn unlinkNode(*_, idx: u16) {
        const p = _.nodes[idx as usize].prev;
        const n = _.nodes[idx as usize].next;
        if (p != NONE_IDX) {
            _.nodes[p as usize].next = n;
        } else {
            _.head = n;
        }
        if (n != NONE_IDX) {
            _.nodes[n as usize].prev = p;
        } else {
            _.tail = p;
        }
        _.nodes[idx as usize].prev = NONE_IDX;
        _.nodes[idx as usize].next = NONE_IDX;
    }

    fn linkAtHead(*_, idx: u16) {
        _.nodes[idx as usize].prev = NONE_IDX;
        _.nodes[idx as usize].next = _.head;
        if (_.head != NONE_IDX) {
            _.nodes[_.head as usize].prev = idx;
        } else {
            _.tail = idx;
        }
        _.head = idx;
    }

    fn evictOne(*_) {
        if (_.count == 0 || _.tail == NONE_IDX) {
            return;
        }
        let o = _.hand;
        if (o == NONE_IDX) {
            o = _.tail;
        }
        while (_.nodes[o as usize].visited) {
            _.nodes[o as usize].visited = false;
            const prev_idx = _.nodes[o as usize].prev;
            if (prev_idx != NONE_IDX) {
                o = prev_idx;
            } else {
                o = _.tail;
            }
        }
        // Advance hand to predecessor of evicted node (NONE_IDX wraps to tail on next eviction)
        _.hand = _.nodes[o as usize].prev;
        const evicted_key = _.nodes[o as usize].key;
        _.htRemove(evicted_key);
        _.unlinkNode(o);
        _.nodes[o as usize].in_use = false;
        _.free_stack[_.free_top] = o;
        _.free_top += 1;
        _.count -= 1;
        _.eviction_count += 1;
    }

    /// Read-only membership check without modifying `visited` or stats.
    pub fn contains(key: u64) -> bool {
        return _.findNodeIdx(key) != NONE_IDX;
    }

    /// Read-only lookup without modifying `visited` or hit/miss stats. Returns `-1` if absent.
    pub fn peek(key: u64) -> i64 {
        const idx = _.findNodeIdx(key);
        if (idx == NONE_IDX) {
            return -1;
        }
        return _.nodes[idx as usize].value;
    }

    /// Looks up `key`: marks `visited = true` on hit (without moving the node!) and updates stats.
    /// Returns `-1` on miss.
    pub fn get(*_, key: u64) -> i64 {
        const idx = _.findNodeIdx(key);
        if (idx == NONE_IDX) {
            _.miss_count += 1;
            return -1;
        }
        _.nodes[idx as usize].visited = true;
        _.hit_count += 1;
        return _.nodes[idx as usize].value;
    }

    /// Inserts or updates `(key, value)` following SIEVE eviction semantics.
    pub fn put(*_, key: u64, value: i64) -> bool {
        const existing = _.findNodeIdx(key);
        if (existing != NONE_IDX) {
            _.nodes[existing as usize].value = value;
            _.nodes[existing as usize].visited = true;
            return true;
        }

        if (_.count >= _.cap) {
            _.evictOne();
        }
        if (_.free_top == 0) {
            return false;
        }

        _.free_top -= 1;
        const idx = _.free_stack[_.free_top];
        _.nodes[idx as usize] = SieveNode{
            key: key,
            value: value,
            visited: false,
            in_use: true,
            prev: NONE_IDX,
            next: NONE_IDX,
        };
        _.linkAtHead(idx);
        _.htInsert(key, idx);
        _.count += 1;
        return true;
    }

    /// Removes `key` from the cache, safely preserving `hand`, `head`, and `tail` pointers.
    pub fn remove(*_, key: u64) -> bool {
        const idx = _.findNodeIdx(key);
        if (idx == NONE_IDX) {
            return false;
        }
        if (_.hand == idx) {
            _.hand = _.nodes[idx as usize].prev;
        }
        _.htRemove(key);
        _.unlinkNode(idx);
        _.nodes[idx as usize].in_use = false;
        _.free_stack[_.free_top] = idx;
        _.free_top += 1;
        _.count -= 1;
        return true;
    }

    /// Clears all entries and resets hit/miss/eviction statistics.
    pub fn clear(*_) {
        let i: usize = 0;
        while (i < MAX_CAP) {
            _.nodes[i] = emptyNode();
            _.free_stack[i] = (MAX_CAP - 1 - i) as u16;
            i += 1;
        }
        let b: usize = 0;
        while (b < HASH_CAP) {
            _.buckets[b] = HT_EMPTY;
            b += 1;
        }
        _.free_top = MAX_CAP;
        _.head = NONE_IDX;
        _.tail = NONE_IDX;
        _.hand = NONE_IDX;
        _.count = 0;
        _.tomb_count = 0;
        _.hit_count = 0;
        _.miss_count = 0;
        _.eviction_count = 0;
    }
}
