// @cap/nohash-hasher — Zero-overhead identity & integer-key hashers in pure CP
// Clean-room Apache-2.0 implementation of crates.io `nohash-hasher` (#860, 133.0M downloads).

/// Identity hasher for integer keys (mirrors Rust `nohash_hasher::IntHasher`).
/// Stores the last written integer value verbatim as a `u64` hash state.
pub struct IntHasher {
    pub state: u64,

    pub static fn new(): IntHasher {
        return IntHasher { state: 0 };
    }

    pub fn writeU8(*_, v: u8) {
        _.state = v as u64;
    }

    pub fn writeU16(*_, v: u16) {
        _.state = v as u64;
    }

    pub fn writeU32(*_, v: u32) {
        _.state = v as u64;
    }

    pub fn writeU64(*_, v: u64) {
        _.state = v;
    }

    pub fn writeUsize(*_, v: usize) {
        _.state = v as u64;
    }

    pub fn writeI8(*_, v: i8) {
        const u: u8 = @bitCast(v);
        _.state = u as u64;
    }

    pub fn writeI16(*_, v: i16) {
        const u: u16 = @bitCast(v);
        _.state = u as u64;
    }

    pub fn writeI32(*_, v: i32) {
        const u: u32 = @bitCast(v);
        _.state = u as u64;
    }

    pub fn writeI64(*_, v: i64) {
        _.state = @bitCast(v);
    }

    pub fn writeIsize(*_, v: isize) {
        const u: usize = @bitCast(v);
        _.state = u as u64;
    }

    pub fn finish(): u64 {
        return _.state;
    }

    pub fn reset(*_) {
        _.state = 0;
    }

    pub fn clone(): IntHasher {
        return IntHasher { state: _.state };
    }
}

/// Alias matching crates.io `nohash-hasher` naming.
pub type NoHashHasher = IntHasher;

/// Factory mirroring Rust `BuildNoHashHasher<T>` / `BuildHasher`.
pub struct BuildNoHashHasher {
    pub seed: u64,

    pub static fn new(): BuildNoHashHasher {
        return BuildNoHashHasher { seed: 0 };
    }

    pub fn buildHasher(): IntHasher {
        return IntHasher { state: _.seed };
    }
}

pub fn newIntHasher(): IntHasher {
    return IntHasher { state: 0 };
}

pub fn newNoHashHasher(): NoHashHasher {
    return IntHasher { state: 0 };
}

/// Identity hash functions for `IsEnabled` integer key types (`u8`..`usize`, `i8`..`isize`).
pub fn nohashU8(key: u8): u64 {
    return key as u64;
}

pub fn nohashU16(key: u16): u64 {
    return key as u64;
}

/// Identity hash for u32 keys.
pub fn nohashU32(key: u32): u64 {
    return key as u64;
}

/// Identity hash for already-random integer keys (zero arithmetic overhead).
pub fn nohashU64(key: u64): u64 {
    return key;
}

pub fn nohashUsize(key: usize): u64 {
    return key as u64;
}

pub fn nohashI8(key: i8): u64 {
    const u: u8 = @bitCast(key);
    return u as u64;
}

pub fn nohashI16(key: i16): u64 {
    const u: u16 = @bitCast(key);
    return u as u64;
}

pub fn nohashI32(key: i32): u64 {
    const u: u32 = @bitCast(key);
    return u as u64;
}

pub fn nohashI64(key: i64): u64 {
    return @bitCast(key);
}

pub fn nohashIsize(key: isize): u64 {
    const u: usize = @bitCast(key);
    return u as u64;
}

/// Fast Fibonacci multiplication hash mapping a 64-bit integer key to a power-of-two bucket index (`0..2^bits - 1`).
pub fn fibBucketIndex(key: u64, bits: u6): usize {
    if (bits == 0) {
        return 0;
    }
    const shift: u6 = (64 - (bits as u32)) as u6;
    const golden: u64 = ((0x9E3779B9 as u64) << 32) | (0x7F4A7C15 as u64);
    const mixed = key *% golden;
    return (mixed >> shift) as usize;
}

