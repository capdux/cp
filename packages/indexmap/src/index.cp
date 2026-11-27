// @cap/indexmap — Pure CP zero-allocation insertion-order-preserving hash map
// Clean-room MIT implementation (Rust `indexmap` v2 parity).
// Dense insertion-order array + open-addressing hash index table powered by @cap/fnv-hash.

import { strEq } from "cap:ascii";
import { fnv1a64 } from "../../@sindresorhus/fnv1a/src/index.cp";

const IM_EMPTY: i32 = -1;
const IM_TOMBSTONE: i32 = -2;
const IM_U64_CAP: usize = 64;
const IM_U64_BUCKETS: usize = 128;
const IM_STR_CAP: usize = 32;
const IM_STR_BUCKETS: usize = 64;
const IM_STR_MAX_KEY: usize = 64;

fn imHashU64(key: u64): usize {
    let bytes: [8]u8 = [
        (key & 0xFF) as u8,
        ((key >> 8) & 0xFF) as u8,
        ((key >> 16) & 0xFF) as u8,
        ((key >> 24) & 0xFF) as u8,
        ((key >> 32) & 0xFF) as u8,
        ((key >> 40) & 0xFF) as u8,
        ((key >> 48) & 0xFF) as u8,
        ((key >> 56) & 0xFF) as u8
    ];
    let h = fnv1a64(bytes[0..8]);
    return (h & 127) as usize;
}

fn imHashStr(key: string): usize {
    let h = fnv1a64(key);
    return (h & 63) as usize;
}

pub struct IndexMapEntry {
    pub key: u64,
    pub val: i64,
}

pub struct InsertResult {
    pub ok: bool,
    pub replaced: bool,
    pub old_val: i64,
    pub index: usize,
}

pub struct GetValResult {
    pub found: bool,
    pub val: i64,
}

pub struct GetEntryResult {
    pub found: bool,
    pub key: u64,
    pub val: i64,
}

pub struct IndexMapStrEntry {
    pub key_buf: [64]u8,
    pub key_len: usize,
    pub val: i64,

    pub fn keyStr(): []const u8 {
        return _.key_buf[0.._.key_len];
    }
}

pub struct GetStrEntryResult {
    pub found: bool,
    pub key_buf: [64]u8,
    pub key_len: usize,
    pub val: i64,

    pub fn keyStr(): []const u8 {
        return _.key_buf[0.._.key_len];
    }
}

/// Zero-allocation insertion-order-preserving hash map (u64 -> i64, capacity 64, 128 hash buckets).
pub struct IndexMapU64 {
    pub entries: [64]IndexMapEntry,
    pub buckets: [128]i32,
    pub used: usize,

    pub static fn new(): IndexMapU64 {
        let empty_entry = IndexMapEntry { key: 0, val: 0 };
        return IndexMapU64 {
            entries: [empty_entry; 64],
            buckets: [-1; 128],
            used: 0
        };
    }

    pub fn len(): usize {
        return _.used;
    }

    pub fn capacity(): usize {
        return IM_U64_CAP;
    }

    pub fn isEmpty(): bool {
        return _.used == 0;
    }

    pub fn clear(*_) {
        _.used = 0;
        let i: usize = 0;
        while (i < IM_U64_BUCKETS) {
            _.buckets[i] = IM_EMPTY;
            i = i + 1;
        }
    }

    pub fn getIndexOf(key: u64): i64 {
        let slot = imHashU64(key);
        let step: usize = 0;
        while (step < IM_U64_BUCKETS) {
            let b = _.buckets[slot];
            if (b == IM_EMPTY) {
                return -1;
            }
            if (b >= 0) {
                let idx = b as usize;
                if (_.entries[idx].key == key) {
                    return idx as i64;
                }
            }
            slot = (slot + 1) & 127;
            step = step + 1;
        }
        return -1;
    }

    pub fn containsKey(key: u64): bool {
        return _.getIndexOf(key) >= 0;
    }

    pub fn get(key: u64): GetValResult {
        let idx = _.getIndexOf(key);
        if (idx < 0) {
            return GetValResult { found: false, val: 0 };
        }
        return GetValResult { found: true, val: _.entries[idx as usize].val };
    }

    pub fn getIndex(idx: usize): GetEntryResult {
        if (idx >= _.used) {
            return GetEntryResult { found: false, key: 0, val: 0 };
        }
        let e = _.entries[idx];
        return GetEntryResult { found: true, key: e.key, val: e.val };
    }

