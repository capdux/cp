// @cap/lru-slab — Zero-heap pre-allocated O(1) intrusive doubly-linked LRU slab
// Matching crates.io #415 lru-slab (358M+ downloads). Clean-room MIT in pure CP.

pub const NONE_IDX: i32 = -1;

pub struct GetResult {
    pub ok: bool,
    pub val: u64,
}

pub struct EvictResult {
    pub idx: i32,
    pub key: u64,
    pub val: u64,
}

pub struct SlabEntry {
    pub occupied: bool,
    pub key: u64,
    pub val: u64,
    pub prev: i32,
    pub next: i32,
}

pub struct LruSlab {
    pub head: i32,      // most recently used
    pub tail: i32,      // least recently used
    pub free_head: i32, // free list head (uses `next`)
    pub count: usize,
    pub capacity: usize,

    pub static fn new(): LruSlab {
        return LruSlab {
            head: NONE_IDX,
            tail: NONE_IDX,
            free_head: NONE_IDX,
            count: 0,
            capacity: 0,
        };
    }

    pub fn init(*_, entries: []SlabEntry): bool {
        let cap = entries.len;
        _.head = NONE_IDX;
        _.tail = NONE_IDX;
        _.count = 0;
        _.capacity = cap;
        if (cap == 0) {
            _.free_head = NONE_IDX;
            return true;
        }
        let i: usize = 0;
        while (i < cap) {
            let nxt: i32 = if (i + 1 < cap) { (i + 1) as i32 } else { NONE_IDX };
            entries[i] = SlabEntry {
                occupied: false,
                key: 0,
                val: 0,
                prev: NONE_IDX,
                next: nxt,
            };
            i = i + 1;
        }
        _.free_head = 0;
        return true;
    }

    pub fn len(): usize { return _.count; }
    pub fn isEmpty(): bool { return _.count == 0; }
    pub fn isFull(): bool { return _.count >= _.capacity; }

    fn unlinkLru(*_, entries: []SlabEntry, idx: i32) {
        let cur = entries[idx as usize];
        let p = cur.prev;
        let n = cur.next;
        if (p != NONE_IDX) {
            let pe = entries[p as usize];
            pe.next = n;
            entries[p as usize] = pe;
        } else {
            _.head = n;
        }
        if (n != NONE_IDX) {
            let ne = entries[n as usize];
            ne.prev = p;
            entries[n as usize] = ne;
        } else {
            _.tail = p;
        }
        cur.prev = NONE_IDX;
        cur.next = NONE_IDX;
        entries[idx as usize] = cur;
    }

    fn pushFrontLru(*_, entries: []SlabEntry, idx: i32) {
        let cur = entries[idx as usize];
        cur.prev = NONE_IDX;
        cur.next = _.head;
        entries[idx as usize] = cur;
        if (_.head != NONE_IDX) {
            let he = entries[_.head as usize];
            he.prev = idx;
            entries[_.head as usize] = he;
        } else {
            _.tail = idx;
        }
        _.head = idx;
    }

    // O(1) insert; returns allocated slab index or NONE_IDX if full.
    pub fn insert(*_, entries: []SlabEntry, key: u64, val: u64): i32 {
        if (_.free_head == NONE_IDX) { return NONE_IDX; }
        let idx = _.free_head;
        let cur = entries[idx as usize];
        _.free_head = cur.next;
        cur.occupied = true;
        cur.key = key;
        cur.val = val;
        entries[idx as usize] = cur;
        _.pushFrontLru(entries, idx);
        _.count = _.count + 1;
        return idx;
    }

    // O(1) touch: promotes entry to MRU head.
    pub fn touch(*_, entries: []SlabEntry, idx: i32): bool {
        if (idx < 0 || (idx as usize) >= _.capacity) { return false; }
        if (!entries[idx as usize].occupied) { return false; }
        if (_.head == idx) { return true; }
        _.unlinkLru(entries, idx);
        _.pushFrontLru(entries, idx);
        return true;
    }

    // O(1) get value & touch.
    pub fn get(*_, entries: []SlabEntry, idx: i32): GetResult {
        if (!_.touch(entries, idx)) { return GetResult { ok: false, val: 0 }; }
        return GetResult { ok: true, val: entries[idx as usize].val };
    }

    // O(1) remove by slab index.
    pub fn remove(*_, entries: []SlabEntry, idx: i32): bool {
        if (idx < 0 || (idx as usize) >= _.capacity) { return false; }
        if (!entries[idx as usize].occupied) { return false; }
        _.unlinkLru(entries, idx);
        let cur = entries[idx as usize];
        cur.occupied = false;
        cur.next = _.free_head;
        entries[idx as usize] = cur;
        _.free_head = idx;
        _.count = _.count - 1;
        return true;
    }

    // O(1) evict least recently used (tail) entry; returns its index or NONE_IDX.
    pub fn evictLru(*_, entries: []SlabEntry): EvictResult {
        if (_.tail == NONE_IDX) { return EvictResult { idx: NONE_IDX, key: 0, val: 0 }; }
        let idx = _.tail;
        let k = entries[idx as usize].key;
        let v = entries[idx as usize].val;
        * = _.remove(entries, idx);
        return EvictResult { idx: idx, key: k, val: v };
    }

    pub fn peekMru(): i32 { return _.head; }
    pub fn peekLru(): i32 { return _.tail; }
}
