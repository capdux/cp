// @cap/bigint-u256 — Zero-allocation 256-bit unsigned integer arithmetic (4x u64 little-endian limbs)
// MIT License

const MASK32: u64 = 0xFFFFFFFF;
const BIT32: u64 = (1 as u64) << 32;
const MAX_U64: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);

struct Add64Res {
    sum: u64,
    carry: u64,
}

struct Sub64Res {
    diff: u64,
    borrow: u64,
}

struct Mul64Res {
    lo: u64,
    hi: u64,
}

fn addWithCarry64(a: u64, b: u64, carry_in: u64): Add64Res {
    let lo: u64 = (a & MASK32) + (b & MASK32) + carry_in;
    let hi: u64 = (a >> 32) + (b >> 32) + (lo >> 32);
    return Add64Res {
        sum: (lo & MASK32) | ((hi & MASK32) << 32),
        carry: hi >> 32,
    };
}

fn subWithBorrow64(a: u64, b: u64, borrow_in: u64): Sub64Res {
    let lo_a: u64 = a & MASK32;
    let lo_b: u64 = (b & MASK32) + borrow_in;
    let lo_d: u64 = 0;
    let b0: u64 = 0;
    if (lo_a < lo_b) {
        lo_d = (lo_a + BIT32) - lo_b;
        b0 = 1;
    } else {
        lo_d = lo_a - lo_b;
    }

    let hi_a: u64 = a >> 32;
    let hi_b: u64 = (b >> 32) + b0;
    let hi_d: u64 = 0;
    let b1: u64 = 0;
    if (hi_a < hi_b) {
        hi_d = (hi_a + BIT32) - hi_b;
        b1 = 1;
    } else {
        hi_d = hi_a - hi_b;
    }

    return Sub64Res {
        diff: (lo_d & MASK32) | ((hi_d & MASK32) << 32),
        borrow: b1,
    };
}

fn mulWide64(a: u64, b: u64): Mul64Res {
    let a_lo: u64 = a & MASK32;
    let a_hi: u64 = a >> 32;
    let b_lo: u64 = b & MASK32;
    let b_hi: u64 = b >> 32;

    let p0: u64 = a_lo * b_lo;
    let p1: u64 = a_lo * b_hi;
    let p2: u64 = a_hi * b_lo;
    let p3: u64 = a_hi * b_hi;

    let mid: u64 = (p0 >> 32) + (p1 & MASK32) + (p2 & MASK32);
    let lo: u64 = (p0 & MASK32) | ((mid & MASK32) << 32);
    let hi: u64 = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
    return Mul64Res { lo: lo, hi: hi };
}

fn clz64(x: u64): u32 {
    if (x == 0) {
        return 64;
    }
    let v: u64 = x;
    let n: u32 = 0;
    if ((v >> 32) == 0) {
        n = n + 32;
        v = v << 32;
    }
    if ((v >> 48) == 0) {
        n = n + 16;
        v = v << 16;
    }
    if ((v >> 56) == 0) {
        n = n + 8;
        v = v << 8;
    }
    if ((v >> 60) == 0) {
        n = n + 4;
        v = v << 4;
    }
    if ((v >> 62) == 0) {
        n = n + 2;
        v = v << 2;
    }
    if ((v >> 63) == 0) {
        n = n + 1;
    }
    return n;
}

fn hexVal(c: u8): i32 {
    if (c >= 48 && c <= 57) {
        return (c - 48) as i32;
    }
    if (c >= 97 && c <= 102) {
        return (c - 97 + 10) as i32;
    }
    if (c >= 65 && c <= 70) {
        return (c - 65 + 10) as i32;
    }
    return -1;
}

fn hexDigit(v: u8): u8 {
    if (v < 10) {
        return v + 48;
    }
    return (v - 10) + 97;
}

pub struct DivRem256 {
    pub quot: U256,
    pub rem: U256,
}

pub struct U256 {
    pub limbs: [4]u64,

    pub static fn zero(): U256 {
        return U256 { limbs: [0, 0, 0, 0] };
    }

    pub static fn one(): U256 {
        return U256 { limbs: [1, 0, 0, 0] };
    }

    pub static fn fromU64(v: u64): U256 {
        return U256 { limbs: [v, 0, 0, 0] };
    }

    pub static fn fromU128(lo: u64, hi: u64): U256 {
        return U256 { limbs: [lo, hi, 0, 0] };
    }

    pub static fn maxVal(): U256 {
        return U256 { limbs: [MAX_U64, MAX_U64, MAX_U64, MAX_U64] };
    }

