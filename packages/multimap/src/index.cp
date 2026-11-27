// @cap/multimap — Zero-allocation insertion-ordered one-to-many string multimap for CP
// Clean-room MIT implementation inspired by Rust `multimap` (crates.io #390, 381M+ downloads).
//
// Features:
// - Fixed-capacity (128 pairs) stack-allocated multimap preserving insertion order.
// - Supports multiple values per key: `insert`, `getFirst`, `getLast`, `getNth`, `countFor`,
//   `containsKey`, `uniqueKeyCount`, `removeKey` (order-preserving bulk delete), and `removePair`.
// - Zero heap allocation.

const MULTIMAP_CAP: usize = 128;

fn strEq(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub struct MultiEntry {
    pub key: string,
    pub value: string,
}

/// Fixed-capacity (128 entries) insertion-ordered one-to-many map from `string` to multiple `string` values.
pub struct MultiMapStr {
    pub entries: [MULTIMAP_CAP]MultiEntry,
    pub count: usize,

    pub static fn init() -> MultiMapStr {
        let arr: [MULTIMAP_CAP]MultiEntry = undefined;
        return MultiMapStr{ entries: arr, count: 0 };
    }

    pub fn totalValues() -> usize {
        return _.count;
    }

    pub fn isEmpty() -> bool {
        return _.count == 0;
    }

    pub fn isFull() -> bool {
        return _.count >= MULTIMAP_CAP;
    }

    pub fn clear(*_) {
        _.count = 0;
    }

    /// Appends `(key, value)` to the multimap. Allows duplicate keys. Returns `false` if full (128).
    pub fn insert(*_, key: string, value: string) -> bool {
        if (_.count >= MULTIMAP_CAP) {
            return false;
        }
        _.entries[_.count] = MultiEntry{ key: key, value: value };
        _.count += 1;
        return true;
    }

    /// Returns the first value associated with `key`, or `null` if absent.
    pub fn getFirst(key: string) -> string? {
        let i: usize = 0;
        while (i < _.count) {
            if (strEq(_.entries[i].key, key)) {
                return _.entries[i].value;
            }
            i += 1;
        }
        return null;
    }

    /// Returns the last value associated with `key`, or `null` if absent.
    pub fn getLast(key: string) -> string? {
        let i = _.count;
        while (i > 0) {
            i -= 1;
            if (strEq(_.entries[i].key, key)) {
                return _.entries[i].value;
            }
        }
        return null;
    }

    /// Returns the 0-based `n`-th value associated with `key`, or `null` if fewer than `n + 1` exist.
    pub fn getNth(key: string, n: usize) -> string? {
        let seen: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (strEq(_.entries[i].key, key)) {
                if (seen == n) {
                    return _.entries[i].value;
                }
                seen += 1;
            }
            i += 1;
        }
        return null;
    }

    /// Returns how many values are stored under `key`.
    pub fn countFor(key: string) -> usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (strEq(_.entries[i].key, key)) {
                total += 1;
            }
            i += 1;
        }
        return total;
    }

    pub fn containsKey(key: string) -> bool {
        return _.getFirst(key) != null;
    }

    /// Counts distinct keys currently present in the multimap (zero heap allocation).
    pub fn uniqueKeyCount() -> usize {
        let uniq: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            let seen_before = false;
            let j: usize = 0;
            while (j < i) {
                if (strEq(_.entries[j].key, _.entries[i].key)) {
                    seen_before = true;
                    break;
                }
                j += 1;
            }
            if (!seen_before) {
                uniq += 1;
            }
            i += 1;
        }
        return uniq;
    }

    /// Removes all entries matching `key` while preserving the relative order of remaining entries.
    /// Returns the number of removed entries.
    pub fn removeKey(*_, key: string) -> usize {
        let write_idx: usize = 0;
        let removed: usize = 0;
        let read_idx: usize = 0;
        while (read_idx < _.count) {
            if (strEq(_.entries[read_idx].key, key)) {
                removed += 1;
            } else {
                if (write_idx != read_idx) {
                    _.entries[write_idx] = _.entries[read_idx];
                }
                write_idx += 1;
            }
            read_idx += 1;
        }
        _.count = write_idx;
        return removed;
    }

    /// Removes the first entry matching both `key` and `value`, preserving relative order.
    pub fn removePair(*_, key: string, value: string) -> bool {
        let i: usize = 0;
        while (i < _.count) {
            if (strEq(_.entries[i].key, key) && strEq(_.entries[i].value, value)) {
                let j = i;
                while (j + 1 < _.count) {
                    _.entries[j] = _.entries[j + 1];
                    j += 1;
                }
                _.count -= 1;
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// Returns the entry at raw insertion index `idx`, or `null` if out of bounds.
    pub fn entryAt(idx: usize) -> MultiEntry? {
        if (idx >= _.count) {
            return null;
        }
        return _.entries[idx];
    }
}
