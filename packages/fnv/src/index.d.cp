// Type definitions for CP

import { readU16LE, readU32LE, readU64LE } from "cap:bin";
export fn mulWrap32(a: u32, b: u32): u32;
export fn mulWrap64(a: u64, b: u64): u64;
export fn rotl64By5(x: u64): u64;
export class Fnv32Hasher {
    state: u32,
    fn init(): Fnv32Hasher;
    fn writeByte(_: auto, b: u8);
    fn write(_: auto, data: string);
    fn finish(): u32;
}

export fn fnv1a32(data: string): u32;
export class Fnv64Hasher {
    state: u64,
    fn init(): Fnv64Hasher;
    fn writeByte(_: auto, b: u8);
    fn write(_: auto, data: string);
    fn finish(): u64;
}

export fn fnv1a64(data: string): u64;
export class FxHasher {
    state: u64,
    fn init(): FxHasher;
    fn withSeed(seed: u64): FxHasher;
    fn writeU64(_: auto, v: u64);
    fn writeBytes(_: auto, data: string);
    fn finish(): u64;
}

export fn fxHash64(data: string): u64;
