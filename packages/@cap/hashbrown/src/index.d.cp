// Type definitions for CP

import { fnv1a64 } from "../../fnv-hash/src/index.cp";
export class HbGetResult {
    found: bool,
    value: i64,
}

export class HbInsertResult {
    ok: bool,
    replaced: bool,
    old_value: i64,
}

export fn hbHashU64(key: u64): u64;
export fn hbH2(h: u64): u8;
export class HashMapU64 {
    ctrl: [128]u8,
    keys: [128]u64,
    vals: [128]i64,
    used: usize,
    fn new(): HashMapU64;
    fn len(): usize;
    fn capacity(): usize;
    fn isEmpty(): bool;
    fn clear(_: auto);
    fn findSlot(key: u64): i32;
    fn containsKey(key: u64): bool;
    fn get(key: u64): HbGetResult;
    fn insert(_: auto, key: u64, value: i64): HbInsertResult;
    fn remove(_: auto, key: u64): HbGetResult;
}

export class HashSetU64 {
    inner: HashMapU64,
    fn new(): HashSetU64;
    fn len(): usize;
    fn isEmpty(): bool;
    fn clear(_: auto);
    fn insert(_: auto, key: u64): bool;
    fn contains(key: u64): bool;
    fn remove(_: auto, key: u64): bool;
}