/// Bijective SplitMix64 finalizer for dispersing sequential integer IDs into uniform 64-bit hashes.
pub fn mixU64(x: u64): u64 {
    const c1: u64 = ((0xBF58476D as u64) << 32) | (0x1CE4E5B9 as u64);
    const c2: u64 = ((0x94D049BB as u64) << 32) | (0x133111EB as u64);
    let z = x;
    z = (z ^ (z >> 30)) *% c1;
    z = (z ^ (z >> 27)) *% c2;
    return z ^ (z >> 31);
}

/// Exact inverse of `mixU64` (`unmixU64(mixU64(x)) == x` for all `u64`).
pub fn unmixU64(x: u64): u64 {
    const inv_c2: u64 = ((0x319642B2 as u64) << 32) | (0xD24D8EC3 as u64);
    const inv_c1: u64 = ((0x96DE1B17 as u64) << 32) | (0x3F119089 as u64);
    let z = x;
    z = z ^ (z >> 31) ^ (z >> 62);
    z = z *% inv_c2;
    z = z ^ (z >> 27) ^ (z >> 54);
    z = z *% inv_c1;
    z = z ^ (z >> 30) ^ (z >> 60);
    return z;
}

pub const INT_TABLE_CAP: usize = 64;
const SLOT_EMPTY: u8 = 0;
const SLOT_OCCUPIED: u8 = 1;
const SLOT_TOMBSTONE: u8 = 2;

/// Zero-heap fixed-capacity open-addressed `u64 -> u64` map using identity hashing (`IntHasher`).
pub struct IntMap64 {
    pub keys: [INT_TABLE_CAP]u64,
    pub vals: [INT_TABLE_CAP]u64,
    pub states: [INT_TABLE_CAP]u8,
    pub count: usize,

    pub static fn init(): IntMap64 {
        return IntMap64 {
            keys: [0; INT_TABLE_CAP],
            vals: [0; INT_TABLE_CAP],
            states: [SLOT_EMPTY; INT_TABLE_CAP],
            count: 0,
        };
    }

    pub fn len(): usize {
        return _.count;
    }

    pub fn isEmpty(): bool {
        return _.count == 0;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < INT_TABLE_CAP) : (i += 1) {
            _.states[i] = SLOT_EMPTY;
        }
        _.count = 0;
    }

    /// Inserts or updates `(key, val)`. Returns `true` on success, `false` if table is full.
    pub fn insert(*_, key: u64, val: u64): bool {
        let h = newIntHasher();
        h.writeU64(key);
        const start: usize = (h.finish() as usize) & (INT_TABLE_CAP - 1);
        let first_tomb: usize = INT_TABLE_CAP;
        let step: usize = 0;
        while (step < INT_TABLE_CAP) : (step += 1) {
            const idx: usize = (start + step) & (INT_TABLE_CAP - 1);
            const st = _.states[idx];
            if (st == SLOT_OCCUPIED) {
                if (_.keys[idx] == key) {
                    _.vals[idx] = val;
                    return true;
                }
            } else if (st == SLOT_TOMBSTONE) {
                if (first_tomb == INT_TABLE_CAP) {
                    first_tomb = idx;
                }
            } else {
                const target: usize = if (first_tomb < INT_TABLE_CAP) first_tomb else idx;
                _.keys[target] = key;
                _.vals[target] = val;
                _.states[target] = SLOT_OCCUPIED;
                _.count += 1;
                return true;
            }
        }
        if (first_tomb < INT_TABLE_CAP) {
            _.keys[first_tomb] = key;
            _.vals[first_tomb] = val;
            _.states[first_tomb] = SLOT_OCCUPIED;
            _.count += 1;
            return true;
        }
        return false;
    }

    pub fn get(key: u64): u64? {
        let h = newIntHasher();
        h.writeU64(key);
        const start: usize = (h.finish() as usize) & (INT_TABLE_CAP - 1);
        let step: usize = 0;
        while (step < INT_TABLE_CAP) : (step += 1) {
            const idx: usize = (start + step) & (INT_TABLE_CAP - 1);
            const st = _.states[idx];
            if (st == SLOT_EMPTY) {
                return null;
            }
            if (st == SLOT_OCCUPIED && _.keys[idx] == key) {
                return _.vals[idx];
            }
        }
        return null;
    }

    pub fn containsKey(key: u64): bool {
        return _.get(key) != null;
    }

    pub fn remove(*_, key: u64): u64? {
        let h = newIntHasher();
        h.writeU64(key);
        const start: usize = (h.finish() as usize) & (INT_TABLE_CAP - 1);
        let step: usize = 0;
        while (step < INT_TABLE_CAP) : (step += 1) {
            const idx: usize = (start + step) & (INT_TABLE_CAP - 1);
            const st = _.states[idx];
            if (st == SLOT_EMPTY) {
                return null;
            }
            if (st == SLOT_OCCUPIED && _.keys[idx] == key) {
                const old = _.vals[idx];
                _.states[idx] = SLOT_TOMBSTONE;
                _.count -= 1;
                return old;
            }
        }
        return null;
    }
}

