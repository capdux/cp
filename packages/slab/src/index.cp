// @cap/slab-alloc — Zero-heap pre-allocated uniform-type Slab with O(1) intrusive free-list.
// Clean-room MIT port of Rust's `slab` crate (1.1B+ downloads).

pub const SLAB_CAP: usize = 128;

pub struct SlabEntryI64 {
    pub occupied: bool,
    pub val: i64,
    pub next_free: usize,
}

/// Pre-allocated O(1) Slab of up to 128 `i64` values (`slab::Slab<i64>`).
pub struct SlabI64 {
    pub entries: [128]SlabEntryI64,
    pub count: usize,
    pub next_free: usize,
    pub watermark: usize,

    pub static fn new(): SlabI64 {
        let empty = SlabEntryI64 { occupied: false, val: 0, next_free: SLAB_CAP };
        let arr: [128]SlabEntryI64 = undefined;
        let i: usize = 0;
        while (i < SLAB_CAP) {
            arr[i] = empty;
            i = i + 1;
        }
        return SlabI64 {
            entries: arr,
            count: 0,
            next_free: SLAB_CAP,
            watermark: 0,
        };
    }

    pub fn len(): usize { return _.count; }
    pub fn capacity(): usize { return SLAB_CAP; }
    pub fn isEmpty(): bool { return _.count == 0; }
    pub fn isFull(): bool { return _.count >= SLAB_CAP; }

    /// Returns the key that will be assigned to the next `insert`, or `null` if full.
    pub fn vacantKey(): usize? {
        if (_.next_free < SLAB_CAP) { return _.next_free; }
        if (_.watermark < SLAB_CAP) { return _.watermark; }
        return null;
    }

    /// Inserts `val` in O(1) and returns its slot key, or `null` if slab is full.
    pub fn insert(*_, val: i64): usize? {
        if (_.next_free < SLAB_CAP) {
            let key = _.next_free;
            _.next_free = _.entries[key].next_free;
            _.entries[key].occupied = true;
            _.entries[key].val = val;
            _.entries[key].next_free = SLAB_CAP;
            _.count = _.count + 1;
            return key;
        }
        if (_.watermark < SLAB_CAP) {
            let key = _.watermark;
            _.watermark = _.watermark + 1;
            _.entries[key].occupied = true;
            _.entries[key].val = val;
            _.entries[key].next_free = SLAB_CAP;
            _.count = _.count + 1;
            return key;
        }
        return null;
    }

    pub fn contains(key: usize): bool {
        if (key >= _.watermark) { return false; }
        return _.entries[key].occupied;
    }

    pub fn get(key: usize): i64? {
        if (key >= _.watermark || !_.entries[key].occupied) { return null; }
        return _.entries[key].val;
    }

    pub fn set(*_, key: usize, val: i64): bool {
        if (key >= _.watermark || !_.entries[key].occupied) { return false; }
        _.entries[key].val = val;
        return true;
    }

    /// Removes and returns the value at `key` in O(1), pushing `key` onto the free-list.
    pub fn remove(*_, key: usize): i64? {
        if (key >= _.watermark || !_.entries[key].occupied) { return null; }
        let out = _.entries[key].val;
        _.entries[key].occupied = false;
        _.entries[key].val = 0;
        _.entries[key].next_free = _.next_free;
        _.next_free = key;
        _.count = _.count - 1;
        return out;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < _.watermark) {
            _.entries[i].occupied = false;
            _.entries[i].val = 0;
            _.entries[i].next_free = SLAB_CAP;
            i = i + 1;
        }
        _.count = 0;
        _.next_free = SLAB_CAP;
        _.watermark = 0;
    }

    /// Finds the next occupied key >= `from_key`, or `null` if none.
    pub fn nextOccupied(from_key: usize): usize? {
        let i = from_key;
        while (i < _.watermark) {
            if (_.entries[i].occupied) { return i; }
            i = i + 1;
        }
        return null;
    }
}