    pub static fn fromHex(s: string): U256 {
        let start: usize = 0;
        if (s.len >= 2 && s[0] == 48 && (s[1] == 120 || s[1] == 88)) {
            start = 2;
        }
        let acc: U256 = U256.zero();
        let i: usize = start;
        while (i < s.len) {
            let v: i32 = hexVal(s[i]);
            if (v >= 0) {
                acc = acc.shl(4);
                acc.limbs[0] = acc.limbs[0] | (v as u64);
            }
            i = i + 1;
        }
        return acc;
    }

    pub fn isZero(): bool {
        return _.limbs[0] == 0 && _.limbs[1] == 0 && _.limbs[2] == 0 && _.limbs[3] == 0;
    }

    pub fn cmp(other: U256): i32 {
        let idx: usize = 4;
        while (idx > 0) {
            idx = idx - 1;
            if (_.limbs[idx] < other.limbs[idx]) {
                return -1;
            }
            if (_.limbs[idx] > other.limbs[idx]) {
                return 1;
            }
        }
        return 0;
    }

    pub fn eq(other: U256): bool {
        return _.limbs[0] == other.limbs[0] &&
            _.limbs[1] == other.limbs[1] &&
            _.limbs[2] == other.limbs[2] &&
            _.limbs[3] == other.limbs[3];
    }

    pub fn wrappingAdd(other: U256): U256 {
        let r0: Add64Res = addWithCarry64(_.limbs[0], other.limbs[0], 0);
        let r1: Add64Res = addWithCarry64(_.limbs[1], other.limbs[1], r0.carry);
        let r2: Add64Res = addWithCarry64(_.limbs[2], other.limbs[2], r1.carry);
        let r3: Add64Res = addWithCarry64(_.limbs[3], other.limbs[3], r2.carry);
        return U256 { limbs: [r0.sum, r1.sum, r2.sum, r3.sum] };
    }

    pub fn wrappingSub(other: U256): U256 {
        let r0: Sub64Res = subWithBorrow64(_.limbs[0], other.limbs[0], 0);
        let r1: Sub64Res = subWithBorrow64(_.limbs[1], other.limbs[1], r0.borrow);
        let r2: Sub64Res = subWithBorrow64(_.limbs[2], other.limbs[2], r1.borrow);
        let r3: Sub64Res = subWithBorrow64(_.limbs[3], other.limbs[3], r2.borrow);
        return U256 { limbs: [r0.diff, r1.diff, r2.diff, r3.diff] };
    }

    pub fn wrappingMul(other: U256): U256 {
        let out: [4]u64 = [0, 0, 0, 0];
        let i: usize = 0;
        while (i < 4) {
            let carry: u64 = 0;
            let j: usize = 0;
            while (j < 4) {
                let k: usize = i + j;
                if (k >= 4) {
                    break;
                }
                let prod: Mul64Res = mulWide64(_.limbs[i], other.limbs[j]);
                let s1: Add64Res = addWithCarry64(out[k], prod.lo, 0);
                let s2: Add64Res = addWithCarry64(s1.sum, carry, 0);
                out[k] = s2.sum;
                let c_next: Add64Res = addWithCarry64(prod.hi, s1.carry + s2.carry, 0);
                carry = c_next.sum;
                j = j + 1;
            }
            i = i + 1;
        }
        return U256 { limbs: out };
    }

    pub fn shl(bits: u32): U256 {
        if (bits >= 256) {
            return U256.zero();
        }
        if (bits == 0) {
            return U256 { limbs: [_.limbs[0], _.limbs[1], _.limbs[2], _.limbs[3]] };
        }
        let word_shift: usize = (bits >> 6) as usize;
        let bit_shift: u32 = bits & 63;
        let out: [4]u64 = [0, 0, 0, 0];
        if (bit_shift == 0) {
            let i: usize = word_shift;
            while (i < 4) {
                out[i] = _.limbs[i - word_shift];
                i = i + 1;
            }
            return U256 { limbs: out };
        }
        let inv_shift: u32 = (64 as u32) - bit_shift;
        let i: usize = word_shift;
        while (i < 4) {
            let src: usize = i - word_shift;
            let val: u64 = _.limbs[src] << bit_shift;
            if (src > 0) {
                val = val | (_.limbs[src - 1] >> inv_shift);
            }
            out[i] = val;
            i = i + 1;
        }
        return U256 { limbs: out };
    }

    pub fn shr(bits: u32): U256 {
        if (bits >= 256) {
            return U256.zero();
        }
        if (bits == 0) {
            return U256 { limbs: [_.limbs[0], _.limbs[1], _.limbs[2], _.limbs[3]] };
        }
        let word_shift: usize = (bits >> 6) as usize;
        let bit_shift: u32 = bits & 63;
        let out: [4]u64 = [0, 0, 0, 0];
        if (bit_shift == 0) {
            let i: usize = 0;
            while (i + word_shift < 4) {
                out[i] = _.limbs[i + word_shift];
                i = i + 1;
            }
            return U256 { limbs: out };
        }
        let inv_shift: u32 = (64 as u32) - bit_shift;
        let i: usize = 0;
        while (i + word_shift < 4) {
            let src: usize = i + word_shift;
            let val: u64 = _.limbs[src] >> bit_shift;
            if (src + 1 < 4) {
                val = val | (_.limbs[src + 1] << inv_shift);
            }
            out[i] = val;
            i = i + 1;
        }
        return U256 { limbs: out };
    }

