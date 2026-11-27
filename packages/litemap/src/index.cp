// @cap/litemap — Zero-allocation compact sorted binary-search map for CP
// Clean-room MIT implementation inspired by Rust ICU4X `litemap` (crates.io #168, 824M+ downloads).
//
// Features:
// - Flat sorted array storage (capacity 64) with zero hash-table overhead and maximum CPU cache locality.
// - O(log N) binary search, deterministic ascending key iteration, in-place replace with previous value,
//   ordered removal, and capacity overflow protection.
// - Provides both `LiteMapStr` (string -> string) and `LiteMapU64` (u64 -> i64).
// - Zero heap allocation.

const LITEMAP_CAP: usize = 64;

pub struct SearchResult {
    pub found: bool,
    pub index: usize,
}

pub struct StrEntry {
    pub key: string,
    pub value: string,
}

pub struct StrInsertResult {
    pub ok: bool,
    pub replaced: bool,
    pub prev: string,
}

pub struct U64Entry {
    pub key: u64,
    pub value: i64,
}

pub struct U64InsertResult {
    pub ok: bool,
    pub replaced: bool,
    pub prev: i64,
}

fn cmpStr(a: string, b: string) -> i8 {
    const min_len = if (a.len < b.len) { a.len } else { b.len };
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] < b[i]) {
            return -1;
        }
        if (a[i] > b[i]) {
            return 1;
        }
        i += 1;
    }
    if (a.len < b.len) {
        return -1;
    }
    if (a.len > b.len) {
        return 1;
    }
    return 0;
}

/// Fixed-capacity (64) sorted flat-array map from `string` to `string` with O(log N) binary search.
pub struct LiteMapStr {
    pub entries: [LITEMAP_CAP]StrEntry,
    pub count: usize,

    pub static fn init() -> LiteMapStr {
        let arr: [LITEMAP_CAP]StrEntry = undefined;
        return LiteMapStr{ entries: arr, count: 0 };
    }

    pub fn len() -> usize {
        return _.count;
    }

    pub fn isEmpty() -> bool {
        return _.count == 0;
    }

    pub fn isFull() -> bool {
        return _.count >= LITEMAP_CAP;
    }

    pub fn clear(*_) {
        _.count = 0;
    }

    /// Binary searches for `key`. If found, `index` is its position; otherwise `index` is the sorted insertion point.
    pub fn binarySearch(key: string) -> SearchResult {
        let lo: usize = 0;
        let hi: usize = _.count;
        while (lo < hi) {
            const mid = lo + ((hi - lo) >> 1);
            const c = cmpStr(_.entries[mid].key, key);
            if (c == 0) {
                return SearchResult{ found: true, index: mid };
            }
            if (c < 0) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return SearchResult{ found: false, index: lo };
    }

    pub fn get(key: string) -> string? {
        const sr = _.binarySearch(key);
        if (sr.found) {
            return _.entries[sr.index].value;
        }
        return null;
    }

    pub fn containsKey(key: string) -> bool {
        return _.binarySearch(key).found;
    }

    /// Inserts or replaces `(key, value)` maintaining strict ascending lexicographical order.
    /// Returns `ok: false` if the map is already at capacity (64) and `key` is new.
    pub fn insert(*_, key: string, value: string) -> StrInsertResult {
        const sr = _.binarySearch(key);
        if (sr.found) {
            const old = _.entries[sr.index].value;
            _.entries[sr.index].value = value;
            return StrInsertResult{ ok: true, replaced: true, prev: old };
        }
        if (_.count >= LITEMAP_CAP) {
            return StrInsertResult{ ok: false, replaced: false, prev: "" };
        }
        let j = _.count;
        while (j > sr.index) {
            _.entries[j] = _.entries[j - 1];
            j -= 1;
        }
        _.entries[sr.index] = StrEntry{ key: key, value: value };
        _.count += 1;
        return StrInsertResult{ ok: true, replaced: false, prev: "" };
    }

    /// Removes `key` if present, shifting subsequent entries left to preserve sort order, and returns its old value.
    pub fn remove(*_, key: string) -> string? {
        const sr = _.binarySearch(key);
        if (!sr.found) {
            return null;
        }
        const removed = _.entries[sr.index].value;
        let j = sr.index;
        while (j + 1 < _.count) {
            _.entries[j] = _.entries[j + 1];
            j += 1;
        }
        _.count -= 1;
        return removed;
    }

    /// Returns the entry at sorted index `idx`, or `null` if out of bounds.
    pub fn entryAt(idx: usize) -> StrEntry? {
        if (idx >= _.count) {
            return null;
        }
        return _.entries[idx];
    }
}

/// Fixed-capacity (64) sorted flat-array map from `u64` to `i64` with O(log N) binary search.
pub struct LiteMapU64 {
    pub entries: [LITEMAP_CAP]U64Entry,
    pub count: usize,

    pub static fn init() -> LiteMapU64 {
        let arr: [LITEMAP_CAP]U64Entry = undefined;
        return LiteMapU64{ entries: arr, count: 0 };
    }

    pub fn len() -> usize {
        return _.count;
    }

    pub fn isEmpty() -> bool {
        return _.count == 0;
    }

    pub fn isFull() -> bool {
        return _.count >= LITEMAP_CAP;
    }

    pub fn clear(*_) {
        _.count = 0;
    }

    pub fn binarySearch(key: u64) -> SearchResult {
        let lo: usize = 0;
        let hi: usize = _.count;
        while (lo < hi) {
            const mid = lo + ((hi - lo) >> 1);
            const cur = _.entries[mid].key;
            if (cur == key) {
                return SearchResult{ found: true, index: mid };
            }
            if (cur < key) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return SearchResult{ found: false, index: lo };
    }

    pub fn get(key: u64) -> i64? {
        const sr = _.binarySearch(key);
        if (sr.found) {
            return _.entries[sr.index].value;
        }
        return null;
    }

    pub fn containsKey(key: u64) -> bool {
        return _.binarySearch(key).found;
    }

    pub fn insert(*_, key: u64, value: i64) -> U64InsertResult {
        const sr = _.binarySearch(key);
        if (sr.found) {
            const old = _.entries[sr.index].value;
            _.entries[sr.index].value = value;
            return U64InsertResult{ ok: true, replaced: true, prev: old };
        }
        if (_.count >= LITEMAP_CAP) {
            return U64InsertResult{ ok: false, replaced: false, prev: 0 };
        }
        let j = _.count;
        while (j > sr.index) {
            _.entries[j] = _.entries[j - 1];
            j -= 1;
        }
        _.entries[sr.index] = U64Entry{ key: key, value: value };
        _.count += 1;
        return U64InsertResult{ ok: true, replaced: false, prev: 0 };
    }

    pub fn remove(*_, key: u64) -> i64? {
        const sr = _.binarySearch(key);
        if (!sr.found) {
            return null;
        }
        const removed = _.entries[sr.index].value;
        let j = sr.index;
        while (j + 1 < _.count) {
            _.entries[j] = _.entries[j + 1];
            j += 1;
        }
        _.count -= 1;
        return removed;
    }

    pub fn entryAt(idx: usize) -> U64Entry? {
        if (idx >= _.count) {
            return null;
        }
        return _.entries[idx];
    }
}
