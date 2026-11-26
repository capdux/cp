// @cap/ahash — Zero-allocation randomized high-speed hasher with 4-key RandomState & folded-multiply rounds for CP
// Clean-room MIT implementation inspired by Rust `ahash` (946M downloads).

fn makeU64(hi: u32, lo: u32) -> u64 {
    return ((hi as u64) * (0x100000000 as u64)) | (lo as u64);
}

fn addWrap64(a: u64, b: u64) -> u64 {
    const mask32: u64 = 0xFFFFFFFF;
    const shift32: u64 = 0x100000000;
    const lo = (a & mask32) + (b & mask32);
    const carry = lo / shift32;
    const hi = ((a / shift32) + (b / shift32) + carry) & mask32;
    return (hi * shift32) | (lo & mask32);
}

fn mulWrap64(a: u64, b: u64) -> u64 {
    const mask32: u64 = 0xFFFFFFFF;
    const shift32: u64 = 0x100000000;
    const a_lo = a & mask32;
    const a_hi = a / shift32;
    const b_lo = b & mask32;
    const b_hi = b / shift32;
    const p_lo = a_lo * b_lo;
    const mid1 = (a_lo * b_hi) & mask32;
    const mid2 = (a_hi * b_lo) & mask32;
    const hi_sum = ((p_lo / shift32) + mid1 + mid2) & mask32;
    return (hi_sum * shift32) | (p_lo & mask32);
}

fn pow2_64(n: u32) -> u64 {
    let v: u64 = 1;
    let i: u32 = 0;
    while (i < n) {
        v = v * (2 as u64);
        i += 1 as u32;
    }
    return v;
}

fn rotl64(x: u64, r_in: u32) -> u64 {
    const r = r_in % (64 as u32);
    if (r == (0 as u32)) {
        return x;
    }
    const loShift = pow2_64(r);
    const hiDiv = pow2_64((64 as u32) - r);
    return mulWrap64(x, loShift) | (x / hiDiv);
}

fn foldedMultiply(x: u64, y: u64) -> u64 {
    const mask32: u64 = 0xFFFFFFFF;
    const shift32: u64 = 0x100000000;

    const x0 = x & mask32;
    const x1 = x / shift32;
    const y0 = y & mask32;
    const y1 = y / shift32;

    const p00 = x0 * y0;
    const p01 = x0 * y1;
    const p10 = x1 * y0;
    const p11 = x1 * y1;

    const lo_lo = p00 & mask32;
    const mid = (p00 / shift32) + (p01 & mask32) + (p10 & mask32);
    const lo = lo_lo | ((mid & mask32) * shift32);
    const hi = p11 + (p01 / shift32) + (p10 / shift32) + (mid / shift32);

    return lo ^ hi;
}

fn multipleConst() -> u64 {
    return makeU64(0x63641362 as u32, 0x23846793 as u32);
}

fn pi0() -> u64 { return makeU64(0x243F6A88 as u32, 0x85A308D3 as u32); }
fn pi1() -> u64 { return makeU64(0x13198A2E as u32, 0x03707344 as u32); }
fn pi2() -> u64 { return makeU64(0xA4093822 as u32, 0x299F31D0 as u32); }
fn pi3() -> u64 { return makeU64(0x082EFA98 as u32, 0xEC4E6C89 as u32); }

fn readU64Le(data: []const u8, offset: usize) -> u64 {
    let v: u64 = 0;
    let mul: u64 = 1;
    let i: usize = 0;
    while (i < 8) {
        v = v | ((data[offset + i] as u64) * mul);
        if (i < 7) {
            mul = mul * (256 as u64);
        }
        i += 1;
    }
    return v;
}

fn packTail(tail: [8]u8, tail_len: usize) -> u64 {
    let v: u64 = 0;
    let mul: u64 = 1;
    let i: usize = 0;
    while (i < tail_len && i < 8) {
        v = v | ((tail[i] as u64) * mul);
        if (i < 7) {
            mul = mul * (256 as u64);
        }
        i += 1;
    }
    return v;
}

pub struct AHasher {
    pub buffer: u64,
    pub pad: u64,
    pub extra_keys: [2]u64,
    pub tail: [8]u8,
    pub tail_len: usize,
    pub total_len: u64,

    pub static fn init(k0: u64, k1: u64, k2: u64, k3: u64) -> AHasher {
        let ek: [2]u64 = undefined;
        ek[0] = k2 ^ pi2();
        ek[1] = k3 ^ pi3();

        let t: [8]u8 = undefined;
        let i: usize = 0;
        while (i < 8) {
            t[i] = 0 as u8;
            i += 1;
        }

        const mult = multipleConst();
        const b0 = foldedMultiply(k0 ^ pi0(), mult);
        const p0 = foldedMultiply(k1 ^ pi1(), mult);

        return AHasher{
            buffer: b0,
            pad: p0,
            extra_keys: ek,
            tail: t,
            tail_len: 0 as usize,
            total_len: 0 as u64,
        };
    }

