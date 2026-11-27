// Type definitions for CP

export class FifoI64 {
    buf: [256]i64,
    head: usize,
    tail: usize,
    count: usize,
    fn len(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn push(_: auto, val: i64): bool;
    fn shift(_: auto): i64?;
    fn peek(): i64?;
    fn clear(_: auto);
}

export fn newFifoI64(): FifoI64;
export class FifoStr {
    buf: [256]string,
    head: usize,
    tail: usize,
    count: usize,
    fn len(): usize;
    fn isEmpty(): bool;
    fn push(_: auto, val: string): bool;
    fn shift(_: auto): string?;
    fn peek(): string?;
    fn clear(_: auto);
}

export fn newFifoStr(): FifoStr;
