// Type definitions for CP

export class SlabKey {
    index: u32,
    generation: u32,
    fn equals(other: SlabKey): bool;
}

export fn nextGeneration(gen: u32): u32;
export class SlabI64 {
    slots: [SLAB_CAPACITY]i64,
    occupied: [SLAB_CAPACITY]bool,
    generations: [SLAB_CAPACITY]u32,
    next_free: [SLAB_CAPACITY]i32,
    free_head: i32,
    next_alloc: usize,
    len: usize,
    capacity: usize,
    fn init(): SlabI64;
    fn insert(_: auto, value: i64): SlabKey?;
    fn contains(key: SlabKey): bool;
    fn get(key: SlabKey): i64?;
    fn set(_: auto, key: SlabKey, value: i64): bool;
    fn remove(_: auto, key: SlabKey): i64?;
    fn count(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn clear(_: auto);
}

export class SlabU64 {
    slots: [SLAB_CAPACITY]u64,
    occupied: [SLAB_CAPACITY]bool,
    generations: [SLAB_CAPACITY]u32,
    next_free: [SLAB_CAPACITY]i32,
    free_head: i32,
    next_alloc: usize,
    len: usize,
    capacity: usize,
    fn init(): SlabU64;
    fn insert(_: auto, value: u64): SlabKey?;
    fn contains(key: SlabKey): bool;
    fn get(key: SlabKey): u64?;
    fn set(_: auto, key: SlabKey, value: u64): bool;
    fn remove(_: auto, key: SlabKey): u64?;
    fn count(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn clear(_: auto);
}

