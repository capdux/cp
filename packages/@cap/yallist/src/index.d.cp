// Type definitions for CP

export fn yalValidHandle(handle: i32): bool;
export class YallistU64 {
    values: [64]u64,
    prev: [64]i32,
    next: [64]i32,
    alive: [64]bool,
    free_stack: [64]i32,
    free_top: usize,
    head: i32,
    tail: i32,
    length: usize,
    fn new(): YallistU64;
    fn len(): usize;
    fn capacity(): usize;
    fn isEmpty(): bool;
    fn push(_: auto, val: u64): i32;
    fn pop(_: auto): u64?;
    fn unshift(_: auto, val: u64): i32;
    fn shift(_: auto): u64?;
    fn get(idx: usize): u64?;
    fn headVal(): u64?;
    fn tailVal(): u64?;
    fn removeNode(_: auto, handle: i32): bool;
    fn unshiftNode(_: auto, handle: i32): bool;
    fn pushNode(_: auto, handle: i32): bool;
    fn reverse(_: auto);
    fn toArray(out: []u64): usize;
}

