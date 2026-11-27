// Type definitions for CP

import { isAlnum, toLowerByte } from "cap:ascii";
export class Hamming74Decoded {
    data: u8,
    corrected: bool,
    error_bit: u8,
}

export fn popcountU32(x: u32): u32;
export fn popcountU8(b: u8): u32;
export fn popcountU64(x: u64): u32;
export fn hammingU64(a: u64, b: u64): u32;
export fn weightBytes(data: &[]u8): u64;
export fn distanceBytes(a: &[]u8, b: &[]u8): i64;
export fn isTokenByte(b: u8): bool;
export fn hashToken64(text: &[]u8, start: usize, end: usize): u64;
export fn simhash64(text: &[]u8): u64;
export fn hamming74Encode(nibble: u8): u8;
export fn hamming74Decode(codeword: u8): Hamming74Decoded;
