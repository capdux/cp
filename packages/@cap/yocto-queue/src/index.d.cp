// Type definitions for CP

export class PopU64Result {
    ok: bool,
    value: u64,
}

export class PopI64Result {
    ok: bool,
    value: i64,
}

export class YoctoQueueU64 {
    buf: [256]u64,
    head: usize,
    tail: usize,
    count: usize,
    fn new(): YoctoQueueU64;
    fn size(): usize;
    fn capacity(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn enqueue(_: auto, value: u64): bool;
    fn dequeue(_: auto): PopU64Result;
    fn peek(): PopU64Result;
    fn peekAt(idx: usize): PopU64Result;
    fn clear(_: auto);
    fn drain(_: auto, out: []u64): usize;
}

export class YoctoQueueI64 {
    buf: [256]i64,
    head: usize,
    tail: usize,
    count: usize,
    fn new(): YoctoQueueI64;
    fn size(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn enqueue(_: auto, value: i64): bool;
    fn dequeue(_: auto): PopI64Result;
    fn peek(): PopI64Result;
    fn clear(_: auto);
}

