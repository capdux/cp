// Type definitions for CP

export class SkipEntry {
    key: i64,
    value: i64,
    found: bool,
    fn none(): SkipEntry;
}

export class SkipNode {
    key: i64,
    value: i64,
    level: u8,
    next: [8]u16,
}

export fn emptyNode(): SkipNode;
export class SkipList {
    nodes: [129]SkipNode,
    free_stack: [128]u16,
    free_top: usize,
    cur_level: u8,
    count: usize,
    rng_state: u64,
    fn init(): SkipList;
    fn len(): usize;
    fn isEmpty(): bool;
    fn clear(_: auto);
    fn randomLevel(_: auto): u8;
    fn insert(_: auto, key: i64, value: i64): bool;
    fn findNodeGE(key: i64): u16;
    fn get(key: i64): SkipEntry;
    fn contains(key: i64): bool;
    fn remove(_: auto, key: i64): bool;
    fn first(): SkipEntry;
    fn last(): SkipEntry;
    fn lowerBound(key: i64): SkipEntry;
    fn rangeCount(min_key: i64, max_key: i64): usize;
}

