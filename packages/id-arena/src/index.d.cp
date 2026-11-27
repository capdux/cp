// Type definitions for CP

export class ArenaId {
    index: usize,
    arena_id: u32,
}

export fn idaIsValid(id: ArenaId, expected_arena_id: u32, count: usize): bool;
export class IdArenaI64 {
    arena_id: u32,
    items: [128]i64,
    count: usize,
    fn new(arena_id: u32): IdArenaI64;
    fn alloc(_: auto, val: i64): ArenaId?;
    fn get(id: ArenaId): i64?;
    fn set(_: auto, id: ArenaId, val: i64): bool;
    fn len(): usize;
    fn isEmpty(): bool;
    fn capacity(): usize;
    fn contains(id: ArenaId): bool;
    fn clear(_: auto);
}

export class IdArenaU64 {
    arena_id: u32,
    items: [128]u64,
    count: usize,
    fn new(arena_id: u32): IdArenaU64;
    fn alloc(_: auto, val: u64): ArenaId?;
    fn get(id: ArenaId): u64?;
    fn set(_: auto, id: ArenaId, val: u64): bool;
    fn len(): usize;
    fn isEmpty(): bool;
    fn capacity(): usize;
    fn contains(id: ArenaId): bool;
    fn clear(_: auto);
}

