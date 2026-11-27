// @cap/hamming — Zero-allocation Hamming Distance, Popcount, 64-bit SimHash & Hamming(7,4) ECC
// Clean-room MIT implementation in pure CP.

import { isAlnum, toLowerByte } from "cap:ascii";

pub struct Hamming74Decoded {
    pub data: u8,
    pub corrected: bool,
    pub error_bit: u8,
}

fn popcountU32(x: u32) -> u32 {
    let v = x - ((x >> 1) & 0x55555555);
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    return (((v + (v >> 4)) & 0x0F0F0F0F) *% 0x01010101) >> 24;
}

fn popcountU8(b: u8) -> u32 {
    let v = b as u32;
    v = v - ((v >> 1) & 0x55);
    v = (v & 0x33) + ((v >> 2) & 0x33);
    return (v + (v >> 4)) & 0x0F;
}

/// Counts the number of 1-bits in a 64-bit unsigned integer.
pub fn popcountU64(x: u64) -> u32 {
    const lo = (x & 0xFFFFFFFF) as u32;
    const hi = ((x >> 32) & 0xFFFFFFFF) as u32;
    return popcountU32(lo) + popcountU32(hi);
}

/// Computes the bitwise Hamming distance between two 64-bit unsigned integers.
pub fn hammingU64(a: u64, b: u64) -> u32 {
    return popcountU64(a ^ b);
}

/// Counts total 1-bits across all bytes in `data`.
pub fn weightBytes(data: []const u8) -> u64 {
    let total: u64 = 0;
    let i: usize = 0;
    while (i < data.len) {
        total += popcountU8(data[i]) as u64;
        i += 1;
    }
    return total;
}

/// Computes bitwise Hamming distance between two equal-length byte slices.
/// Returns -1 if `a.len != b.len`.
pub fn distanceBytes(a: []const u8, b: []const u8) -> i64 {
    if (a.len != b.len) {
        return -1;
    }
    let total: i64 = 0;
    let i: usize = 0;
    while (i < a.len) {
        total += popcountU8(a[i] ^ b[i]) as i64;
        i += 1;
    }
    return total;
}

fn isTokenByte(b: u8) -> bool {
    return isAlnum(b) || b >= 128;
}

fn hashToken64(text: []const u8, start: usize, end: usize) -> u64 {
    const fnv_offset: u64 = ((0xCBF29CE4 as u64) << 32) | (0x84222325 as u64);
    const fnv_prime: u64 = ((0x00000100 as u64) << 32) | (0x000001B3 as u64);
    const mix1: u64 = ((0xBF58476D as u64) << 32) | (0x1CE4E5B9 as u64);
    const mix2: u64 = ((0x94D049BB as u64) << 32) | (0x133111EB as u64);

    let h: u64 = fnv_offset;
    let i = start;
    while (i < end) {
        h ^= toLowerByte(text[i]) as u64;
        h = h *% fnv_prime;
        i += 1;
    }

    // SplitMix64 avalanche finalizer
    h ^= h >> 30;
    h = h *% mix1;
    h ^= h >> 27;
    h = h *% mix2;
    h ^= h >> 31;
    return h;
}

/// Computes a 64-bit Charikar SimHash fingerprint for near-duplicate text detection.
/// Tokenizes on ASCII whitespace and punctuation (case-insensitive for ASCII letters).
pub fn simhash64(text: []const u8) -> u64 {
    let weights: [64]i64 = undefined;
    let b: usize = 0;
    while (b < 64) {
        weights[b] = 0;
        b += 1;
    }

    let i: usize = 0;
    const n = text.len;
    while (i < n) {
        while (i < n && !isTokenByte(text[i])) {
            i += 1;
        }
        if (i >= n) {
            break;
        }
        const start = i;
        while (i < n && isTokenByte(text[i])) {
            i += 1;
        }
        const h = hashToken64(text, start, i);
        let bit: usize = 0;
        while (bit < 64) {
            if (((h >> (bit as u6)) & 1) != 0) {
                weights[bit] += 1;
            } else {
                weights[bit] -= 1;
            }
            bit += 1;
        }
    }

    let fp: u64 = 0;
    b = 0;
    while (b < 64) {
        if (weights[b] > 0) {
            fp |= (1 as u64) << (b as u6);
        }
        b += 1;
    }
    return fp;
}

/// Encodes a 4-bit nibble (low 4 bits of `nibble`) into a 7-bit Hamming(7,4) codeword.
/// Bit layout (1-indexed positions 1..7 stored in bits 0..6):
///   bit 0 (pos 1): p1 = d1 ^ d2 ^ d4
///   bit 1 (pos 2): p2 = d1 ^ d3 ^ d4
///   bit 2 (pos 3): d1
///   bit 3 (pos 4): p4 = d2 ^ d3 ^ d4
///   bit 4 (pos 5): d2
///   bit 5 (pos 6): d3
///   bit 6 (pos 7): d4
pub fn hamming74Encode(nibble: u8) -> u8 {
    const d1: u8 = (nibble >> 0) & 1;
    const d2: u8 = (nibble >> 1) & 1;
    const d3: u8 = (nibble >> 2) & 1;
    const d4: u8 = (nibble >> 3) & 1;

    const p1: u8 = d1 ^ d2 ^ d4;
    const p2: u8 = d1 ^ d3 ^ d4;
    const p4: u8 = d2 ^ d3 ^ d4;

    return p1
        | (p2 << 1)
        | (d1 << 2)
        | (p4 << 3)
        | (d2 << 4)
        | (d3 << 5)
        | (d4 << 6);
}

/// Decodes a 7-bit Hamming(7,4) codeword, automatically correcting any single-bit flip.
/// `error_bit` is 0 when no error was detected, or 1..7 indicating the 1-indexed flipped bit.
pub fn hamming74Decode(codeword: u8) -> Hamming74Decoded {
    const cw: u8 = codeword & 0x7F;
    const b1: u8 = (cw >> 0) & 1;
    const b2: u8 = (cw >> 1) & 1;
    const b3: u8 = (cw >> 2) & 1;
    const b4: u8 = (cw >> 3) & 1;
    const b5: u8 = (cw >> 4) & 1;
    const b6: u8 = (cw >> 5) & 1;
    const b7: u8 = (cw >> 6) & 1;

    const s1: u8 = b1 ^ b3 ^ b5 ^ b7;
    const s2: u8 = b2 ^ b3 ^ b6 ^ b7;
    const s4: u8 = b4 ^ b5 ^ b6 ^ b7;
    const syndrome: u8 = s1 | (s2 << 1) | (s4 << 2);

    let fixed = cw;
    let corrected = false;
    if (syndrome != 0) {
        const shift: u3 = (syndrome - 1) as u3;
        fixed = cw ^ ((1 as u8) << shift);
        corrected = true;
    }

    const d1: u8 = (fixed >> 2) & 1;
    const d2: u8 = (fixed >> 4) & 1;
    const d3: u8 = (fixed >> 5) & 1;
    const d4: u8 = (fixed >> 6) & 1;
    const data: u8 = d1 | (d2 << 1) | (d3 << 2) | (d4 << 3);

    return Hamming74Decoded {
        data: data,
        corrected: corrected,
        error_bit: syndrome,
    };
}
