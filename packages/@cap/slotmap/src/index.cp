// @cap/slotmap — Zero-allocation generational-index slot map for CP
// Clean-room MIT implementation inspired by Rust `slotmap` v1 (550M+ downloads).
// O(1) insert/get/set/remove with intrusive free-list and strict ABA dangling-key protection.

const SLM_CAPACITY: usize = 128;
const SLM_CAPACITY_U32: u32 = 128;
const SLM_NONE_IDX: u32 = 0xFFFFFFFF;

fn slmIsOccupiedVersion(version: u32) -> bool {
    return (version & (1 as u32)) == (1 as u32);
}

fn slmAllocVersion(cur_version: u32) -> u32 {
    // Even version -> odd version (occupied).
    return cur_version + (1 as u32);
}

fn slmFreeVersion(cur_version: u32) -> u32 {
    // Odd version -> even version (vacant). Avoid wrapping to 0 (null sentinel).
    if (cur_version == (0xFFFFFFFF as u32)) {
        return 2 as u32;
    }
    return cur_version + (1 as u32);
}

/// Generational handle identifying a slot in `SlotMapI64` and `SecondaryMapI64`.
/// Odd `version` indicates a live occupied slot; `version == 0` denotes a null key.
pub struct SlotKey {
    pub idx: u32,
    pub version: u32,

    pub static fn nullKey() -> SlotKey {
        return SlotKey{
            idx: 0 as u32,
            version: 0 as u32,
        };
    }

    pub fn isNull() -> bool {
        return _.version == (0 as u32);
    }

    pub fn eq(other: SlotKey) -> bool {
        return _.idx == other.idx && _.version == other.version;
    }

    pub fn toU64() -> u64 {
        return ((_.version as u64) * (0x100000000 as u64)) | (_.idx as u64);
    }

    pub static fn fromU64(raw: u64) -> SlotKey {
        let idx = (raw & (0xFFFFFFFF as u64)) as u32;
        let version = ((raw / (0x100000000 as u64)) & (0xFFFFFFFF as u64)) as u32;
        return SlotKey{
            idx: idx,
            version: version,
        };
    }
}

/// Zero-allocation fixed-capacity (128 slots) generational slot map storing `i64` values.
/// Uses an O(1) free-list; odd versions are occupied and even versions are vacant.
pub struct SlotMapI64 {
    pub values: [128]i64,
    pub versions: [128]u32,
    pub next_free: [128]u32,
    pub free_head: u32,
    pub count: usize,

    pub static fn new() -> SlotMapI64 {
        let vals: [128]i64 = undefined;
        let vers: [128]u32 = undefined;
        let nexts: [128]u32 = undefined;
        let i: usize = 0;
        while (i < SLM_CAPACITY) {
            vals[i] = 0 as i64;
            vers[i] = 0 as u32;
            if (i + (1 as usize) < SLM_CAPACITY) {
                nexts[i] = (i + (1 as usize)) as u32;
            } else {
                nexts[i] = SLM_NONE_IDX;
            }
            i += 1;
        }
        return SlotMapI64{
            values: vals,
            versions: vers,
            next_free: nexts,
            free_head: 0 as u32,
            count: 0 as usize,
        };
    }

    pub fn len() -> usize {
        return _.count;
    }

    pub fn capacity() -> usize {
        return SLM_CAPACITY;
    }

    pub fn isEmpty() -> bool {
        return _.count == (0 as usize);
    }

    pub fn containsKey(key: SlotKey) -> bool {
        if (key.idx >= SLM_CAPACITY_U32) {
            return false;
        }
        if (!slmIsOccupiedVersion(key.version)) {
            return false;
        }
        return _.versions[key.idx as usize] == key.version;
    }

    pub fn insert(*_, val: i64) -> SlotKey? {
        if (_.free_head >= SLM_CAPACITY_U32) {
            return null;
        }
        let idx = _.free_head;
        let i = idx as usize;
        _.free_head = _.next_free[i];
        _.next_free[i] = SLM_NONE_IDX;

        let new_ver = slmAllocVersion(_.versions[i]);
        _.versions[i] = new_ver;
        _.values[i] = val;
        _.count += 1;

        return SlotKey{
            idx: idx,
            version: new_ver,
        };
    }

    pub fn get(key: SlotKey) -> i64? {
        if (!_.containsKey(key)) {
            return null;
        }
        return _.values[key.idx as usize];
    }

    pub fn set(*_, key: SlotKey, val: i64) -> bool {
        if (!_.containsKey(key)) {
            return false;
        }
        _.values[key.idx as usize] = val;
        return true;
    }

    pub fn remove(*_, key: SlotKey) -> i64? {
        if (!_.containsKey(key)) {
            return null;
        }
        let i = key.idx as usize;
        let old_val = _.values[i];
        _.versions[i] = slmFreeVersion(_.versions[i]);
        _.values[i] = 0 as i64;
        _.next_free[i] = _.free_head;
        _.free_head = key.idx;
        _.count -= 1;
        return old_val;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < SLM_CAPACITY) {
            if (slmIsOccupiedVersion(_.versions[i])) {
                _.versions[i] = slmFreeVersion(_.versions[i]);
            }
            _.values[i] = 0 as i64;
            if (i + (1 as usize) < SLM_CAPACITY) {
                _.next_free[i] = (i + (1 as usize)) as u32;
            } else {
                _.next_free[i] = SLM_NONE_IDX;
            }
            i += 1;
        }
        _.free_head = 0 as u32;
        _.count = 0 as usize;
    }
}

/// Zero-allocation fixed-capacity (128 slots) secondary attribute map paired with `SlotKey`.
pub struct SecondaryMapI64 {
    pub values: [128]i64,
    pub versions: [128]u32,
    pub count: usize,

    pub static fn new() -> SecondaryMapI64 {
        let vals: [128]i64 = undefined;
        let vers: [128]u32 = undefined;
        let i: usize = 0;
        while (i < SLM_CAPACITY) {
            vals[i] = 0 as i64;
            vers[i] = 0 as u32;
            i += 1;
        }
        return SecondaryMapI64{
            values: vals,
            versions: vers,
            count: 0 as usize,
        };
    }

    pub fn len() -> usize {
        return _.count;
    }

    pub fn containsKey(key: SlotKey) -> bool {
        if (key.idx >= SLM_CAPACITY_U32) {
            return false;
        }
        if (!slmIsOccupiedVersion(key.version)) {
            return false;
        }
        return _.versions[key.idx as usize] == key.version;
    }

    pub fn insert(*_, key: SlotKey, val: i64) -> bool {
        if (key.idx >= SLM_CAPACITY_U32) {
            return false;
        }
        if (!slmIsOccupiedVersion(key.version)) {
            return false;
        }
        let i = key.idx as usize;
        if (_.versions[i] == (0 as u32)) {
            _.count += 1;
        }
        _.versions[i] = key.version;
        _.values[i] = val;
        return true;
    }

    pub fn get(key: SlotKey) -> i64? {
        if (!_.containsKey(key)) {
            return null;
        }
        return _.values[key.idx as usize];
    }

    pub fn remove(*_, key: SlotKey) -> i64? {
        if (!_.containsKey(key)) {
            return null;
        }
        let i = key.idx as usize;
        let old_val = _.values[i];
        _.versions[i] = 0 as u32;
        _.values[i] = 0 as i64;
        _.count -= 1;
        return old_val;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < SLM_CAPACITY) {
            _.versions[i] = 0 as u32;
            _.values[i] = 0 as i64;
            i += 1;
        }
        _.count = 0 as usize;
    }
}
