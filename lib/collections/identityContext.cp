// Source: Absorbed from /data/dev/bun/src/collections/identity_context.cp | Synced: 2026-11-23T21:08:11Z
// ============================================================================
// lib/collections/identityContext.cp — Zero-Rehash Identity Hash Contexts & Tables
// ============================================================================
// Avoids double-hashing pre-hashed integer keys (e.g. Wyhash / XxHash64 digests),
// aligned with Bun's `IdentityContext` and `ArrayIdentityContext`, plus zero-alloc
// fixed-capacity open-addressing `IdentitySetU64` and `IdentityMapU64`.

pub const IDENTITY_TABLE_SLOTS: usize = 64;
const SLOT_MASK: usize = IDENTITY_TABLE_SLOTS - 1;
const SLOT_EMPTY: u8 = 0;
const SLOT_OCCUPIED: u8 = 1;
const SLOT_TOMBSTONE: u8 = 2;

pub err IdentityMapError {
    TableFull,
}

/// Identity hash context for 64-bit pre-hashed keys (no re-hashing).
pub struct IdentityContextU64 {
    static fn hash(key: u64): u64 {
        return key;
    }

    static fn eql(a: u64, b: u64): bool {
        return a == b;
    }
}

/// Identity hash context for 32-bit integer / enum keys (widens to u64 without mixing).
pub struct IdentityContextU32 {
    static fn hash(key: u32): u64 {
        return @as(u64, key);
    }

    static fn eql(a: u32, b: u32): bool {
        return a == b;
    }
}

/// ArrayHashMap-compatible 32-bit identity context (matches Bun `ArrayIdentityContext`).
pub struct ArrayIdentityContext {
    static fn hash(key: u32): u32 {
        return key;
    }

    static fn eql(a: u32, b: u32, idx: usize): bool {
        * = idx;
        return a == b;
    }
}

/// ArrayHashMap-compatible 64-bit -> 32-bit identity context (matches Bun `ArrayIdentityContext.U64`).
pub struct ArrayIdentityContextU64 {
    static fn hash(key: u64): u32 {
        return @as(u32, @intCast(key & 0xFFFFFFFF));
    }

    static fn eql(a: u64, b: u64, idx: usize): bool {
        * = idx;
        return a == b;
    }
}

/// Direct bucket index for a pre-hashed `u64` key given a power-of-two `capacity_mask`.
pub fn identityBucketIndexU64(key: u64, capacity_mask: usize): usize {
    return @as(usize, @intCast(key & @as(u64, @intCast(capacity_mask))));
}

/// Direct bucket index for a pre-hashed `u32` key given a power-of-two `capacity_mask`.
pub fn identityBucketIndexU32(key: u32, capacity_mask: usize): usize {
    return @as(usize, key) & capacity_mask;
}

/// Zero-alloc fixed-capacity open-addressing hash set for pre-hashed `u64` keys.
/// Uses direct identity bucket indexing (no secondary hashing) and tombstone recycling.
pub struct IdentitySetU64 {
    keys: [IDENTITY_TABLE_SLOTS]u64 = [0; IDENTITY_TABLE_SLOTS],
    states: [IDENTITY_TABLE_SLOTS]u8 = [0; IDENTITY_TABLE_SLOTS],
    count: usize = 0,
    tombstones: usize = 0,

    static fn init(): IdentitySetU64 {
        return IdentitySetU64{};
    }

    fn len(): usize {
        return _.count;
    }

    fn isEmpty(): bool {
        return _.count == 0;
    }

    fn capacity(): usize {
        return IDENTITY_TABLE_SLOTS;
    }

    fn contains(key: u64): bool {
        const start = identityBucketIndexU64(key, SLOT_MASK);
        let step: usize = 0;
        while (step < IDENTITY_TABLE_SLOTS) {
            const idx = (start + step) & SLOT_MASK;
            const st = _.states[idx];
            if (st == SLOT_EMPTY) return false;
            if (st == SLOT_OCCUPIED && IdentityContextU64.eql(_.keys[idx], key)) return true;
            step += 1;
        }
        return false;
    }

    /// Inserts `key`. Returns `true` if newly inserted, `false` if already present,
    /// or `IdentityMapError.TableFull` if all slots are occupied.
    fn insert(*_, key: u64): IdentityMapError!bool {
        const start = identityBucketIndexU64(key, SLOT_MASK);
        let first_tombstone: usize = IDENTITY_TABLE_SLOTS;
        let step: usize = 0;
        while (step < IDENTITY_TABLE_SLOTS) {
            const idx = (start + step) & SLOT_MASK;
            const st = _.states[idx];
            if (st == SLOT_EMPTY) {
                const dst = if (first_tombstone < IDENTITY_TABLE_SLOTS) first_tombstone else idx;
                if (dst == first_tombstone) _.tombstones -= 1;
                _.keys[dst] = key;
                _.states[dst] = SLOT_OCCUPIED;
                _.count += 1;
                return true;
            }
            if (st == SLOT_OCCUPIED && IdentityContextU64.eql(_.keys[idx], key)) {
                return false;
            }
            if (st == SLOT_TOMBSTONE && first_tombstone == IDENTITY_TABLE_SLOTS) {
                first_tombstone = idx;
            }
            step += 1;
        }
        if (first_tombstone < IDENTITY_TABLE_SLOTS) {
            _.keys[first_tombstone] = key;
            _.states[first_tombstone] = SLOT_OCCUPIED;
            _.count += 1;
            _.tombstones -= 1;
            return true;
        }
        return IdentityMapError.TableFull;
    }

