// Origin: Pure-CP Stdlib (Zero-Alloc 64-bit Charikar SimHash, Hamming Similarity & 4x16-bit LSH Bands) | Created: 2026-11-22

import std from "std";

/// Fingerprint bit width (64 bits).
pub const SIMHASH_BITS: u8 = 64;

/// Standard near-duplicate Hamming distance threshold (<= 3 bits out of 64).
pub const SIMHASH_DEFAULT_THRESHOLD: u8 = 3;

const FNV1A_64_OFFSET: u64 = 0xCBF29CE484222325;
const FNV1A_64_PRIME: u64 = 0x00000100000001B3;

fn mulWrap64(a: u64, b: u64): u64 {
    return (((a as u128) * (b as u128)) & 0xFFFFFFFFFFFFFFFF) as u64;
}

fn toLowerAscii(ch: u8): u8 {
    if (ch >= 0x41 && ch <= 0x5A) {
        return ch + 0x20;
    }
    return ch;
}

fn isAsciiAlnum(ch: u8): bool {
    if (ch >= 0x30 && ch <= 0x39) { return true; }
    if (ch >= 0x41 && ch <= 0x5A) { return true; }
    if (ch >= 0x61 && ch <= 0x7A) { return true; }
    return false;
}

/// Computes 64-bit FNV-1a feature hash over `data`.
pub fn fnv1a64(data: []const u8): u64 {
    let h: u64 = FNV1A_64_OFFSET;
    let i: usize = 0;
    while (i < data.len) {
        h = h ^ (data[i] as u64);
        h = mulWrap64(h, FNV1A_64_PRIME);
        i += 1;
    }
    return h;
}

fn fnv1a64Lower(data: []const u8): u64 {
    let h: u64 = FNV1A_64_OFFSET;
    let i: usize = 0;
    while (i < data.len) {
        h = h ^ (toLowerAscii(data[i]) as u64);
        h = mulWrap64(h, FNV1A_64_PRIME);
        i += 1;
    }
    return h;
}

/// Zero-allocation 64-bit Moses Charikar SimHash feature accumulator.
pub struct SimHasher {
    weights: [64]i64;
    feature_count: u64;

    /// Creates a zero-initialized `SimHasher`.
    pub fn init(): SimHasher {
        return SimHasher {
            weights: std.mem.zeroes([64]i64),
            feature_count: 0,
        };
    }

    /// Resets all 64 bit weights and `feature_count` to 0.
    fn reset(*_) {
        let i: usize = 0;
        while (i < 64) {
            _.weights[i] = 0;
            i += 1;
        }
        _.feature_count = 0;
    }

    /// Accumulates a 64-bit feature hash with signed `weight`:
    /// adds `weight` for 1-bits and subtracts `weight` for 0-bits.
    fn addHash(*_, hash: u64, weight: i64) {
        let i: usize = 0;
        while (i < 64) {
            if (((hash >> (i as u6)) & 1) != 0) {
                _.weights[i] += weight;
            } else {
                _.weights[i] -= weight;
            }
            i += 1;
        }
        _.feature_count += 1;
    }

    /// Hashes `token` via 64-bit FNV-1a and accumulates it with `weight`.
    fn addToken(*_, token: []const u8, weight: i64) {
        _.addHash(fnv1a64(token), weight);
    }

    /// Zero-copy ASCII alphanumeric tokenizer with case-folding (`A-Z` -> `a-z`).
    /// Each word is hashed via FNV-1a and added with weight `1`.
    fn addTextWords(*_, text: []const u8) {
        let i: usize = 0;
        while (i < text.len) {
            while (i < text.len && !isAsciiAlnum(text[i])) {
                i += 1;
            }
            const start = i;
            while (i < text.len && isAsciiAlnum(text[i])) {
                i += 1;
            }
            if (i > start) {
                _.addHash(fnv1a64Lower(text[start..i]), 1);
            }
        }
    }

    /// Zero-copy sliding `k`-byte shingle extractor (ideal for CJK/UTF-8 or whitespace-free streams).
    /// No-op when `k == 0` or `text.len == 0`. If `0 < text.len < k`, hashes the entire slice once.
    fn addCharShingles(*_, text: []const u8, k: usize) {
        if (k == 0 || text.len == 0) {
            return;
        }
        if (text.len < k) {
            _.addHash(fnv1a64(text), 1);
            return;
        }
        let i: usize = 0;
        const limit = text.len - k;
        while (i <= limit) {
            _.addHash(fnv1a64(text[i..i + k]), 1);
            i += 1;
        }
    }

    /// Finalizes the 64-bit SimHash fingerprint (bit `i` is `1` iff `weights[i] > 0`).
    fn finish(): u64 {
        let fp: u64 = 0;
        let i: usize = 0;
        while (i < 64) {
            if (_.weights[i] > 0) {
                fp = fp | ((1 as u64) << (i as u6));
            }
            i += 1;
        }
        return fp;
    }
}

/// Computes the 64-bit word-tokenized SimHash fingerprint of `text`.
pub fn simhashText(text: []const u8): u64 {
    let h = SimHasher.init();
    h.addTextWords(text);
    return h.finish();
}

/// Computes the 64-bit `k`-shingle SimHash fingerprint of `text`.
pub fn simhashShingles(text: []const u8, k: usize): u64 {
    let h = SimHasher.init();
    h.addCharShingles(text, k);
    return h.finish();
}

/// Computes Hamming distance (`0..64`) between two 64-bit fingerprints.
pub fn hammingDistance(a: u64, b: u64): u8 {
    let x = a ^ b;
    let c: u8 = 0;
    while (x != 0) {
        x = x & (x - 1);
        c += 1;
    }
    return c;
}

/// Computes normalized SimHash similarity in `[0.0, 1.0]` (`1.0 - dist / 64.0`).
pub fn simhashSimilarity(a: u64, b: u64): f64 {
    const dist = hammingDistance(a, b) as f64;
    return 1.0 - (dist / 64.0);
}

/// Returns `true` iff `hammingDistance(a, b) <= max_dist`.
pub fn isNearDuplicate(a: u64, b: u64, max_dist: u8): bool {
    return hammingDistance(a, b) <= max_dist;
}

/// Splits a 64-bit SimHash fingerprint into four 16-bit LSH index bands.
/// By the pigeonhole principle, any two fingerprints with Hamming distance <= 3
/// are guaranteed to share at least one identical 16-bit band.
pub fn simhashBands4(fp: u64): [4]u16 {
    return [
        (fp & 0xFFFF) as u16,
        ((fp >> 16) & 0xFFFF) as u16,
        ((fp >> 32) & 0xFFFF) as u16,
        ((fp >> 48) & 0xFFFF) as u16,
    ];
}

/// Returns `true` iff `a` and `b` match in at least one of their four 16-bit LSH bands.
pub fn sharesAnyBand4(a: u64, b: u64): bool {
    const ba = simhashBands4(a);
    const bb = simhashBands4(b);
    return ba[0] == bb[0] || ba[1] == bb[1] || ba[2] == bb[2] || ba[3] == bb[3];
}