    pub fn first(): GetEntryResult {
        return _.getIndex(0);
    }

    pub fn last(): GetEntryResult {
        if (_.used == 0) {
            return GetEntryResult { found: false, key: 0, val: 0 };
        }
        return _.getIndex(_.used - 1);
    }

    /// Inserts `(key, val)`. If `key` already exists, updates its value in place while preserving
    /// its original insertion order and returns `replaced: true` with `old_val`.
    pub fn insert(*_, key: u64, val: i64): InsertResult {
        let slot = imHashU64(key);
        let first_tomb: i64 = -1;
        let empty_slot: i64 = -1;
        let step: usize = 0;

        while (step < IM_U64_BUCKETS) {
            let b = _.buckets[slot];
            if (b == IM_EMPTY) {
                empty_slot = slot as i64;
                break;
            }
            if (b == IM_TOMBSTONE) {
                if (first_tomb < 0) {
                    first_tomb = slot as i64;
                }
            } else {
                let idx = b as usize;
                if (_.entries[idx].key == key) {
                    let old = _.entries[idx].val;
                    _.entries[idx].val = val;
                    return InsertResult { ok: true, replaced: true, old_val: old, index: idx };
                }
            }
            slot = (slot + 1) & 127;
            step = step + 1;
        }

        if (_.used >= IM_U64_CAP) {
            return InsertResult { ok: false, replaced: false, old_val: 0, index: 0 };
        }

        let target: usize = if (first_tomb >= 0) { first_tomb as usize } else { empty_slot as usize };
        let new_idx = _.used;
        _.entries[new_idx] = IndexMapEntry { key: key, val: val };
        _.buckets[target] = new_idx as i32;
        _.used = new_idx + 1;
        return InsertResult { ok: true, replaced: false, old_val: 0, index: new_idx };
    }

    /// O(1) swap-remove: replaces removed slot with the last entry and updates its hash bucket.
    pub fn swapRemove(*_, key: u64): GetValResult {
        let slot = imHashU64(key);
        let found_bucket: i64 = -1;
        let e_idx: usize = 0;
        let step: usize = 0;

        while (step < IM_U64_BUCKETS) {
            let b = _.buckets[slot];
            if (b == IM_EMPTY) {
                return GetValResult { found: false, val: 0 };
            }
            if (b >= 0) {
                let idx = b as usize;
                if (_.entries[idx].key == key) {
                    found_bucket = slot as i64;
                    e_idx = idx;
                    break;
                }
            }
            slot = (slot + 1) & 127;
            step = step + 1;
        }
        if (found_bucket < 0) {
            return GetValResult { found: false, val: 0 };
        }

        let old_val = _.entries[e_idx].val;
        _.buckets[found_bucket as usize] = IM_TOMBSTONE;

        let last_idx = _.used - 1;
        if (e_idx != last_idx) {
            let moved = _.entries[last_idx];
            _.entries[e_idx] = moved;
            let m_slot = imHashU64(moved.key);
            let m_step: usize = 0;
            while (m_step < IM_U64_BUCKETS) {
                if (_.buckets[m_slot] == (last_idx as i32)) {
                    _.buckets[m_slot] = e_idx as i32;
                    break;
                }
                m_slot = (m_slot + 1) & 127;
                m_step = m_step + 1;
            }
        }
        _.used = last_idx;
        return GetValResult { found: true, val: old_val };
    }

    /// Order-preserving remove: shifts subsequent entries left and rebuilds hash buckets.
    pub fn shiftRemove(*_, key: u64): GetValResult {
        let idx_i64 = _.getIndexOf(key);
        if (idx_i64 < 0) {
            return GetValResult { found: false, val: 0 };
        }
        let e_idx = idx_i64 as usize;
        let old_val = _.entries[e_idx].val;

        let j = e_idx;
        while (j + 1 < _.used) {
            _.entries[j] = _.entries[j + 1];
            j = j + 1;
        }
        _.used = _.used - 1;

        // Rebuild buckets cleanly (also clears all tombstones)
        let bi: usize = 0;
        while (bi < IM_U64_BUCKETS) {
            _.buckets[bi] = IM_EMPTY;
            bi = bi + 1;
        }
        let ei: usize = 0;
        while (ei < _.used) {
            let s = imHashU64(_.entries[ei].key);
            while (_.buckets[s] != IM_EMPTY) {
                s = (s + 1) & 127;
            }
            _.buckets[s] = ei as i32;
            ei = ei + 1;
        }
        return GetValResult { found: true, val: old_val };
    }

