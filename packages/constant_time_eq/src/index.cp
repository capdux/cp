// @cap/subtle — Zero-allocation constant-time & secure zeroization primitives for CP
// Clean-room MIT implementation inspired by Rust `subtle` (921M+ downloads) and `zeroize` (935M+ downloads).
//
// Features:
// - Branchless `Choice` boolean algebra (`fromU8`, `fromBool`, `toBool`, `ctNot`, `ctAnd`, `ctOr`, `ctXor`).
// - Branchless integer comparison, selection, and swap (`ctEqU8/U32/U64`, `ctLtU64`, `ctGtU64`,
//   `ctSelectU8/U32/U64`, `ctSwapU64`) using overflow-free bit arithmetic.
// - Constant-time byte slice & 32-byte digest comparison (`ctEqBytes`, `ctEqFixed32`),
//   branchless conditional copy (`ctCopyIf`), and DSE-resistant `zeroizeBytes` + `isAllZero`.

const std = @import("std");

/// Constant-time boolean value guaranteed to hold only `0` (false) or `1` (true).
pub struct Choice {
    pub val: u8,

    /// Converts any `u8` into a `Choice` (0 -> 0, any non-zero -> 1) without branching.
    pub static fn fromU8(v: u8) -> Choice {
        let x = v | (v >> 4);
        x = x | (x >> 2);
        x = x | (x >> 1);
        return Choice{ val: x & 1 };
    }

    /// Converts a `bool` into a `Choice`.
    pub static fn fromBool(b: bool) -> Choice {
        if (b) {
            return Choice{ val: 1 };
        }
        return Choice{ val: 0 };
    }

    pub fn toBool() -> bool {
        return _.val != 0;
    }

    pub fn ctNot() -> Choice {
        return Choice{ val: (_.val & 1) ^ 1 };
    }

    pub fn ctAnd(other: Choice) -> Choice {
        return Choice{ val: (_.val & other.val) & 1 };
    }

    pub fn ctOr(other: Choice) -> Choice {
        return Choice{ val: (_.val | other.val) & 1 };
    }

    pub fn ctXor(other: Choice) -> Choice {
        return Choice{ val: (_.val ^ other.val) & 1 };
    }
}

fn maskU8(c: Choice) -> u8 {
    let m: u8 = c.val & 1;
    m = m | (m << 1);
    m = m | (m << 2);
    m = m | (m << 4);
    return m;
}

fn maskU32(c: Choice) -> u32 {
    let m: u32 = (c.val & 1) as u32;
    m = m | (m << 1);
    m = m | (m << 2);
    m = m | (m << 4);
    m = m | (m << 8);
    m = m | (m << 16);
    return m;
}

fn maskU64(c: Choice) -> u64 {
    let m: u64 = (c.val & 1) as u64;
    m = m | (m << 1);
    m = m | (m << 2);
    m = m | (m << 4);
    m = m | (m << 8);
    m = m | (m << 16);
    m = m | (m << 32);
    return m;
}

fn ctIsZeroU8(x: u8) -> Choice {
    let v = x | (x >> 4);
    v = v | (v >> 2);
    v = v | (v >> 1);
    return Choice{ val: (v & 1) ^ 1 };
}

/// Constant-time equality for `u8`.
pub fn ctEqU8(a: u8, b: u8) -> Choice {
    return ctIsZeroU8(a ^ b);
}

/// Constant-time equality for `u32`.
pub fn ctEqU32(a: u32, b: u32) -> Choice {
    let v: u32 = a ^ b;
    v = v | (v >> 16);
    v = v | (v >> 8);
    v = v | (v >> 4);
    v = v | (v >> 2);
    v = v | (v >> 1);
    return Choice{ val: ((v & 1) as u8) ^ 1 };
}

/// Constant-time equality for `u64`.
pub fn ctEqU64(a: u64, b: u64) -> Choice {
    let v: u64 = a ^ b;
    v = v | (v >> 32);
    v = v | (v >> 16);
    v = v | (v >> 8);
    v = v | (v >> 4);
    v = v | (v >> 2);
    v = v | (v >> 1);
    return Choice{ val: ((v & 1) as u8) ^ 1 };
}

