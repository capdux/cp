// Source: Custom implementation by Antigravity AI
// lib/collections/string_map.cp
// Length-filtered fast string lookup map
// Absorbed from Bun src/collections/comptime_string_map.cp & StaticHashMap.cp

pub struct StringEntry<V> {
    key: []const u8 = "";
    value: V = undefined;
    occupied: bool = false;
}

pub struct FastStringMap<V, const CAP: usize> {
    entries: [CAP]StringEntry<V> = undefined;
    len: usize = 0;

    pub static fn init(): FastStringMap<V, CAP> {
        let m = FastStringMap<V, CAP> {
            entries: undefined,
            len: 0,
        };
        let i: usize = 0;
        while (i < CAP) {
            m.entries[i].occupied = false;
            m.entries[i].key = "";
            i += 1;
        }
        return m;
    }

    pub static fn equalsBytes(a: []const u8, b: []const u8): bool {
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

    pub static fn equalsAsciiCaseInsensitive(a: []const u8, b: []const u8): bool {
        if (a.len != b.len) {
            return false;
        }
        let i: usize = 0;
        while (i < a.len) {
            const ca = if (a[i] >= 65 && a[i] <= 90) a[i] + 32 else a[i];
            const cb = if (b[i] >= 65 && b[i] <= 90) b[i] + 32 else b[i];
            if (ca != cb) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    pub fn count(): usize {
        return _.len;
    }

    pub fn put(*_, key: []const u8, value: V): bool {
        let i: usize = 0;
        while (i < _.len) {
            if (_.entries[i].occupied && FastStringMap<V, CAP>.equalsBytes(_.entries[i].key, key)) {
                _.entries[i].value = value;
                return true;
            }
            i += 1;
        }
        if (_.len >= CAP) {
            return false;
        }
        _.entries[_.len] = StringEntry<V> {
            key: key,
            value: value,
            occupied: true,
        };
        _.len += 1;
        return true;
    }

    // Absorbed from Bun ComptimeStringMap: filters by key.len first before byte comparison
    pub fn get(key: []const u8): V? {
        let i: usize = 0;
        while (i < _.len) {
            if (_.entries[i].occupied && _.entries[i].key.len == key.len) {
                if (FastStringMap<V, CAP>.equalsBytes(_.entries[i].key, key)) {
                    return _.entries[i].value;
                }
            }
            i += 1;
        }
        return null;
    }

    pub fn getCaseInsensitive(key: []const u8): V? {
        let i: usize = 0;
        while (i < _.len) {
            if (_.entries[i].occupied && _.entries[i].key.len == key.len) {
                if (FastStringMap<V, CAP>.equalsAsciiCaseInsensitive(_.entries[i].key, key)) {
                    return _.entries[i].value;
                }
            }
            i += 1;
        }
        return null;
    }

    pub fn has(key: []const u8): bool {
        let i: usize = 0;
        while (i < _.len) {
            if (_.entries[i].occupied && _.entries[i].key.len == key.len) {
                if (FastStringMap<V, CAP>.equalsBytes(_.entries[i].key, key)) {
                    return true;
                }
            }
            i += 1;
        }
        return false;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < _.len) {
            _.entries[i].occupied = false;
            i += 1;
        }
        _.len = 0;
    }
}
