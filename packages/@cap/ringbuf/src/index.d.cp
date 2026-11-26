// Type definitions for CP

export class RingDequeI64 {
    data: [64]i64,
    head: usize,
    count: usize,
    fn init(): RingDequeI64;
    fn len(): usize;
    fn capacity(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn clear(_: auto);
    fn pushBack(_: auto, v: i64): bool;
    fn pushFront(_: auto, v: i64): bool;
    fn pushBackOverwrite(_: auto, v: i64): i64?;
    fn popFront(_: auto): i64?;
    fn popBack(_: auto): i64?;
    fn front(): i64?;
    fn back(): i64?;
    fn get(idx: usize): i64?;
}

export class ByteRingBuf {
    data: [512]u8,
    head: usize,
    count: usize,
    fn init(): ByteRingBuf;
    fn len(): usize;
    fn freeSpace(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn clear(_: auto);
    fn writeByte(_: auto, b: u8): bool;
    fn readByte(_: auto): u8?;
    fn peekByte(offset: usize): u8?;
    fn writeBytes(_: auto, data: string): usize;
    fn readBytes(_: auto, out: []u8): string;
}