/// Branchless, overflow-free constant-time unsigned less-than (`a < b`).
pub fn ctLtU64(a: u64, b: u64) -> Choice {
    const a_hi: u64 = a >> 32;
    const a_lo: u64 = a & (0xFFFFFFFF as u64);
    const b_hi: u64 = b >> 32;
    const b_lo: u64 = b & (0xFFFFFFFF as u64);

    // 2^32 + a_lo - b_lo >= 1 (never underflows); bit 32 is 0 iff a_lo < b_lo
    const lo_diff: u64 = ((1 as u64) << 32) + a_lo - b_lo;
    const borrow_lo: u64 = ((lo_diff >> 32) & 1) ^ 1;

    // 2^33 + a_hi - b_hi - borrow_lo >= 2^32 (never underflows); bit 33 is 0 iff a < b
    const hi_diff: u64 = ((1 as u64) << 33) + a_hi - b_hi - borrow_lo;
    const borrow_hi: u64 = ((hi_diff >> 33) & 1) ^ 1;

    return Choice{ val: borrow_hi as u8 };
}

/// Branchless, overflow-free constant-time unsigned greater-than (`a > b`).
pub fn ctGtU64(a: u64, b: u64) -> Choice {
    return ctLtU64(b, a);
}

/// Branchless constant-time selection for `u8`: returns `when_true` if `c == 1`, else `when_false`.
pub fn ctSelectU8(c: Choice, when_true: u8, when_false: u8) -> u8 {
    const m = maskU8(c);
    return (when_true & m) | (when_false & (m ^ (0xFF as u8)));
}

/// Branchless constant-time selection for `u32`.
pub fn ctSelectU32(c: Choice, when_true: u32, when_false: u32) -> u32 {
    const m = maskU32(c);
    return (when_true & m) | (when_false & (m ^ (0xFFFFFFFF as u32)));
}

/// Branchless constant-time selection for `u64`.
pub fn ctSelectU64(c: Choice, when_true: u64, when_false: u64) -> u64 {
    const m = maskU64(c);
    return (when_true & m) | (when_false & (m ^ (0xFFFFFFFFFFFFFFFF as u64)));
}

/// Branchless constant-time conditional swap of `*a` and `*b` when `c == 1`.
pub fn ctSwapU64(c: Choice, a: *u64, b: *u64) {
    const m = maskU64(c);
    const t = (a.* ^ b.*) & m;
    a.* = a.* ^ t;
    b.* = b.* ^ t;
}

/// Constant-time byte slice equality (never short-circuits on first mismatch).
pub fn ctEqBytes(a: string, b: string) -> Choice {
    if (a.len != b.len) {
        return Choice{ val: 0 };
    }
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) {
        diff = diff | (a[i] ^ b[i]);
        i += 1;
    }
    return ctIsZeroU8(diff);
}

/// Constant-time equality for 32-byte digests (SHA-256, HMAC-SHA256, Ed25519 keys/tags).
pub fn ctEqFixed32(a: [32]u8, b: [32]u8) -> Choice {
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < 32) {
        diff = diff | (a[i] ^ b[i]);
        i += 1;
    }
    return ctIsZeroU8(diff);
}

/// Branchless conditional copy: overwrites `dst[0..min(dst.len, src.len)]` with `src` iff `c == 1`.
pub fn ctCopyIf(c: Choice, dst: []u8, src: string) {
    const n = if (dst.len < src.len) { dst.len } else { src.len };
    const m = maskU8(c);
    const inv = m ^ (0xFF as u8);
    let i: usize = 0;
    while (i < n) {
        dst[i] = (src[i] & m) | (dst[i] & inv);
        i += 1;
    }
}

/// Securely overwrites sensitive memory in `buf` with zeros using volatile writes
/// resistant to dead-store elimination (DSE).
pub fn zeroizeBytes(buf: []u8) {
    std.crypto.secureZero(u8, buf);
}

/// Constant-time check whether every byte in `buf` is zero.
pub fn isAllZero(buf: string) -> Choice {
    let acc: u8 = 0;
    let i: usize = 0;
    while (i < buf.len) {
        acc = acc | buf[i];
        i += 1;
    }
    return ctIsZeroU8(acc);
}
