// @cap/id-arena — Zero-allocation typed handle arena allocator
// Inspired by crates.io:id-arena (https://github.com/fitzgen/id-arena)

pub const ARENA_CAP: usize = 128;

pub struct ArenaId {
    index: usize,
    arena_id: u32,
}

fn idaIsValid(id: ArenaId, expected_arena_id: u32, count: usize): bool {
    return id.arena_id == expected_arena_id && id.index < count;
}

/// Zero-heap typed handle arena storing up to `ARENA_CAP` (128) `i64` values.
/// Validates `id.arena_id` on every lookup/mutation to prevent cross-arena misuse.
pub struct IdArenaI64 {
    arena_id: u32,
    items: [128]i64,
    count: usize,

    static fn new(arena_id: u32): IdArenaI64 {
        return IdArenaI64{
            arena_id: arena_id,
            items: [0; 128],
            count: 0,
        };
    }

    fn alloc(*_, val: i64): ArenaId? {
        if (_.count >= ARENA_CAP) {
            return null;
        }
        const idx: usize = _.count;
        _.items[idx] = val;
        _.count += 1;
        return ArenaId{
            index: idx,
            arena_id: _.arena_id,
        };
    }

    fn get(id: ArenaId): i64? {
        if (!idaIsValid(id, _.arena_id, _.count)) {
            return null;
        }
        return _.items[id.index];
    }

    fn set(*_, id: ArenaId, val: i64): bool {
        if (!idaIsValid(id, _.arena_id, _.count)) {
            return false;
        }
        _.items[id.index] = val;
        return true;
    }

    fn len(): usize {
        return _.count;
    }

    fn isEmpty(): bool {
        return _.count == 0;
    }

    fn capacity(): usize {
        return ARENA_CAP;
    }

    fn contains(id: ArenaId): bool {
        return idaIsValid(id, _.arena_id, _.count);
    }

    fn clear(*_) {
        _.count = 0;
    }
}

/// Zero-heap typed handle arena storing up to `ARENA_CAP` (128) `u64` values.
/// Validates `id.arena_id` on every lookup/mutation to prevent cross-arena misuse.
pub struct IdArenaU64 {
    arena_id: u32,
    items: [128]u64,
    count: usize,

    static fn new(arena_id: u32): IdArenaU64 {
        return IdArenaU64{
            arena_id: arena_id,
            items: [0; 128],
            count: 0,
        };
    }

    fn alloc(*_, val: u64): ArenaId? {
        if (_.count >= ARENA_CAP) {
            return null;
        }
        const idx: usize = _.count;
        _.items[idx] = val;
        _.count += 1;
        return ArenaId{
            index: idx,
            arena_id: _.arena_id,
        };
    }

    fn get(id: ArenaId): u64? {
        if (!idaIsValid(id, _.arena_id, _.count)) {
            return null;
        }
        return _.items[id.index];
    }

    fn set(*_, id: ArenaId, val: u64): bool {
        if (!idaIsValid(id, _.arena_id, _.count)) {
            return false;
        }
        _.items[id.index] = val;
        return true;
    }

    fn len(): usize {
        return _.count;
    }

    fn isEmpty(): bool {
        return _.count == 0;
    }

    fn capacity(): usize {
        return ARENA_CAP;
    }

    fn contains(id: ArenaId): bool {
        return idaIsValid(id, _.arena_id, _.count);
    }

    fn clear(*_) {
        _.count = 0;
    }
}
