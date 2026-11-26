// @cap/crypto-bigint — Zero-allocation constant-time fixed-width U256 arithmetic
// Parity with RustCrypto crates.io:crypto-bigint (Apache-2.0 OR MIT)

fn cbiMask64(): u128 {
    return ((1 as u128) << 64) - (1 as u128);
}

fn cbiAllOnes64(): u64 {
    return ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
}

fn cbiCtMask(cond: bool): u64 {
    if (cond) {
        return cbiAllOnes64();
    }
    return 0 as u64;
}

fn cbiReadBeU64(b: []const u8, off: usize): u64 {
    let v: u64 = 0;
    let i: usize = 0;
    while (i < 8) {
        v = (v << 8) | (b[off + i] as u64);
        i += 1;
    }
    return v;
}

fn cbiReadLeU64(b: []const u8, off: usize): u64 {
    let v: u64 = 0;
    let i: usize = 0;
    while (i < 8) {
        v = v | ((b[off + i] as u64) << (i * 8));
        i += 1;
    }
    return v;
}

fn cbiWriteBeU64(v: u64, out: []u8, off: usize) {
    let i: usize = 0;
    while (i < 8) {
        const shift = (7 - i) * 8;
        out[off + i] = u8((v >> shift) & (0xFF as u64));
        i += 1;
    }
}

fn cbiWriteLeU64(v: u64, out: []u8, off: usize) {
    let i: usize = 0;
    while (i < 8) {
        out[off + i] = u8((v >> (i * 8)) & (0xFF as u64));
        i += 1;
    }
}

fn cbiHexNibble(ch: u8): i32 {
    if (ch >= ('0' as u8) && ch <= ('9' as u8)) {
        return (ch - ('0' as u8)) as i32;
    }
    if (ch >= ('a' as u8) && ch <= ('f' as u8)) {
        return ((ch - ('a' as u8)) as i32) + 10;
    }
    if (ch >= ('A' as u8) && ch <= ('F' as u8)) {
        return ((ch - ('A' as u8)) as i32) + 10;
    }
    return -1;
}

fn cbiHexChar(nibble: u8): u8 {
    if (nibble < (10 as u8)) {
        return ('0' as u8) + nibble;
    }
    return ('a' as u8) + (nibble - (10 as u8));
}

fn cbiBitLen64(v_in: u64): usize {
    let v = v_in;
    let n: usize = 0;
    while (v != (0 as u64)) {
        v = v >> 1;
        n += 1;
    }
    return n;
}

struct CbiAddRes {
    val: U256;
    carry: u64;
}

struct CbiSubRes {
    val: U256;
    borrow: u64;
}

fn cbiAddWithCarry(a: U256, b: U256): CbiAddRes {
    const mask = cbiMask64();
    let limbs: [4]u64 = @as([4]u64, [0, 0, 0, 0]);
    let carry: u64 = 0;
    let i: usize = 0;
    while (i < 4) {
        const sum: u128 = (a.limbs[i] as u128) + (b.limbs[i] as u128) + (carry as u128);
        limbs[i] = u64(sum & mask);
        carry = u64(sum >> 64);
        i += 1;
    }
    return CbiAddRes{ val: U256{ limbs: limbs }, carry: carry };
}

fn cbiSubWithBorrow(a: U256, b: U256): CbiSubRes {
    const mask = cbiMask64();
    const base: u128 = (1 as u128) << 64;
    let limbs: [4]u64 = @as([4]u64, [0, 0, 0, 0]);
    let borrow: u64 = 0;
    let i: usize = 0;
    while (i < 4) {
        const sub: u128 = (b.limbs[i] as u128) + (borrow as u128);
        const diff: u128 = (base + (a.limbs[i] as u128)) - sub;
        limbs[i] = u64(diff & mask);
        borrow = (1 as u64) - u64(diff >> 64);
        i += 1;
    }
    return CbiSubRes{ val: U256{ limbs: limbs }, borrow: borrow };
}

