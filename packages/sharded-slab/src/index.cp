// @cap/slab — Zero-allocation O(1) generational slab allocator & ABA-safe handle pool for CP
// Clean-room MIT implementation inspired by Rust `slab` (1.19B downloads) and generational-arena.

const SLAB_CAPACITY: usize = 256;
const NO_FREE: i32 = -1;

pub struct SlabKey {
    pub index: u32,
    pub generation: u32,

    pub fn equals(other: SlabKey) -> bool {
        return _.index == other.index && _.generation == other.generation;
    }
}

fn nextGeneration(gen: u32) -> u32 {
    if (gen >= 0xFFFFFFFE) {
        return 1 as u32;
    }
    return gen + (1 as u32);
}

pub struct SlabI64 {
    pub slots: [SLAB_CAPACITY]i64,
    pub occupied: [SLAB_CAPACITY]bool,
    pub generations: [SLAB_CAPACITY]u32,
    pub next_free: [SLAB_CAPACITY]i32,
    pub free_head: i32,
    pub next_alloc: usize,
    pub len: usize,
    pub capacity: usize,

    pub static fn init() -> SlabI64 {
        let slots: [SLAB_CAPACITY]i64 = undefined;
        let occupied: [SLAB_CAPACITY]bool = undefined;
        let generations: [SLAB_CAPACITY]u32 = undefined;
        let next_free: [SLAB_CAPACITY]i32 = undefined;
        let i: usize = 0;
        while (i < SLAB_CAPACITY) {
            slots[i] = 0 as i64;
            occupied[i] = false;
            generations[i] = 1 as u32;
            next_free[i] = NO_FREE;
            i += 1;
        }
        return SlabI64{
            slots: slots,
            occupied: occupied,
            generations: generations,
            next_free: next_free,
            free_head: NO_FREE,
            next_alloc: 0 as usize,
            len: 0 as usize,
            capacity: SLAB_CAPACITY,
        };
    }

    pub fn insert(*_, value: i64) -> SlabKey? {
        let idx: usize = 0;
        if (_.free_head >= 0) {
            idx = _.free_head as usize;
            _.free_head = _.next_free[idx];
            _.next_free[idx] = NO_FREE;
        } else if (_.next_alloc < _.capacity) {
            idx = _.next_alloc;
            _.next_alloc += 1;
        } else {
            return null;
        }
        _.slots[idx] = value;
        _.occupied[idx] = true;
        _.len += 1;
        return SlabKey{
            index: idx as u32,
            generation: _.generations[idx],
        };
    }

    pub fn contains(key: SlabKey) -> bool {
        const idx = key.index as usize;
        if (idx >= _.capacity) {
            return false;
        }
        return _.occupied[idx] && _.generations[idx] == key.generation;
    }

    pub fn get(key: SlabKey) -> i64? {
        const idx = key.index as usize;
        if (idx >= _.capacity) {
            return null;
        }
        if (!_.occupied[idx] || _.generations[idx] != key.generation) {
            return null;
        }
        return _.slots[idx];
    }

    pub fn set(*_, key: SlabKey, value: i64) -> bool {
        const idx = key.index as usize;
        if (idx >= _.capacity) {
            return false;
        }
        if (!_.occupied[idx] || _.generations[idx] != key.generation) {
            return false;
        }
        _.slots[idx] = value;
        return true;
    }

    pub fn remove(*_, key: SlabKey) -> i64? {
        const idx = key.index as usize;
        if (idx >= _.capacity) {
            return null;
        }
        if (!_.occupied[idx] || _.generations[idx] != key.generation) {
            return null;
        }
        const val = _.slots[idx];
        _.occupied[idx] = false;
        _.generations[idx] = nextGeneration(_.generations[idx]);
        _.next_free[idx] = _.free_head;
        _.free_head = idx as i32;
        _.len -= 1;
        return val;
    }

    pub fn count() -> usize {
        return _.len;
    }

    pub fn isEmpty() -> bool {
        return _.len == 0;
    }

