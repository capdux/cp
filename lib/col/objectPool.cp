// Source: Absorbed from /data/dev/bun/src/collections/pool.cp | Synced: 2026-11-21T12:28:05Z
// ============================================================================
// Absorbed from Bun: bun/src/collections/object_pool.cp
// Upstream Sync: Refer to bun/src/collections/object_pool.zig for future upstream updates.
// ============================================================================
// lib/col/object_pool.cp
// Fixed-capacity, zero-heap-fragmentation intrusive index free-list ObjectPool<T, CAP>
// Absorbed from Bun src/collections/

pub struct ObjectPool<T, const CAP: usize> {
    items: [CAP]T = undefined;
    next_free: [CAP]usize = undefined;
    in_use: [CAP]bool = undefined;
    free_head: usize = 0;
    used_count: usize = 0;

    pub static fn init(): ObjectPool<T, CAP> {
        let pool = ObjectPool<T, CAP> {
            items: undefined,
            next_free: undefined,
            in_use: undefined,
            free_head: 0,
            used_count: 0,
        };
        let i: usize = 0;
        while (i < CAP) {
            pool.next_free[i] = i + 1;
            pool.in_use[i] = false;
            i += 1;
        }
        return pool;
    }

    pub fn capacity(): usize {
        return _.items.len;
    }

    pub fn usedCount(): usize {
        return _.used_count;
    }

    pub fn availableCount(): usize {
        return CAP - _.used_count;
    }

    pub fn isEmpty(): bool {
        return _.used_count == 0;
    }

    pub fn isFull(): bool {
        return _.used_count >= CAP;
    }

    /// Acquires a free slot in O(1) from the free-list head, stores `val`, and returns the slot index.
    pub fn acquire(*_, val: T): usize? {
        if (_.used_count >= CAP || _.free_head >= CAP) {
            return null;
        }
        const slot = _.free_head;
        _.free_head = _.next_free[slot];
        _.in_use[slot] = true;
        _.items[slot] = val;
        _.used_count += 1;
        return slot;
    }

    /// Reads the value at `slot` if currently active.
    pub fn get(slot: usize): T? {
        if (slot >= CAP || !_.in_use[slot]) {
            return null;
        }
        return _.items[slot];
    }

    /// Updates the value at `slot` if currently active.
    pub fn set(*_, slot: usize, val: T): bool {
        if (slot >= CAP || !_.in_use[slot]) {
            return false;
        }
        _.items[slot] = val;
        return true;
    }

    /// Releases an active `slot` back to the front of the free list in O(1).
    pub fn release(*_, slot: usize): bool {
        if (slot >= CAP || !_.in_use[slot]) {
            return false;
        }
        _.in_use[slot] = false;
        _.next_free[slot] = _.free_head;
        _.free_head = slot;
        _.used_count -= 1;
        return true;
    }

    /// Alias of `release` that avoids ARC `.release()` method interception in codegen.
    pub fn releaseSlot(*_, slot: usize): bool {
        if (slot >= CAP || !_.in_use[slot]) {
            return false;
        }
        _.in_use[slot] = false;
        _.next_free[slot] = _.free_head;
        _.free_head = slot;
        _.used_count -= 1;
        return true;
    }

    /// Resets the entire pool and rebuilds the sequential free list in O(CAP).
    pub fn reset(*_) {
        let i: usize = 0;
        while (i < CAP) {
            _.next_free[i] = i + 1;
            _.in_use[i] = false;
            i += 1;
        }
        _.free_head = 0;
        _.used_count = 0;
    }
}
