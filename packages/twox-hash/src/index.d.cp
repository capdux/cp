// Type definitions for CP

import { readU32LE, readU64LE } from "cap:bin";
export fn xxhU64(hi: u32, lo: u32): u64;
export fn xxhAdd32(a: u32, b: u32): u32;
export fn xxhSub32(a: u32, b: u32): u32;
export fn xxhMul32(a: u32, b: u32): u32;
export fn xxhRol32(x: u32, r: u32): u32;
export fn xxhAdd64(a: u64, b: u64): u64;
export fn xxhSub64(a: u64, b: u64): u64;
export fn xxhMul64(a: u64, b: u64): u64;
export fn xxhRol64(x: u64, r: u64): u64;
export fn xxh32Round(acc: u32, lane: u32): u32;
export fn xxh64Prime1(): u64;
export fn xxh64Prime2(): u64;
export fn xxh64Prime3(): u64;
export fn xxh64Prime4(): u64;
export fn xxh64Prime5(): u64;
export fn xxh64Round(acc: u64, lane: u64): u64;
export fn xxh64MergeRound(acc: u64, val: u64): u64;
export class XxHash32 {
    seed: u32,
    v1: u32,
    v2: u32,
    v3: u32,
    v4: u32,
    buf: [16]u8,
    buf_len: usize,
    total_len: u64,
    fn withSeed(seed: u32): XxHash32;
    fn oneshot(seed: u32, data: []u8): u32;
    fn write(_: auto, data: []u8);
    fn writeStr(_: auto, s: string);
    fn finish(): u32;
}

export class XxHash64 {
    seed: u64,
    v1: u64,
    v2: u64,
    v3: u64,
    v4: u64,
    buf: [32]u8,
    buf_len: usize,
    total_len: u64,
    fn withSeed(seed: u64): XxHash64;
    fn oneshot(seed: u64, data: []u8): u64;
    fn write(_: auto, data: []u8);
    fn writeStr(_: auto, s: string);
    fn finish(): u64;
}

