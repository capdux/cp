// Type definitions for CP

import { fnv1a64 } from "../../fnv-hash/src/index.cp";
export fn imHashU64(key: u64): usize;
export fn imHashStr(key: string): usize;
export fn imStrEq(a: &[]u8, b: &[]u8): bool;
export class IndexMapEntry {
    key: u64,
    val: i64,
}

export class InsertResult {
    ok: bool,
    replaced: bool,
    old_val: i64,
    index: usize,
}

export class GetValResult {
    found: bool,
    val: i64,
}

export class GetEntryResult {
    found: bool,
    key: u64,
    val: i64,
}

export class IndexMapStrEntry {
    key_buf: [64]u8,
    key_len: usize,
    val: i64,
    fn keyStr(): &[]u8;
}

export class GetStrEntryResult {
    found: bool,
    key_buf: [64]u8,
    key_len: usize,
    val: i64,
    fn keyStr(): &[]u8;
}

export class IndexMapU64 {
    entries: [64]IndexMapEntry,
    buckets: [128]i32,
    used: usize,
    fn new(): IndexMapU64;
    fn len(): usize;
    fn capacity(): usize;
    fn isEmpty(): bool;
    fn clear(_: auto);
    fn getIndexOf(key: u64): i64;
    fn containsKey(key: u64): bool;
    fn get(key: u64): GetValResult;
    fn getIndex(idx: usize): GetEntryResult;
    fn first(): GetEntryResult;
    fn last(): GetEntryResult;
    fn insert(_: auto, key: u64, val: i64): InsertResult;
    fn swapRemove(_: auto, key: u64): GetValResult;
    fn shiftRemove(_: auto, key: u64): GetValResult;
    fn pop(_: auto): GetEntryResult;
}

export class IndexMapStr {
    entries: [32]IndexMapStrEntry,
    buckets: [64]i32,
    used: usize,
    fn new(): IndexMapStr;
    fn len(): usize;
    fn capacity(): usize;
    fn isEmpty(): bool;
    fn clear(_: auto);
    fn getIndexOf(key: string): i64;
    fn containsKey(key: string): bool;
    fn get(key: string): GetValResult;
    fn getIndex(idx: usize): GetStrEntryResult;
    fn first(): GetStrEntryResult;
    fn last(): GetStrEntryResult;
    fn insert(_: auto, key: string, val: i64): InsertResult;
    fn swapRemove(_: auto, key: string): GetValResult;
    fn shiftRemove(_: auto, key: string): GetValResult;
    fn pop(_: auto): GetStrEntryResult;
}