/// Zero-heap fixed-capacity open-addressed `u64` set using identity hashing (`IntHasher`).
pub struct IntSet64 {
    pub keys: [INT_TABLE_CAP]u64,
    pub states: [INT_TABLE_CAP]u8,
    pub count: usize,

    pub static fn init(): IntSet64 {
        return IntSet64 {
            keys: [0; INT_TABLE_CAP],
            states: [SLOT_EMPTY; INT_TABLE_CAP],
            count: 0,
        };
    }

    pub fn len(): usize {
        return _.count;
    }

    pub fn isEmpty(): bool {
        return _.count == 0;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < INT_TABLE_CAP) : (i += 1) {
            _.states[i] = SLOT_EMPTY;
        }
        _.count = 0;
    }

    /// Inserts `key`. Returns `true` if newly inserted or already present, `false` if full.
    pub fn insert(*_, key: u64): bool {
        let h = newIntHasher();
        h.writeU64(key);
        const start: usize = (h.finish() as usize) & (INT_TABLE_CAP - 1);
        let first_tomb: usize = INT_TABLE_CAP;
        let step: usize = 0;
        while (step < INT_TABLE_CAP) : (step += 1) {
            const idx: usize = (start + step) & (INT_TABLE_CAP - 1);
            const st = _.states[idx];
            if (st == SLOT_OCCUPIED) {
                if (_.keys[idx] == key) {
                    return true;
                }
            } else if (st == SLOT_TOMBSTONE) {
                if (first_tomb == INT_TABLE_CAP) {
                    first_tomb = idx;
                }
            } else {
                const target: usize = if (first_tomb < INT_TABLE_CAP) first_tomb else idx;
                _.keys[target] = key;
                _.states[target] = SLOT_OCCUPIED;
                _.count += 1;
                return true;
            }
        }
        if (first_tomb < INT_TABLE_CAP) {
            _.keys[first_tomb] = key;
            _.states[first_tomb] = SLOT_OCCUPIED;
            _.count += 1;
            return true;
        }
        return false;
    }

    pub fn contains(key: u64): bool {
        let h = newIntHasher();
        h.writeU64(key);
        const start: usize = (h.finish() as usize) & (INT_TABLE_CAP - 1);
        let step: usize = 0;
        while (step < INT_TABLE_CAP) : (step += 1) {
            const idx: usize = (start + step) & (INT_TABLE_CAP - 1);
            const st = _.states[idx];
            if (st == SLOT_EMPTY) {
                return false;
            }
            if (st == SLOT_OCCUPIED && _.keys[idx] == key) {
                return true;
            }
        }
        return false;
    }

    pub fn remove(*_, key: u64): bool {
        let h = newIntHasher();
        h.writeU64(key);
        const start: usize = (h.finish() as usize) & (INT_TABLE_CAP - 1);
        let step: usize = 0;
        while (step < INT_TABLE_CAP) : (step += 1) {
            const idx: usize = (start + step) & (INT_TABLE_CAP - 1);
            const st = _.states[idx];
            if (st == SLOT_EMPTY) {
                return false;
            }
            if (st == SLOT_OCCUPIED && _.keys[idx] == key) {
                _.states[idx] = SLOT_TOMBSTONE;
                _.count -= 1;
                return true;
            }
        }
        return false;
    }
}

export {
    INT_TABLE_CAP,
    IntHasher,
    NoHashHasher,
    BuildNoHashHasher,
    newIntHasher,
    newNoHashHasher,
    nohashU8,
    nohashU16,
    nohashU32,
    nohashU64,
    nohashUsize,
    nohashI8,
    nohashI16,
    nohashI32,
    nohashI64,
    nohashIsize,
    fibBucketIndex,
    mixU64,
    unmixU64,
    IntMap64,
    IntSet64,
};
