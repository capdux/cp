// @cap/lru-crate — Zero-heap O(1) LRU cache (`LruCacheI64`, up to 64 entries).
// Clean-room MIT port of Rust's `lru` crate (900M+ downloads); reuses `@cap/fnv-hash`.

import { fnv1a64 } from "../../fnv-hash/src/index.cp";

pub const LRU_MAX_CAP: usize = 64;
pub const LRU_NIL: usize = 255;

pub struct LruNode {
    pub occupied: bool,
    pub key: string,
    pub val: i64,
    pub hash: u64,
    pub prev: usize,
    pub next: usize,
}

fn lruStrEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

/// Fixed-capacity O(1) LRU cache mapping `string -> i64` (`lru::LruCache`).
pub struct LruCacheI64 {
    pub nodes: [64]LruNode,
    pub cap: usize,
    pub count: usize,
    pub head: usize, // MRU
    pub tail: usize, // LRU

    pub static fn withCapacity(max_cap: usize): LruCacheI64 {
        let clamped = if (max_cap == 0) { 1 } else if (max_cap > LRU_MAX_CAP) { LRU_MAX_CAP } else { max_cap };
        let empty = LruNode { occupied: false, key: "", val: 0, hash: 0, prev: LRU_NIL, next: LRU_NIL };
        let arr: [64]LruNode = undefined;
        let i: usize = 0;
        while (i < LRU_MAX_CAP) {
            arr[i] = empty;
            i = i + 1;
        }
        return LruCacheI64 {
            nodes: arr,
            cap: clamped,
            count: 0,
            head: LRU_NIL,
            tail: LRU_NIL,
        };
    }

    pub fn len(): usize { return _.count; }
    pub fn capacity(): usize { return _.cap; }
    pub fn isEmpty(): bool { return _.count == 0; }

    fn findSlot(key: string, h: u64): usize {
        let i: usize = 0;
        while (i < _.cap) {
            if (_.nodes[i].occupied && _.nodes[i].hash == h && lruStrEq(_.nodes[i].key, key)) {
                return i;
            }
            i = i + 1;
        }
        return LRU_NIL;
    }

    fn findFreeSlot(): usize {
        let i: usize = 0;
        while (i < _.cap) {
            if (!_.nodes[i].occupied) { return i; }
            i = i + 1;
        }
        return LRU_NIL;
    }

    fn detach(*_, idx: usize) {
        let p = _.nodes[idx].prev;
        let n = _.nodes[idx].next;
        if (p != LRU_NIL) {
            _.nodes[p].next = n;
        } else {
            _.head = n;
        }
        if (n != LRU_NIL) {
            _.nodes[n].prev = p;
        } else {
            _.tail = p;
        }
        _.nodes[idx].prev = LRU_NIL;
        _.nodes[idx].next = LRU_NIL;
    }

    fn attachFront(*_, idx: usize) {
        _.nodes[idx].prev = LRU_NIL;
        _.nodes[idx].next = _.head;
        if (_.head != LRU_NIL) {
            _.nodes[_.head].prev = idx;
        } else {
            _.tail = idx;
        }
        _.head = idx;
    }

    pub fn contains(key: string): bool {
        let h = fnv1a64(key);
        return _.findSlot(key, h) != LRU_NIL;
    }

    /// Reads value without updating recency order.
    pub fn peek(key: string): i64? {
        let h = fnv1a64(key);
        let idx = _.findSlot(key, h);
        if (idx == LRU_NIL) { return null; }
        return _.nodes[idx].val;
    }

    /// Returns the least-recently-used key without removing it.
    pub fn peekLruKey(): string? {
        if (_.tail == LRU_NIL) { return null; }
        return _.nodes[_.tail].key;
    }

    /// Returns the most-recently-used key without removing it.
    pub fn peekMruKey(): string? {
        if (_.head == LRU_NIL) { return null; }
        return _.nodes[_.head].key;
    }

    /// Reads value and promotes entry to MRU head.
    pub fn get(*_, key: string): i64? {
        let h = fnv1a64(key);
        let idx = _.findSlot(key, h);
        if (idx == LRU_NIL) { return null; }
        if (idx != _.head) {
            _.detach(idx);
            _.attachFront(idx);
        }
        return _.nodes[idx].val;
    }

    /// Inserts or updates `(key, val)` at MRU head; evicts LRU tail if full.
    /// Returns previous value if `key` already existed, else `null`.
    pub fn put(*_, key: string, val: i64): i64? {
        let h = fnv1a64(key);
        let existing = _.findSlot(key, h);
        if (existing != LRU_NIL) {
            let old = _.nodes[existing].val;
            _.nodes[existing].val = val;
            if (existing != _.head) {
                _.detach(existing);
                _.attachFront(existing);
            }
            return old;
        }

        let slot = LRU_NIL;
        if (_.count >= _.cap) {
            // Evict LRU tail and reuse its slot
            slot = _.tail;
            _.detach(slot);
            _.count = _.count - 1;
        } else {
            slot = _.findFreeSlot();
        }
        if (slot == LRU_NIL) { return null; }

        _.nodes[slot] = LruNode {
            occupied: true,
            key: key,
            val: val,
            hash: h,
            prev: LRU_NIL,
            next: LRU_NIL,
        };
        _.attachFront(slot);
        _.count = _.count + 1;
        return null;
    }

    /// Removes `key` and returns its value, or `null` if absent.
    pub fn pop(*_, key: string): i64? {
        let h = fnv1a64(key);
        let idx = _.findSlot(key, h);
        if (idx == LRU_NIL) { return null; }
        let out = _.nodes[idx].val;
        _.detach(idx);
        _.nodes[idx].occupied = false;
        _.nodes[idx].key = "";
        _.count = _.count - 1;
        return out;
    }

    /// Evicts and returns the least-recently-used value, or `null` if empty.
    pub fn popLru(*_): i64? {
        if (_.tail == LRU_NIL) { return null; }
        let idx = _.tail;
        let out = _.nodes[idx].val;
        _.detach(idx);
        _.nodes[idx].occupied = false;
        _.nodes[idx].key = "";
        _.count = _.count - 1;
        return out;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < _.cap) {
            _.nodes[i].occupied = false;
            _.nodes[i].key = "";
            _.nodes[i].prev = LRU_NIL;
            _.nodes[i].next = LRU_NIL;
            i = i + 1;
        }
        _.count = 0;
        _.head = LRU_NIL;
        _.tail = LRU_NIL;
    }
}