    /// Removes and returns the last inserted entry.
    pub fn pop(*_): GetEntryResult {
        if (_.used == 0) {
            return GetEntryResult { found: false, key: 0, val: 0 };
        }
        let last_idx = _.used - 1;
        let e = _.entries[last_idx];
        let slot = imHashU64(e.key);
        let step: usize = 0;
        while (step < IM_U64_BUCKETS) {
            if (_.buckets[slot] == (last_idx as i32)) {
                _.buckets[slot] = IM_TOMBSTONE;
                break;
            }
            slot = (slot + 1) & 127;
            step = step + 1;
        }
        _.used = last_idx;
        return GetEntryResult { found: true, key: e.key, val: e.val };
    }
}

/// Zero-allocation insertion-order-preserving string-keyed hash map (string -> i64, capacity 32).
pub struct IndexMapStr {
    pub entries: [32]IndexMapStrEntry,
    pub buckets: [64]i32,
    pub used: usize,

    pub static fn new(): IndexMapStr {
        let empty_entry = IndexMapStrEntry { key_buf: [0; 64], key_len: 0, val: 0 };
        return IndexMapStr {
            entries: [empty_entry; 32],
            buckets: [-1; 64],
            used: 0
        };
    }

    pub fn len(): usize {
        return _.used;
    }

    pub fn capacity(): usize {
        return IM_STR_CAP;
    }

    pub fn isEmpty(): bool {
        return _.used == 0;
    }

    pub fn clear(*_) {
        _.used = 0;
        let i: usize = 0;
        while (i < IM_STR_BUCKETS) {
            _.buckets[i] = IM_EMPTY;
            i = i + 1;
        }
    }

    pub fn getIndexOf(key: string): i64 {
        let slot = imHashStr(key);
        let step: usize = 0;
        while (step < IM_STR_BUCKETS) {
            let b = _.buckets[slot];
            if (b == IM_EMPTY) {
                return -1;
            }
            if (b >= 0) {
                let idx = b as usize;
                let klen = _.entries[idx].key_len;
                if (strEq(_.entries[idx].key_buf[0..klen], key)) {
                    return idx as i64;
                }
            }
            slot = (slot + 1) & 63;
            step = step + 1;
        }
        return -1;
    }

    pub fn containsKey(key: string): bool {
        return _.getIndexOf(key) >= 0;
    }

    pub fn get(key: string): GetValResult {
        let idx = _.getIndexOf(key);
        if (idx < 0) {
            return GetValResult { found: false, val: 0 };
        }
        return GetValResult { found: true, val: _.entries[idx as usize].val };
    }

    pub fn getIndex(idx: usize): GetStrEntryResult {
        if (idx >= _.used) {
            return GetStrEntryResult { found: false, key_buf: [0; 64], key_len: 0, val: 0 };
        }
        let e = _.entries[idx];
        return GetStrEntryResult { found: true, key_buf: e.key_buf, key_len: e.key_len, val: e.val };
    }

    pub fn first(): GetStrEntryResult {
        return _.getIndex(0);
    }

    pub fn last(): GetStrEntryResult {
        if (_.used == 0) {
            return GetStrEntryResult { found: false, key_buf: [0; 64], key_len: 0, val: 0 };
        }
        return _.getIndex(_.used - 1);
    }

    pub fn insert(*_, key: string, val: i64): InsertResult {
        if (key.len > IM_STR_MAX_KEY) {
            return InsertResult { ok: false, replaced: false, old_val: 0, index: 0 };
        }
        let slot = imHashStr(key);
        let first_tomb: i64 = -1;
        let empty_slot: i64 = -1;
        let step: usize = 0;

        while (step < IM_STR_BUCKETS) {
            let b = _.buckets[slot];
            if (b == IM_EMPTY) {
                empty_slot = slot as i64;
                break;
            }
            if (b == IM_TOMBSTONE) {
                if (first_tomb < 0) {
                    first_tomb = slot as i64;
                }
            } else {
                let idx = b as usize;
                let klen = _.entries[idx].key_len;
                if (strEq(_.entries[idx].key_buf[0..klen], key)) {
                    let old = _.entries[idx].val;
                    _.entries[idx].val = val;
                    return InsertResult { ok: true, replaced: true, old_val: old, index: idx };
                }
            }
            slot = (slot + 1) & 63;
            step = step + 1;
        }

        if (_.used >= IM_STR_CAP) {
            return InsertResult { ok: false, replaced: false, old_val: 0, index: 0 };
        }

        let target: usize = if (first_tomb >= 0) { first_tomb as usize } else { empty_slot as usize };
        let new_idx = _.used;
        let kbuf: [64]u8 = undefined;
        @memcpy(kbuf[0..key.len], key);
        _.entries[new_idx] = IndexMapStrEntry { key_buf: kbuf, key_len: key.len, val: val };
        _.buckets[target] = new_idx as i32;
        _.used = new_idx + 1;
        return InsertResult { ok: true, replaced: false, old_val: 0, index: new_idx };
    }

