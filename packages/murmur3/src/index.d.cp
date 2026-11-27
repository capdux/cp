// Type definitions for CP

import { readU32LE, readU64LE } from "cap:bin";
export fn rotl32(x: u32, r: u5): u32;
export fn rotl64(x: u64, r: u6): u64;
export fn fmix32(h_in: u32): u32;
export fn fmix64(k_in: u64): u64;
export fn murmur3_32(data: string, seed: u32): u32;
export class Hash128 {
    lo: u64,
    hi: u64,
}

export fn murmur3_128(data: string, seed: u64): Hash128;
