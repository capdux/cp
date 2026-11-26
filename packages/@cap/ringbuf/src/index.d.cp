// Type definitions for CP

export class RingBufU8 {
    buf: [256]u8,
    head: usize,
    tail: usize,
    count: usize,
    fn new(): RingBufU8;
    fn len(): usize;
    fn capacity(): usize;
    fn freeLen(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn push(_: auto, b: u8): bool;
    fn pushOverwrite(_: auto, b: u8): u8?;
    fn pop(_: auto): u8?;
    fn peek(): u8?;
    fn pushSlice(_: auto, src: &[]u8): usize;
    fn popSlice(_: auto, dst: []u8): usize;
    fn clear(_: auto);
}

export class RingBufI64 {
    buf: [64]i64,
    head: usize,
    tail: usize,
    count: usize,
    fn new(): RingBufI64;
    fn len(): usize;
    fn capacity(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn push(_: auto, val: i64): bool;
    fn pushOverwrite(_: auto, val: i64): i64?;
    fn pop(_: auto): i64?;
    fn peek(): i64?;
    fn clear(_: auto);
}