    pub fn swapRemove(*_, key: string): GetValResult {
        let slot = imHashStr(key);
        let found_bucket: i64 = -1;
        let e_idx: usize = 0;
        let step: usize = 0;

        while (step < IM_STR_BUCKETS) {
            let b = _.buckets[slot];
            if (b == IM_EMPTY) {
                return GetValResult { found: false, val: 0 };
            }
            if (b >= 0) {
                let idx = b as usize;
                let klen = _.entries[idx].key_len;
                if (strEq(_.entries[idx].key_buf[0..klen], key)) {
                    found_bucket = slot as i64;
                    e_idx = idx;
                    break;
                }
            }
            slot = (slot + 1) & 63;
            step = step + 1;
        }
        if (found_bucket < 0) {
            return GetValResult { found: false, val: 0 };
        }

        let old_val = _.entries[e_idx].val;
        _.buckets[found_bucket as usize] = IM_TOMBSTONE;

        let last_idx = _.used - 1;
        if (e_idx != last_idx) {
            let moved = _.entries[last_idx];
            _.entries[e_idx] = moved;
            let m_slot = imHashStr(moved.key_buf[0..moved.key_len]);
            let m_step: usize = 0;
            while (m_step < IM_STR_BUCKETS) {
                if (_.buckets[m_slot] == (last_idx as i32)) {
                    _.buckets[m_slot] = e_idx as i32;
                    break;
                }
                m_slot = (m_slot + 1) & 63;
                m_step = m_step + 1;
            }
        }
        _.used = last_idx;
        return GetValResult { found: true, val: old_val };
    }

    pub fn shiftRemove(*_, key: string): GetValResult {
        let idx_i64 = _.getIndexOf(key);
        if (idx_i64 < 0) {
            return GetValResult { found: false, val: 0 };
        }
        let e_idx = idx_i64 as usize;
        let old_val = _.entries[e_idx].val;

        let j = e_idx;
        while (j + 1 < _.used) {
            _.entries[j] = _.entries[j + 1];
            j = j + 1;
        }
        _.used = _.used - 1;

        let bi: usize = 0;
        while (bi < IM_STR_BUCKETS) {
            _.buckets[bi] = IM_EMPTY;
            bi = bi + 1;
        }
        let ei: usize = 0;
        while (ei < _.used) {
            let klen = _.entries[ei].key_len;
            let s = imHashStr(_.entries[ei].key_buf[0..klen]);
            while (_.buckets[s] != IM_EMPTY) {
                s = (s + 1) & 63;
            }
            _.buckets[s] = ei as i32;
            ei = ei + 1;
        }
        return GetValResult { found: true, val: old_val };
    }

    pub fn pop(*_): GetStrEntryResult {
        if (_.used == 0) {
            return GetStrEntryResult { found: false, key_buf: [0; 64], key_len: 0, val: 0 };
        }
        let last_idx = _.used - 1;
        let e = _.entries[last_idx];
        let slot = imHashStr(e.key_buf[0..e.key_len]);
        let step: usize = 0;
        while (step < IM_STR_BUCKETS) {
            if (_.buckets[slot] == (last_idx as i32)) {
                _.buckets[slot] = IM_TOMBSTONE;
                break;
            }
            slot = (slot + 1) & 63;
            step = step + 1;
        }
        _.used = last_idx;
        return GetStrEntryResult { found: true, key_buf: e.key_buf, key_len: e.key_len, val: e.val };
    }
}

export {
    IndexMapEntry,
    InsertResult,
    GetValResult,
    GetEntryResult,
    IndexMapStrEntry,
    GetStrEntryResult,
    IndexMapU64,
    IndexMapStr
};