    pub fn isFull() -> bool {
        return _.len >= _.capacity;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < _.next_alloc) {
            if (_.occupied[i]) {
                _.occupied[i] = false;
                _.generations[i] = nextGeneration(_.generations[i]);
            }
            _.next_free[i] = NO_FREE;
            i += 1;
        }
        _.free_head = NO_FREE;
        _.next_alloc = 0 as usize;
        _.len = 0 as usize;
    }
}

pub struct SlabU64 {
    pub slots: [SLAB_CAPACITY]u64,
    pub occupied: [SLAB_CAPACITY]bool,
    pub generations: [SLAB_CAPACITY]u32,
    pub next_free: [SLAB_CAPACITY]i32,
    pub free_head: i32,
    pub next_alloc: usize,
    pub len: usize,
    pub capacity: usize,

    pub static fn init() -> SlabU64 {
        let slots: [SLAB_CAPACITY]u64 = undefined;
        let occupied: [SLAB_CAPACITY]bool = undefined;
        let generations: [SLAB_CAPACITY]u32 = undefined;
        let next_free: [SLAB_CAPACITY]i32 = undefined;
        let i: usize = 0;
        while (i < SLAB_CAPACITY) {
            slots[i] = 0 as u64;
            occupied[i] = false;
            generations[i] = 1 as u32;
            next_free[i] = NO_FREE;
            i += 1;
        }
        return SlabU64{
            slots: slots,
            occupied: occupied,
            generations: generations,
            next_free: next_free,
            free_head: NO_FREE,
            next_alloc: 0 as usize,
            len: 0 as usize,
            capacity: SLAB_CAPACITY,
        };
    }

    pub fn insert(*_, value: u64) -> SlabKey? {
        let idx: usize = 0;
        if (_.free_head >= 0) {
            idx = _.free_head as usize;
            _.free_head = _.next_free[idx];
            _.next_free[idx] = NO_FREE;
        } else if (_.next_alloc < _.capacity) {
            idx = _.next_alloc;
            _.next_alloc += 1;
        } else {
            return null;
        }
        _.slots[idx] = value;
        _.occupied[idx] = true;
        _.len += 1;
        return SlabKey{
            index: idx as u32,
            generation: _.generations[idx],
        };
    }

    pub fn contains(key: SlabKey) -> bool {
        const idx = key.index as usize;
        if (idx >= _.capacity) {
            return false;
        }
        return _.occupied[idx] && _.generations[idx] == key.generation;
    }

    pub fn get(key: SlabKey) -> u64? {
        const idx = key.index as usize;
        if (idx >= _.capacity) {
            return null;
        }
        if (!_.occupied[idx] || _.generations[idx] != key.generation) {
            return null;
        }
        return _.slots[idx];
    }

    pub fn set(*_, key: SlabKey, value: u64) -> bool {
        const idx = key.index as usize;
        if (idx >= _.capacity) {
            return false;
        }
        if (!_.occupied[idx] || _.generations[idx] != key.generation) {
            return false;
        }
        _.slots[idx] = value;
        return true;
    }

    pub fn remove(*_, key: SlabKey) -> u64? {
        const idx = key.index as usize;
        if (idx >= _.capacity) {
            return null;
        }
        if (!_.occupied[idx] || _.generations[idx] != key.generation) {
            return null;
        }
        const val = _.slots[idx];
        _.occupied[idx] = false;
        _.generations[idx] = nextGeneration(_.generations[idx]);
        _.next_free[idx] = _.free_head;
        _.free_head = idx as i32;
        _.len -= 1;
        return val;
    }

    pub fn count() -> usize {
        return _.len;
    }

    pub fn isEmpty() -> bool {
        return _.len == 0;
    }

    pub fn isFull() -> bool {
        return _.len >= _.capacity;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < _.next_alloc) {
            if (_.occupied[i]) {
                _.occupied[i] = false;
                _.generations[i] = nextGeneration(_.generations[i]);
            }
            _.next_free[i] = NO_FREE;
            i += 1;
        }
        _.free_head = NO_FREE;
        _.next_alloc = 0 as usize;
        _.len = 0 as usize;
    }
}

export {
    SlabKey,
    SlabI64,
    SlabU64,
};