    fn remove(*_, key: u64): bool {
        const start = identityBucketIndexU64(key, SLOT_MASK);
        let step: usize = 0;
        while (step < IDENTITY_TABLE_SLOTS) {
            const idx = (start + step) & SLOT_MASK;
            const st = _.states[idx];
            if (st == SLOT_EMPTY) return false;
            if (st == SLOT_OCCUPIED && IdentityContextU64.eql(_.keys[idx], key)) {
                _.states[idx] = SLOT_TOMBSTONE;
                _.count -= 1;
                _.tombstones += 1;
                return true;
            }
            step += 1;
        }
        return false;
    }

    fn clear(*_) {
        let i: usize = 0;
        while (i < IDENTITY_TABLE_SLOTS) {
            _.keys[i] = 0;
            _.states[i] = SLOT_EMPTY;
            i += 1;
        }
        _.count = 0;
        _.tombstones = 0;
    }
}

/// Zero-alloc fixed-capacity open-addressing map (`u64` pre-hashed key -> `u64` value).
pub struct IdentityMapU64 {
    keys: [IDENTITY_TABLE_SLOTS]u64 = [0; IDENTITY_TABLE_SLOTS],
    values: [IDENTITY_TABLE_SLOTS]u64 = [0; IDENTITY_TABLE_SLOTS],
    states: [IDENTITY_TABLE_SLOTS]u8 = [0; IDENTITY_TABLE_SLOTS],
    count: usize = 0,
    tombstones: usize = 0,

    static fn init(): IdentityMapU64 {
        return IdentityMapU64{};
    }

    fn len(): usize {
        return _.count;
    }

    fn isEmpty(): bool {
        return _.count == 0;
    }

    fn capacity(): usize {
        return IDENTITY_TABLE_SLOTS;
    }

    fn get(key: u64): u64? {
        const start = identityBucketIndexU64(key, SLOT_MASK);
        let step: usize = 0;
        while (step < IDENTITY_TABLE_SLOTS) {
            const idx = (start + step) & SLOT_MASK;
            const st = _.states[idx];
            if (st == SLOT_EMPTY) return null;
            if (st == SLOT_OCCUPIED && IdentityContextU64.eql(_.keys[idx], key)) return _.values[idx];
            step += 1;
        }
        return null;
    }

    fn contains(key: u64): bool {
        return _.get(key) != null;
    }

    /// Inserts or updates `(key, value)`. Returns `true` if newly inserted, `false` if updated.
    fn put(*_, key: u64, value: u64): IdentityMapError!bool {
        const start = identityBucketIndexU64(key, SLOT_MASK);
        let first_tombstone: usize = IDENTITY_TABLE_SLOTS;
        let step: usize = 0;
        while (step < IDENTITY_TABLE_SLOTS) {
            const idx = (start + step) & SLOT_MASK;
            const st = _.states[idx];
            if (st == SLOT_EMPTY) {
                const dst = if (first_tombstone < IDENTITY_TABLE_SLOTS) first_tombstone else idx;
                if (dst == first_tombstone) _.tombstones -= 1;
                _.keys[dst] = key;
                _.values[dst] = value;
                _.states[dst] = SLOT_OCCUPIED;
                _.count += 1;
                return true;
            }
            if (st == SLOT_OCCUPIED && IdentityContextU64.eql(_.keys[idx], key)) {
                _.values[idx] = value;
                return false;
            }
            if (st == SLOT_TOMBSTONE && first_tombstone == IDENTITY_TABLE_SLOTS) {
                first_tombstone = idx;
            }
            step += 1;
        }
        if (first_tombstone < IDENTITY_TABLE_SLOTS) {
            _.keys[first_tombstone] = key;
            _.values[first_tombstone] = value;
            _.states[first_tombstone] = SLOT_OCCUPIED;
            _.count += 1;
            _.tombstones -= 1;
            return true;
        }
        return IdentityMapError.TableFull;
    }

    fn remove(*_, key: u64): u64? {
        const start = identityBucketIndexU64(key, SLOT_MASK);
        let step: usize = 0;
        while (step < IDENTITY_TABLE_SLOTS) {
            const idx = (start + step) & SLOT_MASK;
            const st = _.states[idx];
            if (st == SLOT_EMPTY) return null;
            if (st == SLOT_OCCUPIED && IdentityContextU64.eql(_.keys[idx], key)) {
                const prev = _.values[idx];
                _.states[idx] = SLOT_TOMBSTONE;
                _.count -= 1;
                _.tombstones += 1;
                return prev;
            }
            step += 1;
        }
        return null;
    }

    fn clear(*_) {
        let i: usize = 0;
        while (i < IDENTITY_TABLE_SLOTS) {
            _.keys[i] = 0;
            _.values[i] = 0;
            _.states[i] = SLOT_EMPTY;
            i += 1;
        }
        _.count = 0;
        _.tombstones = 0;
    }
}
