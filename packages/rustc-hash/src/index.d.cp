// Type definitions for CP

import { readU16LE, readU32LE, readU64LE } from "cap:bin";
export fn fxhMul64(a: u64, b: u64): u64;
export fn fxhMul32(a: u32, b: u32): u32;
export fn fxhRotl64By5(x: u64): u64;
export fn fxhRotl32By5(x: u32): u32;
export fn fxhAddWord64(hash: u64, word: u64): u64;
export fn fxhAddWord32(hash: u32, word: u32): u32;
export fn fxhHashSlice64(hash: u64, data: &[]u8): u64;
export fn fxhHashSlice32(hash: u32, data: &[]u8): u32;
export class FxHasher64 {
    hash: u64,
    fn new(): FxHasher64;
    fn withSeed(seed: u64): FxHasher64;
    fn writeUsize(_: auto, v: usize);
    fn writeU64(_: auto, v: u64);
    fn writeU32(_: auto, v: u32);
    fn writeBytes(_: auto, data: []u8);
    fn writeStr(_: auto, s: string);
    fn finish(): u64;
    fn reset(_: auto);
}

export class FxHasher32 {
    hash: u32,
    fn new(): FxHasher32;
    fn withSeed(seed: u32): FxHasher32;
    fn writeU32(_: auto, v: u32);
    fn writeBytes(_: auto, data: []u8);
    fn writeStr(_: auto, s: string);
    fn finish(): u32;
    fn reset(_: auto);
}

export fn fxHash64(data: []u8): u64;
export fn fxHash64Str(s: string): u64;
export fn fxHash32(data: []u8): u32;
