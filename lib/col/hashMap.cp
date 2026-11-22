// Source: Absorbed from /data/dev/bun/src/collections/StaticHashMap.cp | Synced: 2026-11-21T12:28:05Z
// ============================================================================
// Absorbed from Bun: bun/src/collections/hash_map.cp
// Upstream Sync: Refer to bun/src/collections/hash_map.zig for future upstream updates.
// ============================================================================
// lib/col/hash_map.cp
// High-performance fixed-capacity zero-heap StaticHashMap<K, V, CAP> & AutoHashMap<K, V>
// Absorbed from Bun src/collections/StaticHashMap.cp with pure CP semantics

pub struct HashEntry<K, V> {
    hash: u64 = 0,
    key: K = undefined,
    value: V = undefined,
    occupied: bool = false,
}

// Compute 64-bit hash for arbitrary primitive keys
pub fn hashKey<K>(key: K): u64 {
    let h: u64 = 0;
    zig {
        const Tk = @TypeOf(key);
        if (comptime (Tk == []const u8 or Tk == []u8 or Tk == string)) {
            h = std.hash.Wyhash.hash(0, key);
        } else if (comptime (@typeInfo(Tk) == .int or @typeInfo(Tk) == .comptime_int)) {
            const val_u64: u64 = @truncate(@as(u128, @bitCast(@as(i128, key))));
            h = val_u64 *% 11400714819323198485;
        } else {
            h = std.hash.Wyhash.hash(0, std.mem.asBytes(&key));
        }
    }
    return if (h == 0) 1 else h;
}

pub fn keyEql<K>(a: K, b: K): bool {
    let eq: bool = false;
    zig {
        const Tk = @TypeOf(a);
        if (comptime (Tk == []const u8 or Tk == []u8 or Tk == string)) {
            eq = std.mem.eql(u8, a, b);
        } else {
            eq = (a == b);
        }
    }
    return eq;
}

pub struct StaticHashMap<K, V, const CAP: usize> {
    entries: [CAP]HashEntry<K, V> = undefined,
    len: usize = 0,

    pub static fn init(): StaticHashMap<K, V, CAP> {
        let map = StaticHashMap<K, V, CAP> {
            entries: undefined,
            len: 0,
        };
        let i: usize = 0;
        while (i < CAP) {
            map.entries[i] = HashEntry<K, V> {
                hash: 0,
                key: undefined,
                value: undefined,
                occupied: false,
            };
            i += 1;
        }
        return map;
    }

    pub fn capacity(): usize {
        return _.entries.len;
    }

    pub fn count(): usize {
        return _.len;
    }

    pub fn isEmpty(): bool {
        return _.len == 0;
    }

    pub fn isFull(): bool {
        return _.len >= CAP;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < CAP) {
            _.entries[i].occupied = false;
            _.entries[i].hash = 0;
            i += 1;
        }
        _.len = 0;
    }

    pub fn put(*_, key: K, value: V): bool {
        const h = hashKey(key);
        let mask = CAP - 1;
        let idx = (h as usize) & mask;
        let probes: usize = 0;

        while (probes < CAP) {
            let slot = *_.entries[idx];
            if (!slot.occupied) {
                slot.occupied = true;
                slot.hash = h;
                slot.key = key;
                slot.value = value;
                _.len += 1;
                return true;
            }
            if (slot.hash == h && keyEql(slot.key, key)) {
                slot.value = value;
                return true;
            }
            idx = (idx + 1) & mask;
            probes += 1;
        }
        return false;
    }

    pub fn get(key: K): V? {
        if (_.len == 0) {
            return null;
        }
        const h = hashKey(key);
        let mask = CAP - 1;
        let idx = (h as usize) & mask;
        let probes: usize = 0;

        while (probes < CAP) {
            const slot = _.entries[idx];
            if (!slot.occupied) {
                return null;
            }
            if (slot.hash == h && keyEql(slot.key, key)) {
                return slot.value;
            }
            idx = (idx + 1) & mask;
            probes += 1;
        }
        return null;
    }

    pub fn has(key: K): bool {
        if (_.len == 0) {
            return false;
        }
        const h = hashKey(key);
        let mask = CAP - 1;
        let idx = (h as usize) & mask;
        let probes: usize = 0;

        while (probes < CAP) {
            const slot = _.entries[idx];
            if (!slot.occupied) {
                return false;
            }
            if (slot.hash == h && keyEql(slot.key, key)) {
                return true;
            }
            idx = (idx + 1) & mask;
            probes += 1;
        }
        return false;
    }

    pub fn delete(*_, key: K): bool {
        if (_.len == 0) {
            return false;
        }
        const h = hashKey(key);
        let mask = CAP - 1;
        let idx = (h as usize) & mask;
        let probes: usize = 0;

        while (probes < CAP) {
            let slot = *_.entries[idx];
            if (!slot.occupied) {
                return false;
            }
            if (slot.hash == h && keyEql(slot.key, key)) {
                slot.occupied = false;
                slot.hash = 0;
                _.len -= 1;
                // Rehash subsequent cluster elements to avoid broken probing chains
                let next_idx = (idx + 1) & mask;
                while (_.entries[next_idx].occupied) {
                    let rehash_entry = _.entries[next_idx];
                    _.entries[next_idx].occupied = false;
                    _.entries[next_idx].hash = 0;
                    _.len -= 1;
                    _.put(rehash_entry.key, rehash_entry.value);
                    next_idx = (next_idx + 1) & mask;
                }
                return true;
            }
            idx = (idx + 1) & mask;
            probes += 1;
        }
        return false;
    }
}
