// Type definitions for CP

import { fnv1a64 } from "../../fnv-hash/src/index.cp";
export class QlruSlot {
    occupied: bool,
    key: string,
    val: i64,
    hash: u64,
}

export fn qlruStrEq(a: string, b: string): bool;
export class QuickLruI64 {
    cur: [32]QlruSlot,
    old: [32]QlruSlot,
    cur_len: usize,
    old_len: usize,
    max_size: usize,
    fn new(max_entries: usize): QuickLruI64;
    fn maxSize(): usize;
    fn size(): usize;
    fn findInCur(key: string, h: u64): usize;
    fn findInOld(key: string, h: u64): usize;
    fn rotateGenerations(_: auto);
    fn insertCur(_: auto, key: string, val: i64, h: u64);
    fn set(_: auto, key: string, val: i64);
    fn has(key: string): bool;
    fn peek(key: string): i64?;
    fn get(_: auto, key: string): i64?;
    fn delete(_: auto, key: string): bool;
    fn clear(_: auto);
}