/// Constant-time fixed-width 256-bit unsigned integer stored as 4 little-endian 64-bit limbs.
pub struct U256 {
    limbs: [4]u64;

    static const ZERO: U256 = U256{ limbs: @as([4]u64, [0, 0, 0, 0]) };
    static const ONE: U256 = U256{ limbs: @as([4]u64, [1, 0, 0, 0]) };

    static fn fromU64(v: u64): U256 {
        let limbs: [4]u64 = @as([4]u64, [0, 0, 0, 0]);
        limbs[0] = v;
        return U256{ limbs: limbs };
    }

    static fn fromBeBytes(b: [32]u8): U256 {
        let localB: [32]u8 = b;
        let limbs: [4]u64 = @as([4]u64, [0, 0, 0, 0]);
        limbs[3] = cbiReadBeU64(localB[0..32], 0);
        limbs[2] = cbiReadBeU64(localB[0..32], 8);
        limbs[1] = cbiReadBeU64(localB[0..32], 16);
        limbs[0] = cbiReadBeU64(localB[0..32], 24);
        return U256{ limbs: limbs };
    }

    static fn fromLeBytes(b: [32]u8): U256 {
        let localB: [32]u8 = b;
        let limbs: [4]u64 = @as([4]u64, [0, 0, 0, 0]);
        limbs[0] = cbiReadLeU64(localB[0..32], 0);
        limbs[1] = cbiReadLeU64(localB[0..32], 8);
        limbs[2] = cbiReadLeU64(localB[0..32], 16);
        limbs[3] = cbiReadLeU64(localB[0..32], 24);
        return U256{ limbs: limbs };
    }

    fn toBeBytes(): [32]u8 {
        let out: [32]u8 = undefined;
        cbiWriteBeU64(_.limbs[3], out[0..32], 0);
        cbiWriteBeU64(_.limbs[2], out[0..32], 8);
        cbiWriteBeU64(_.limbs[1], out[0..32], 16);
        cbiWriteBeU64(_.limbs[0], out[0..32], 24);
        return out;
    }

    fn toLeBytes(): [32]u8 {
        let out: [32]u8 = undefined;
        cbiWriteLeU64(_.limbs[0], out[0..32], 0);
        cbiWriteLeU64(_.limbs[1], out[0..32], 8);
        cbiWriteLeU64(_.limbs[2], out[0..32], 16);
        cbiWriteLeU64(_.limbs[3], out[0..32], 24);
        return out;
    }

    static fn fromBeHex(hex: string): U256? {
        if (hex.len != 64) {
            return null;
        }
        let bytes: [32]u8 = undefined;
        let i: usize = 0;
        while (i < 32) {
            const hi = cbiHexNibble(hex[i * 2]);
            const lo = cbiHexNibble(hex[i * 2 + 1]);
            if (hi < 0 || lo < 0) {
                return null;
            }
            bytes[i] = u8(((hi as u32) << 4) | (lo as u32));
            i += 1;
        }
        return U256.fromBeBytes(bytes);
    }

    fn toHex(out: []u8): usize {
        if (out.len < 64) {
            return 0;
        }
        const bytes: [32]u8 = _.toBeBytes();
        let i: usize = 0;
        while (i < 32) {
            const b = bytes[i];
            out[i * 2] = cbiHexChar(b >> 4);
            out[i * 2 + 1] = cbiHexChar(b & (0x0F as u8));
            i += 1;
        }
        return 64;
    }

    fn wrappingAdd(other: U256): U256 {
        const res = cbiAddWithCarry(_, other);
        return res.val;
    }

    fn wrappingSub(other: U256): U256 {
        const res = cbiSubWithBorrow(_, other);
        return res.val;
    }