    pub fn bitAnd(other: U256): U256 {
        return U256 {
            limbs: [
                _.limbs[0] & other.limbs[0],
                _.limbs[1] & other.limbs[1],
                _.limbs[2] & other.limbs[2],
                _.limbs[3] & other.limbs[3],
            ],
        };
    }

    pub fn bitOr(other: U256): U256 {
        return U256 {
            limbs: [
                _.limbs[0] | other.limbs[0],
                _.limbs[1] | other.limbs[1],
                _.limbs[2] | other.limbs[2],
                _.limbs[3] | other.limbs[3],
            ],
        };
    }

    pub fn bitXor(other: U256): U256 {
        return U256 {
            limbs: [
                _.limbs[0] ^ other.limbs[0],
                _.limbs[1] ^ other.limbs[1],
                _.limbs[2] ^ other.limbs[2],
                _.limbs[3] ^ other.limbs[3],
            ],
        };
    }

    pub fn leadingZeros(): u32 {
        if (_.limbs[3] != 0) { return clz64(_.limbs[3]); }
        if (_.limbs[2] != 0) { return clz64(_.limbs[2]) + 64; }
        if (_.limbs[1] != 0) { return clz64(_.limbs[1]) + 128; }
        if (_.limbs[0] != 0) { return clz64(_.limbs[0]) + 192; }
        return 256;
    }

    pub fn bitLen(): u32 {
        return (256 as u32) - _.leadingZeros();
    }

    pub fn divRem(divisor: U256): DivRem256 {
        if (divisor.isZero()) {
            return DivRem256 { quot: U256.zero(), rem: U256.zero() };
        }
        let c: i32 = _.cmp(divisor);
        if (c < 0) {
            return DivRem256 {
                quot: U256.zero(),
                rem: U256 { limbs: [_.limbs[0], _.limbs[1], _.limbs[2], _.limbs[3]] },
            };
        }
        if (c == 0) {
            return DivRem256 { quot: U256.one(), rem: U256.zero() };
        }

        let quot: U256 = U256.zero();
        let rem: U256 = U256.zero();
        let bits: u32 = _.bitLen();
        let b: u32 = bits;
        while (b > 0) {
            b = b - 1;
            rem = rem.shl(1);
            let limb_idx: usize = (b >> 6) as usize;
            let bit_idx: u32 = b & 63;
            let cur_bit: u64 = (_.limbs[limb_idx] >> bit_idx) & 1;
            rem.limbs[0] = rem.limbs[0] | cur_bit;
            if (rem.cmp(divisor) >= 0) {
                rem = rem.wrappingSub(divisor);
                quot.limbs[limb_idx] = quot.limbs[limb_idx] | ((1 as u64) << bit_idx);
            }
        }
        return DivRem256 { quot: quot, rem: rem };
    }

    pub fn toHex(out: []u8): usize {
        if (out.len == 0) {
            return 0;
        }
        if (_.isZero()) {
            out[0] = 48;
            return 1;
        }
        let tmp: [64]u8 = [0; 64];
        let pos: usize = 0;
        let started: bool = false;
        let limb_rev: usize = 4;
        while (limb_rev > 0) {
            limb_rev = limb_rev - 1;
            let word: u64 = _.limbs[limb_rev];
            let nib_rev: usize = 16;
            while (nib_rev > 0) {
                nib_rev = nib_rev - 1;
                let shift: u32 = (nib_rev * 4) as u32;
                let nib: u8 = ((word >> shift) & 15) as u8;
                if (nib != 0 || started) {
                    started = true;
                    tmp[pos] = hexDigit(nib);
                    pos = pos + 1;
                }
            }
        }
        if (out.len < pos) {
            return 0;
        }
        let i: usize = 0;
        while (i < pos) {
            out[i] = tmp[i];
            i = i + 1;
        }
        return pos;
    }
}

pub fn zero(): U256 { return U256.zero(); }
pub fn one(): U256 { return U256.one(); }
pub fn fromU64(v: u64): U256 { return U256.fromU64(v); }
pub fn fromU128(lo: u64, hi: u64): U256 { return U256.fromU128(lo, hi); }
pub fn maxVal(): U256 { return U256.maxVal(); }
pub fn fromHex(s: string): U256 { return U256.fromHex(s); }
