// @cap/xorshift — Pure CP zero-allocation Marsaglia Xorshift PRNG family
// Parity with Rust `rand_xorshift::XorShiftRng` (XorShift128) + XorShift64* + XorShift32.

fn xsU64FromHalves(hi: u32, lo: u32): u64 {
    return ((hi as u64) << 32) | (lo as u64);
}

fn xsAddWrap64(a: u64, b: u64): u64 {
    let lo_sum: u64 = (a & 0xFFFFFFFF) + (b & 0xFFFFFFFF);
    let carry: u64 = lo_sum >> 32;
    let hi_sum: u64 = ((a >> 32) + (b >> 32) + carry) & 0xFFFFFFFF;
    return (hi_sum << 32) | (lo_sum & 0xFFFFFFFF);
}

fn xsMulWrap64(a: u64, b: u64): u64 {
    let a_lo: u64 = a & 0xFFFFFFFF;
    let a_hi: u64 = a >> 32;
    let b_lo: u64 = b & 0xFFFFFFFF;
    let b_hi: u64 = b >> 32;
    let p0: u64 = a_lo * b_lo;
    let p1: u64 = (a_lo * b_hi) & 0xFFFFFFFF;
    let p2: u64 = (a_hi * b_lo) & 0xFFFFFFFF;
    let hi: u64 = ((p0 >> 32) + p1 + p2) & 0xFFFFFFFF;
    return (hi << 32) | (p0 & 0xFFFFFFFF);
}

fn xsSplitMixNext(st: *u64): u64 {
    let inc = xsU64FromHalves(0x9E3779B9, 0x7F4A7C15);
    st.* = xsAddWrap64(st.*, inc);
    let z = st.*;
    z = xsMulWrap64(z ^ (z >> 30), xsU64FromHalves(0xBF58476D, 0x1CE4E5B9));
    z = xsMulWrap64(z ^ (z >> 27), xsU64FromHalves(0x94D049BB, 0x133111EB));
    return z ^ (z >> 31);
}

fn xsReadU32Le(b: [16]u8, off: usize): u32 {
    return (b[off] as u32)
        | ((b[off + 1] as u32) << 8)
        | ((b[off + 2] as u32) << 16)
        | ((b[off + 3] as u32) << 24);
}

pub struct XorShift128 {
    pub x: u32,
    pub y: u32,
    pub z: u32,
    pub w: u32,

    pub static fn fromSeed(seed: [16]u8): XorShift128 {
        let x = xsReadU32Le(seed, 0);
        let y = xsReadU32Le(seed, 4);
        let z = xsReadU32Le(seed, 8);
        let w = xsReadU32Le(seed, 12);
        if (x == 0 && y == 0 && z == 0 && w == 0) {
            return XorShift128 {
                x: 0x193a6754,
                y: 0xa8a7d469,
                z: 0x97830e05,
                w: 0x113ba7bb,
            };
        }
        return XorShift128 { x: x, y: y, z: z, w: w };
    }

    pub static fn fromU64(seed: u64): XorShift128 {
        let st = seed;
        let a = xsSplitMixNext(&st);
        let b = xsSplitMixNext(&st);
        let x = (a & 0xFFFFFFFF) as u32;
        let y = (a >> 32) as u32;
        let z = (b & 0xFFFFFFFF) as u32;
        let w = (b >> 32) as u32;
        if (x == 0 && y == 0 && z == 0 && w == 0) {
            return XorShift128 {
                x: 0x193a6754,
                y: 0xa8a7d469,
                z: 0x97830e05,
                w: 0x113ba7bb,
            };
        }
        return XorShift128 { x: x, y: y, z: z, w: w };
    }

    pub fn nextU32(*_): u32 {
        let xv = _.x;
        let t = xv ^ (xv << 11);
        _.x = _.y;
        _.y = _.z;
        _.z = _.w;
        let wv = _.w;
        _.w = wv ^ (wv >> 19) ^ (t ^ (t >> 8));
        return _.w;
    }

    pub fn nextU64(*_): u64 {
        let lo = _.nextU32() as u64;
        let hi = _.nextU32() as u64;
        return (hi << 32) | lo;
    }

    pub fn nextF64(*_): f64 {
        let bits: u64 = _.nextU64() >> 11;
        return (bits as f64) / 9007199254740992.0;
    }

    pub fn fillBytes(*_, out: []u8) {
        let i: usize = 0;
        while (i + 4 <= out.len) {
            let v = _.nextU32();
            out[i] = (v & 0xFF) as u8;
            out[i + 1] = ((v >> 8) & 0xFF) as u8;
            out[i + 2] = ((v >> 16) & 0xFF) as u8;
            out[i + 3] = ((v >> 24) & 0xFF) as u8;
            i = i + 4;
        }
        if (i < out.len) {
            let v = _.nextU32();
            let shift: u32 = 0;
            while (i < out.len) {
                out[i] = ((v >> (shift as u5)) & 0xFF) as u8;
                shift = shift + 8;
                i = i + 1;
            }
        }
    }
}

pub struct XorShift64Star {
    pub state: u64,

    pub static fn new(seed: u64): XorShift64Star {
        if (seed == 0) {
            return XorShift64Star {
                state: xsU64FromHalves(0x9E3779B9, 0x7F4A7C15),
            };
        }
        return XorShift64Star { state: seed };
    }

    pub fn nextU64(*_): u64 {
        let s = _.state;
        s = s ^ (s >> 12);
        s = s ^ (s << 25);
        s = s ^ (s >> 27);
        _.state = s;
        let mult = xsU64FromHalves(0x2545F491, 0x4F6CDD1D);
        return xsMulWrap64(s, mult);
    }

    pub fn nextU32(*_): u32 {
        return (_.nextU64() >> 32) as u32;
    }
}

pub struct XorShift32 {
    pub state: u32,

    pub static fn new(seed: u32): XorShift32 {
        if (seed == 0) {
            return XorShift32 { state: 0x9E3779B9 };
        }
        return XorShift32 { state: seed };
    }

    pub fn nextU32(*_): u32 {
        let s = _.state;
        s = s ^ (s << 13);
        s = s ^ (s >> 17);
        s = s ^ (s << 5);
        _.state = s;
        return s;
    }
}

export {
    XorShift128,
    XorShift64Star,
    XorShift32
};
