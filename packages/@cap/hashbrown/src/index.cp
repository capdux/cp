// @cap/hashbrown — Zero-heap SwissTable-style control-byte hash map & hash set
// Clean-room MIT CP implementation of crates.io #1 `hashbrown` (3.26B+ downloads)
// Uses 1-byte H2 metadata control tags (`CTRL_EMPTY = 0x80`, `CTRL_DELETED = 0xFE`, `0x00..0x7F` = H2 hash)
// for fast cache-friendly probing, reusing `@cap/fnv-hash`.

import { fnv1a64 } from "../../fnv-hash/src/index.cp";

pub const HASHBROWN_SLOTS: usize = 128;
pub const HASHBROWN_MAX_LOAD: usize = 96; // 75% load factor

const HB_CTRL_EMPTY: u8 = 0x80;
const HB_CTRL_DELETED: u8 = 0xFE;

pub struct HbGetResult {
    pub found: bool,
    pub value: i64,
}

pub struct HbInsertResult {
    pub ok: bool,
    pub replaced: bool,
    pub old_value: i64,
}

fn hbHashU64(key: u64): u64 {
    let bytes: [8]u8 = undefined;
    bytes[0] = (key & 0xFF) as u8;
    bytes[1] = ((key >> 8) & 0xFF) as u8;
    bytes[2] = ((key >> 16) & 0xFF) as u8;
    bytes[3] = ((key >> 24) & 0xFF) as u8;
    bytes[4] = ((key >> 32) & 0xFF) as u8;
    bytes[5] = ((key >> 40) & 0xFF) as u8;
    bytes[6] = ((key >> 48) & 0xFF) as u8;
    bytes[7] = ((key >> 56) & 0xFF) as u8;
    return fnv1a64(bytes[0..8]);
}

fn hbH2(h: u64): u8 {
    // Top 7 bits of hash as SwissTable H2 tag (0x00..0x7F)
    return ((h >> 57) & 0x7F) as u8;
}

/// SwissTable-style zero-heap `u64 -> i64` hash map (128 slots, max 96 items).
pub struct HashMapU64 {
    ctrl: [128]u8,
    keys: [128]u64,
    vals: [128]i64,
    used: usize,

    pub static fn new(): HashMapU64 {
        let m = HashMapU64 {
            ctrl: undefined,
            keys: undefined,
            vals: undefined,
            used: 0,
        };
        let i: usize = 0;
        while (i < HASHBROWN_SLOTS) {
            m.ctrl[i] = HB_CTRL_EMPTY;
            i = i + 1;
        }
        return m;
    }

    pub fn len(): usize { return _.used; }
    pub fn capacity(): usize { return HASHBROWN_MAX_LOAD; }
    pub fn isEmpty(): bool { return _.used == 0; }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < HASHBROWN_SLOTS) {
            _.ctrl[i] = HB_CTRL_EMPTY;
            i = i + 1;
        }
        _.used = 0;
    }

    fn findSlot(key: u64): i32 {
        let h = hbHashU64(key);
        let tag = hbH2(h);
        let mask: usize = HASHBROWN_SLOTS - 1;
        let idx = (h as usize) & mask;
        let step: usize = 0;
        while (step < HASHBROWN_SLOTS) {
            let c = _.ctrl[idx];
            if (c == HB_CTRL_EMPTY) { return -1; }
            if (c == tag && _.keys[idx] == key) { return idx as i32; }
            idx = (idx + 1) & mask;
            step = step + 1;
        }
        return -1;
    }

    pub fn containsKey(key: u64): bool {
        return _.findSlot(key) >= 0;
    }

    pub fn get(key: u64): HbGetResult {
        let slot = _.findSlot(key);
        if (slot < 0) {
            return HbGetResult { found: false, value: 0 };
        }
        return HbGetResult { found: true, value: _.vals[slot as usize] };
    }

    pub fn insert(*_, key: u64, value: i64): HbInsertResult {
        let h = hbHashU64(key);
        let tag = hbH2(h);
        let mask: usize = HASHBROWN_SLOTS - 1;
        let idx = (h as usize) & mask;
        let first_deleted: i32 = -1;
        let step: usize = 0;

        while (step < HASHBROWN_SLOTS) {
            let c = _.ctrl[idx];
            if (c == HB_CTRL_EMPTY) {
                if (_.used >= HASHBROWN_MAX_LOAD) {
                    return HbInsertResult { ok: false, replaced: false, old_value: 0 };
                }
                let target = if (first_deleted >= 0) { first_deleted as usize } else { idx };
                _.ctrl[target] = tag;
                _.keys[target] = key;
                _.vals[target] = value;
                _.used = _.used + 1;
                return HbInsertResult { ok: true, replaced: false, old_value: 0 };
            }
            if (c == HB_CTRL_DELETED) {
                if (first_deleted < 0) { first_deleted = idx as i32; }
            } else if (c == tag && _.keys[idx] == key) {
                let old = _.vals[idx];
                _.vals[idx] = value;
                return HbInsertResult { ok: true, replaced: true, old_value: old };
            }
            idx = (idx + 1) & mask;
            step = step + 1;
        }

        if (first_deleted >= 0 && _.used < HASHBROWN_MAX_LOAD) {
            let target = first_deleted as usize;
            _.ctrl[target] = tag;
            _.keys[target] = key;
            _.vals[target] = value;
            _.used = _.used + 1;
            return HbInsertResult { ok: true, replaced: false, old_value: 0 };
        }
        return HbInsertResult { ok: false, replaced: false, old_value: 0 };
    }

    pub fn remove(*_, key: u64): HbGetResult {
        let slot = _.findSlot(key);
        if (slot < 0) {
            return HbGetResult { found: false, value: 0 };
        }
        let u = slot as usize;
        let old = _.vals[u];
        _.ctrl[u] = HB_CTRL_DELETED;
        _.used = _.used - 1;
        return HbGetResult { found: true, value: old };
    }
}

/// SwissTable-style zero-heap `u64` hash set built on `HashMapU64`.
pub struct HashSetU64 {
    inner: HashMapU64,

    pub static fn new(): HashSetU64 {
        return HashSetU64 { inner: HashMapU64.new() };
    }

    pub fn len(): usize { return _.inner.len(); }
    pub fn isEmpty(): bool { return _.inner.isEmpty(); }
    pub fn clear(*_) { _.inner.clear(); }

    pub fn insert(*_, key: u64): bool {
        let r: HbInsertResult = _.inner.insert(key, 1);
        return r.ok && !r.replaced;
    }

    pub fn contains(key: u64): bool {
        return _.inner.containsKey(key);
    }

    pub fn remove(*_, key: u64): bool {
        let r: HbGetResult = _.inner.remove(key);
        return r.found;
    }
}