    fn wrappingMul(other: U256): U256 {
        const mask = cbiMask64();
        let out: [4]u64 = @as([4]u64, [0, 0, 0, 0]);
        let i: usize = 0;
        while (i < 4) {
            let carry: u64 = 0;
            let j: usize = 0;
            while (i + j < 4) {
                const prod: u128 = (out[i + j] as u128) + (_.limbs[i] as u128) * (other.limbs[j] as u128) + (carry as u128);
                out[i + j] = u64(prod & mask);
                carry = u64(prod >> 64);
                j += 1;
            }
            i += 1;
        }
        return U256{ limbs: out };
    }

    fn shl1(): U256 {
        let out: [4]u64 = @as([4]u64, [0, 0, 0, 0]);
        let carry: u64 = 0;
        let i: usize = 0;
        while (i < 4) {
            const v = _.limbs[i];
            out[i] = (v << 1) | carry;
            carry = v >> 63;
            i += 1;
        }
        return U256{ limbs: out };
    }

    fn shr1(): U256 {
        let out: [4]u64 = @as([4]u64, [0, 0, 0, 0]);
        let carry: u64 = 0;
        let k: usize = 0;
        while (k < 4) {
            const idx = 3 - k;
            const v = _.limbs[idx];
            out[idx] = (v >> 1) | (carry << 63);
            carry = v & (1 as u64);
            k += 1;
        }
        return U256{ limbs: out };
    }

    fn bitLen(): usize {
        let k: usize = 0;
        while (k < 4) {
            const idx = 3 - k;
            if (_.limbs[idx] != (0 as u64)) {
                return idx * 64 + cbiBitLen64(_.limbs[idx]);
            }
            k += 1;
        }
        return 0;
    }

    fn ctEq(other: U256): bool {
        let diff: u64 = 0;
        let i: usize = 0;
        while (i < 4) {
            diff = diff | (_.limbs[i] ^ other.limbs[i]);
            i += 1;
        }
        return diff == (0 as u64);
    }

    /// Constant-time comparison: returns -1 if self < other, 0 if equal, 1 if self > other.
    fn ctCmp(other: U256): i32 {
        const subAB = cbiSubWithBorrow(_, other);
        const subBA = cbiSubWithBorrow(other, _);
        const lt = subAB.borrow;
        const gt = subBA.borrow;
        return (gt as i32) - (lt as i32);
    }

    /// Constant-time select: returns `a` if `cond` is true, else `b`.
    static fn ctSelect(cond: bool, a: U256, b: U256): U256 {
        const mask = cbiCtMask(cond);
        let out: [4]u64 = @as([4]u64, [0, 0, 0, 0]);
        let i: usize = 0;
        while (i < 4) {
            out[i] = b.limbs[i] ^ ((a.limbs[i] ^ b.limbs[i]) & mask);
            i += 1;
        }
        return U256{ limbs: out };
    }

    /// Constant-time modular addition `(a + b) mod m` (for `a < m`, `b < m`).
    static fn addMod(a: U256, b: U256, m: U256): U256 {
        const added = cbiAddWithCarry(a, b);
        const subbed = cbiSubWithBorrow(added.val, m);
        const needSub = (added.carry != (0 as u64)) || (subbed.borrow == (0 as u64));
        return U256.ctSelect(needSub, subbed.val, added.val);
    }

    /// Constant-time modular subtraction `(a - b) mod m` (for `a < m`, `b < m`).
    static fn subMod(a: U256, b: U256, m: U256): U256 {
        const subbed = cbiSubWithBorrow(a, b);
        const fixed = cbiAddWithCarry(subbed.val, m);
        const underflow = subbed.borrow != (0 as u64);
        return U256.ctSelect(underflow, fixed.val, subbed.val);
    }
}

pub fn addMod(a: U256, b: U256, m: U256): U256 {
    return U256.addMod(a, b, m);
}

pub fn subMod(a: U256, b: U256, m: U256): U256 {
    return U256.subMod(a, b, m);
}
