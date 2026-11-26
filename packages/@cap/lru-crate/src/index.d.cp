// Type definitions for CP

import { fnv1a64 } from "../../fnv-hash/src/index.cp";
export class LruNode {
    occupied: bool,
    key: string,
    val: i64,
    hash: u64,
    prev: usize,
    next: usize,
}

export fn lruStrEq(a: string, b: string): bool;
export class LruCacheI64 {
    nodes: [64]LruNode,
    cap: usize,
    count: usize,
    head: usize,
    tail: usize,
    fn withCapacity(max_cap: usize): LruCacheI64;
    fn len(): usize;
    fn capacity(): usize;
    fn isEmpty(): bool;
    fn findSlot(key: string, h: u64): usize;
    fn findFreeSlot(): usize;
    fn detach(_: auto, idx: usize);
    fn attachFront(_: auto, idx: usize);
    fn contains(key: string): bool;
    fn peek(key: string): i64?;
    fn peekLruKey(): string?;
    fn peekMruKey(): string?;
    fn get(_: auto, key: string): i64?;
    fn put(_: auto, key: string, val: i64): i64?;
    fn pop(_: auto, key: string): i64?;
    fn popLru(_: auto): i64?;
    fn clear(_: auto);
}

