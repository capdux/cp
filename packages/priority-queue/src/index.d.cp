// Type definitions for CP

export class PqItem {
    id: u16,
    priority: i64,
    valid: bool,
}

export class IndexedMinHeap {
    heap_ids: [MAX_ITEMS]u16,
    priorities: [MAX_ITEMS]i64,
    pos: [MAX_ITEMS]i16,
    size: usize,
    fn init(): IndexedMinHeap;
    fn len(): usize;
    fn isEmpty(): bool;
    fn contains(id: u16): bool;
    fn priorityOf(id: u16): i64;
    fn peek(): PqItem;
    fn lessAt(i: usize, j: usize): bool;
    fn swapAt(_: auto, i: usize, j: usize);
    fn siftUp(_: auto, idx_in: usize);
    fn siftDown(_: auto, idx_in: usize);
    fn changePriority(_: auto, id: u16, new_priority: i64): bool;
    fn push(_: auto, id: u16, priority: i64): bool;
    fn remove(_: auto, id: u16): bool;
    fn pop(_: auto): PqItem;
    fn clear(_: auto);
}

