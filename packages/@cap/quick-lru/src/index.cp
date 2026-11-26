// @cap/quick-lru — Zero-heap two-generation QuickLRU cache modeled on Sindre Sorhus's `quick-lru`.
// Clean-room MIT implementation; reuses `@cap/fnv-hash` for 64-bit key hashing.
// Uses two generation tables (`cur` and `old`): when `cur` reaches `max_size`, `cur` becomes `old`
// and a fresh `cur` starts, giving O(1) amortized eviction with minimal metadata overhead.

import { fnv1a64 } from "../../fnv-hash/src/index.cp";

pub const QLRU_MAX_GEN: usize = 32;

pub struct QlruSlot {
    pub occupied: bool,
    pub key: string,
    pub val: i64,
    pub hash: u64,
}

fn qlruStrEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

pub struct QuickLruI64 {
    pub cur: [32]QlruSlot,
    pub old: [32]QlruSlot,
    pub cur_len: usize,
    pub old_len: usize,
    pub max_size: usize,

    pub static fn new(max_entries: usize): QuickLruI64 {
        let clamped = if (max_entries == 0) { 1 } else if (max_entries > QLRU_MAX_GEN) { QLRU_MAX_GEN } else { max_entries };
        let empty = QlruSlot { occupied: false, key: "", val: 0, hash: 0 };
        let a: [32]QlruSlot = undefined;
        let b: [32]QlruSlot = undefined;
        let i: usize = 0;
        while (i < QLRU_MAX_GEN) {
            a[i] = empty;
            b[i] = empty;
            i = i + 1;
        }
        return QuickLruI64 {
            cur: a,
            old: b,
            cur_len: 0,
            old_len: 0,
            max_size: clamped,
        };
    }

    pub fn maxSize(): usize { return _.max_size; }

    /// Approximate total entries across current + previous generation (deduplicated).
    pub fn size(): usize {
        let count = _.cur_len;
        let i: usize = 0;
        while (i < _.max_size) {
            if (_.old[i].occupied && _.findInCur(_.old[i].key, _.old[i].hash) >= QLRU_MAX_GEN) {
                count = count + 1;
            }
            i = i + 1;
        }
        return count;
    }

    fn findInCur(key: string, h: u64): usize {
        let i: usize = 0;
        while (i < _.max_size) {
            if (_.cur[i].occupied && _.cur[i].hash == h && qlruStrEq(_.cur[i].key, key)) {
                return i;
            }
            i = i + 1;
        }
        return QLRU_MAX_GEN;
    }

    fn findInOld(key: string, h: u64): usize {
        let i: usize = 0;
        while (i < _.max_size) {
            if (_.old[i].occupied && _.old[i].hash == h && qlruStrEq(_.old[i].key, key)) {
                return i;
            }
            i = i + 1;
        }
        return QLRU_MAX_GEN;
    }

    fn rotateGenerations(*_) {
        let empty = QlruSlot { occupied: false, key: "", val: 0, hash: 0 };
        let i: usize = 0;
        while (i < _.max_size) {
            _.old[i] = _.cur[i];
            _.cur[i] = empty;
            i = i + 1;
        }
        _.old_len = _.cur_len;
        _.cur_len = 0;
    }

    fn insertCur(*_, key: string, val: i64, h: u64) {
        let existing = _.findInCur(key, h);
        if (existing < QLRU_MAX_GEN) {
            _.cur[existing].val = val;
            return;
        }
        if (_.cur_len >= _.max_size) {
            _.rotateGenerations();
        }
        let i: usize = 0;
        while (i < _.max_size) {
            if (!_.cur[i].occupied) {
                _.cur[i] = QlruSlot { occupied: true, key: key, val: val, hash: h };
                _.cur_len = _.cur_len + 1;
                return;
            }
            i = i + 1;
        }
    }

    pub fn set(*_, key: string, val: i64) {
        let h = fnv1a64(key);
        _.insertCur(key, val, h);
    }

    pub fn has(key: string): bool {
        let h = fnv1a64(key);
        return _.findInCur(key, h) < QLRU_MAX_GEN || _.findInOld(key, h) < QLRU_MAX_GEN;
    }

    pub fn peek(key: string): i64? {
        let h = fnv1a64(key);
        let ci = _.findInCur(key, h);
        if (ci < QLRU_MAX_GEN) { return _.cur[ci].val; }
        let oi = _.findInOld(key, h);
        if (oi < QLRU_MAX_GEN) { return _.old[oi].val; }
        return null;
    }

    /// Reads `key`, promoting it from `old` generation into `cur` if found in `old`.
    pub fn get(*_, key: string): i64? {
        let h = fnv1a64(key);
        let ci = _.findInCur(key, h);
        if (ci < QLRU_MAX_GEN) { return _.cur[ci].val; }
        let oi = _.findInOld(key, h);
        if (oi < QLRU_MAX_GEN) {
            let v = _.old[oi].val;
            _.old[oi].occupied = false;
            if (_.old_len > 0) { _.old_len = _.old_len - 1; }
            _.insertCur(key, v, h);
            return v;
        }
        return null;
    }

    pub fn delete(*_, key: string): bool {
        let h = fnv1a64(key);
        let removed = false;
        let ci = _.findInCur(key, h);
        if (ci < QLRU_MAX_GEN) {
            _.cur[ci].occupied = false;
            if (_.cur_len > 0) { _.cur_len = _.cur_len - 1; }
            removed = true;
        }
        let oi = _.findInOld(key, h);
        if (oi < QLRU_MAX_GEN) {
            _.old[oi].occupied = false;
            if (_.old_len > 0) { _.old_len = _.old_len - 1; }
            removed = true;
        }
        return removed;
    }

    pub fn clear(*_) {
        let empty = QlruSlot { occupied: false, key: "", val: 0, hash: 0 };
        let i: usize = 0;
        while (i < _.max_size) {
            _.cur[i] = empty;
            _.old[i] = empty;
            i = i + 1;
        }
        _.cur_len = 0;
        _.old_len = 0;
    }
}
