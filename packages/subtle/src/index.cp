// @cap/subtle-ct — Zero-allocation branchless constant-time primitives & zeroize
// Inspired by RustCrypto `subtle` & `zeroize`
// MIT License

const MASK32: u64 = 0xFFFFFFFF;
const BIT32: u64 = (1 as u64) << 32;

pub struct Choice {
    pub val: u8,

    pub static fn fromU8(v: u8): Choice {
        return Choice { val: v & 1 };
    }

    pub fn toBool(): bool {
        return _.val == 1;
    }

    pub fn not(): Choice {
        return Choice { val: (_.val & 1) ^ 1 };
    }

    pub fn and(other: Choice): Choice {
        return Choice { val: (_.val & other.val) & 1 };
    }

    pub fn or(other: Choice): Choice {
        return Choice { val: (_.val | other.val) & 1 };
    }

    pub fn xor(other: Choice): Choice {
        return Choice { val: (_.val ^ other.val) & 1 };
    }
}

fn mask8(c: Choice): u8 {
    let b: u8 = c.val & 1;
    let m: u8 = b | (b << 1);
    m = m | (m << 2);
    m = m | (m << 4);
    return m;
}

fn mask32(c: Choice): u32 {
    let m8: u32 = mask8(c) as u32;
    let m16: u32 = m8 | (m8 << 8);
    return m16 | (m16 << 16);
}

fn mask64(c: Choice): u64 {
    let m32: u64 = mask32(c) as u64;
    return m32 | (m32 << 32);
}

fn ctIsZeroU8(x: u8): Choice {
    let v: u8 = x;
    v = v | (v >> 4);
    v = v | (v >> 2);
    v = v | (v >> 1);
    return Choice { val: (v & 1) ^ 1 };
}

fn ctIsZeroU32(x: u32): Choice {
    let v: u32 = x;
    v = v | (v >> 16);
    v = v | (v >> 8);
    v = v | (v >> 4);
    v = v | (v >> 2);
    v = v | (v >> 1);
    return Choice { val: ((v & 1) as u8) ^ 1 };
}

pub fn ctIsZeroU64(x: u64): Choice {
    let v: u64 = x;
    v = v | (v >> 32);
    v = v | (v >> 16);
    v = v | (v >> 8);
    v = v | (v >> 4);
    v = v | (v >> 2);
    v = v | (v >> 1);
    return Choice { val: ((v & 1) as u8) ^ 1 };
}

pub fn ctEqU8(a: u8, b: u8): Choice {
    return ctIsZeroU8(a ^ b);
}

pub fn ctEqU32(a: u32, b: u32): Choice {
    return ctIsZeroU32(a ^ b);
}

pub fn ctEqU64(a: u64, b: u64): Choice {
    return ctIsZeroU64(a ^ b);
}

pub fn ctEqBytes(a: []const u8, b: []const u8): Choice {
    if (a.len != b.len) {
        return Choice { val: 0 };
    }
    let acc: u8 = 0;
    let i: usize = 0;
    while (i < a.len) {
        acc = acc | (a[i] ^ b[i]);
        i = i + 1;
    }
    return ctIsZeroU8(acc);
}

pub fn ctLtU64(a: u64, b: u64): Choice {
    // Overflow-free, branchless 32-bit half subtraction borrow chain
    let lo_a: u64 = a & MASK32;
    let lo_b: u64 = b & MASK32;
    let diff_lo: u64 = (lo_a + BIT32) - lo_b;
    let borrow_lo: u64 = ((diff_lo >> 32) & 1) ^ 1;

    let hi_a: u64 = a >> 32;
    let hi_b: u64 = b >> 32;
    let diff_hi: u64 = (hi_a + BIT32) - (hi_b + borrow_lo);
    let borrow_hi: u64 = ((diff_hi >> 32) & 1) ^ 1;
    return Choice { val: borrow_hi as u8 };
}

pub fn ctGtU64(a: u64, b: u64): Choice {
    return ctLtU64(b, a);
}

pub fn ctSelectU8(a: u8, b: u8, c: Choice): u8 {
    let m: u8 = mask8(c);
    return a ^ (m & (a ^ b));
}

pub fn ctSelectU32(a: u32, b: u32, c: Choice): u32 {
    let m: u32 = mask32(c);
    return a ^ (m & (a ^ b));
}

pub fn ctSelectU64(a: u64, b: u64, c: Choice): u64 {
    let m: u64 = mask64(c);
    return a ^ (m & (a ^ b));
}

pub fn ctSelectBytes(dst: []u8, a: []const u8, b: []const u8, c: Choice): usize {
    let count: usize = dst.len;
    if (a.len < count) {
        count = a.len;
    }
    if (b.len < count) {
        count = b.len;
    }
    let m: u8 = mask8(c);
    let i: usize = 0;
    while (i < count) {
        dst[i] = a[i] ^ (m & (a[i] ^ b[i]));
        i = i + 1;
    }
    return count;
}

pub fn ctCondSwapU64(a: *u64, b: *u64, c: Choice) {
    let m: u64 = mask64(c);
    let diff: u64 = m & (a.* ^ b.*);
    a.* = a.* ^ diff;
    b.* = b.* ^ diff;
}

pub fn zeroizeBytes(buf: []u8) {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
        i = i + 1;
    }
}
