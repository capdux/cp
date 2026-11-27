// Type definitions for CP

export class RingQueueU64 {
    head: usize,
    tail: usize,
    count: usize,
    capacity: usize,
    fn new(cap: usize): RingQueueU64;
    fn len(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn remaining(): usize;
    fn clear(_: auto);
    fn push(_: auto, buf: []u64, val: u64): bool;
    fn forcePush(_: auto, buf: []u64, val: u64): bool;
    fn pop(_: auto, buf: &[]u64, out: &mut u64): bool;
    fn peek(buf: &[]u64, out: &mut u64): bool;
}

export class ByteRing {
    head: usize,
    tail: usize,
    count: usize,
    capacity: usize,
    fn new(cap: usize): ByteRing;
    fn len(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn remaining(): usize;
    fn clear(_: auto);
    fn writeBytes(_: auto, buf: []u8, src: &[]u8): usize;
    fn readBytes(_: auto, buf: &[]u8, dst: []u8): usize;
}

