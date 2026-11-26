// Type definitions for CP

export fn slmIsOccupiedVersion(version: u32): bool;
export fn slmAllocVersion(cur_version: u32): u32;
export fn slmFreeVersion(cur_version: u32): u32;
export class SlotKey {
    idx: u32,
    version: u32,
    fn nullKey(): SlotKey;
    fn isNull(): bool;
    fn eq(other: SlotKey): bool;
    fn toU64(): u64;
    fn fromU64(raw: u64): SlotKey;
}

export class SlotMapI64 {
    values: [128]i64,
    versions: [128]u32,
    next_free: [128]u32,
    free_head: u32,
    count: usize,
    fn new(): SlotMapI64;
    fn len(): usize;
    fn capacity(): usize;
    fn isEmpty(): bool;
    fn containsKey(key: SlotKey): bool;
    fn insert(_: auto, val: i64): SlotKey?;
    fn get(key: SlotKey): i64?;
    fn set(_: auto, key: SlotKey, val: i64): bool;
    fn remove(_: auto, key: SlotKey): i64?;
    fn clear(_: auto);
}

export class SecondaryMapI64 {
    values: [128]i64,
    versions: [128]u32,
    count: usize,
    fn new(): SecondaryMapI64;
    fn len(): usize;
    fn containsKey(key: SlotKey): bool;
    fn insert(_: auto, key: SlotKey, val: i64): bool;
    fn get(key: SlotKey): i64?;
    fn remove(_: auto, key: SlotKey): i64?;
    fn clear(_: auto);
}