    pub fn writeU8(*_, v: u8) {
        _.tail[_.tail_len] = v;
        _.tail_len += 1;
        _.total_len += 1 as u64;
        if (_.tail_len == 8) {
            const word = packTail(_.tail, 8 as usize);
            const mult = multipleConst();
            _.buffer = rotl64(foldedMultiply(addWrap64(word, _.buffer), mult), 23 as u32);
            _.pad = addWrap64(_.pad, word ^ _.extra_keys[0]);
            _.tail_len = 0 as usize;
        }
    }

    pub fn writeU32(*_, v: u32) {
        _.writeU8((v & (0xFF as u32)) as u8);
        _.writeU8(((v / (256 as u32)) & (0xFF as u32)) as u8);
        _.writeU8(((v / (65536 as u32)) & (0xFF as u32)) as u8);
        _.writeU8(((v / (16777216 as u32)) & (0xFF as u32)) as u8);
    }

    pub fn writeU64(*_, v: u64) {
        if (_.tail_len == 0) {
            const mult = multipleConst();
            _.buffer = rotl64(foldedMultiply(addWrap64(v, _.buffer), mult), 23 as u32);
            _.pad = addWrap64(_.pad, v ^ _.extra_keys[0]);
            _.total_len += 8 as u64;
            return;
        }
        let cur = v;
        let i: usize = 0;
        while (i < 8) {
            _.writeU8((cur & (0xFF as u64)) as u8);
            cur = cur / (256 as u64);
            i += 1;
        }
    }

    pub fn writeBytes(*_, data: []const u8) {
        let i: usize = 0;
        while (i < data.len && _.tail_len > 0) {
            _.writeU8(data[i]);
            i += 1;
        }
        const mult = multipleConst();
        while (i + 8 <= data.len) {
            const word = readU64Le(data, i);
            _.buffer = rotl64(foldedMultiply(addWrap64(word, _.buffer), mult), 23 as u32);
            _.pad = addWrap64(_.pad, word ^ _.extra_keys[0]);
            _.total_len += 8 as u64;
            i += 8;
        }
        while (i < data.len) {
            _.writeU8(data[i]);
            i += 1;
        }
    }

    pub fn finish() -> u64 {
        const mult = multipleConst();
        let buf = _.buffer;
        let p = _.pad;

        if (_.tail_len > 0) {
            const word = packTail(_.tail, _.tail_len);
            buf = rotl64(foldedMultiply(addWrap64(word, buf), mult), 23 as u32);
            p = addWrap64(p, word ^ _.extra_keys[0]);
        }

        // Mix total_len and extra_keys with data-dependent rotation
        buf = foldedMultiply(buf ^ _.total_len ^ _.extra_keys[0], mult);
        const rot = ((buf & (63 as u64)) as u32);
        buf = rotl64(buf, rot);
        buf = foldedMultiply(buf ^ p ^ _.extra_keys[1], mult);
        return buf;
    }
}

pub struct RandomState {
    pub k0: u64,
    pub k1: u64,
    pub k2: u64,
    pub k3: u64,

    pub static fn withSeeds(k0: u64, k1: u64, k2: u64, k3: u64) -> RandomState {
        return RandomState{
            k0: k0,
            k1: k1,
            k2: k2,
            k3: k3,
        };
    }

    pub static fn defaultState() -> RandomState {
        return RandomState{
            k0: pi0(),
            k1: pi1(),
            k2: pi2(),
            k3: pi3(),
        };
    }

    pub fn buildHasher() -> AHasher {
        return AHasher.init(_.k0, _.k1, _.k2, _.k3);
    }

    pub fn hashOneBytes(data: []const u8) -> u64 {
        let h = _.buildHasher();
        h.writeBytes(data);
        return h.finish();
    }

    pub fn hashOneU64(v: u64) -> u64 {
        let h = _.buildHasher();
        h.writeU64(v);
        return h.finish();
    }
}

pub fn hashBytes(data: []const u8, k0: u64, k1: u64) -> u64 {
    let h = AHasher.init(k0, k1, pi2(), pi3());
    h.writeBytes(data);
    return h.finish();
}

pub fn hashU64(v: u64, k0: u64, k1: u64) -> u64 {
    let h = AHasher.init(k0, k1, pi2(), pi3());
    h.writeU64(v);
    return h.finish();
}

export {
    RandomState,
    AHasher,
    hashBytes,
    hashU64,
};
