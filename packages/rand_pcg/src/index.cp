// @cap/pcg — Zero-heap PCG32 (PCG-XSH-RR) & PCG64Mcg (MCG-128-XSL-RR) PRNGs
// with Independent Streams & O(log N) Jump-Ahead.
// Clean-room MIT implementation in pure CP (modeled on Rust `rand_pcg` [295M+ downloads]).

const PCG_MULT: u64 = 6364136223846793005;
const PCG_DEFAULT_STREAM: u64 = 721347520444481703; // (stream << 1) | 1 == 1442695040888963407

// PCG64Mcg 128-bit multiplier: 0x2360_ED05_1FC6_5DA4_4385_DF64_9FCC_F645
const PCG64_MULT_HI: u64 = 0x2360ED051FC65DA4;
const PCG64_MULT_LO: u64 = 0x4385DF649FCCF645;

// Overflow-free 64-bit wrapping addition via 32-bit halves
fn wrapAdd64(a: u64, b: u64) -> u64 {
    const lo: u64 = (a & 0xFFFFFFFF) + (b & 0xFFFFFFFF);
    const hi: u64 = (a >> 32) + (b >> 32) + (lo >> 32);
    return ((hi & 0xFFFFFFFF) << 32) | (lo & 0xFFFFFFFF);
}

// Overflow-free 64-bit wrapping multiplication (mod 2^64) via 32-bit halves
fn wrapMul64(a: u64, b: u64) -> u64 {
    const a0: u64 = a & 0xFFFFFFFF;
    const a1: u64 = a >> 32;
    const b0: u64 = b & 0xFFFFFFFF;
    const b1: u64 = b >> 32;
    const p00: u64 = a0 * b0;
    const p01: u64 = a0 * b1;
    const p10: u64 = a1 * b0;
    const hi: u64 = ((p00 >> 32) + (p01 & 0xFFFFFFFF) + (p10 & 0xFFFFFFFF)) & 0xFFFFFFFF;
    return (hi << 32) | (p00 & 0xFFFFFFFF);
}

struct U128Pair {
    hi: u64,
    lo: u64,
}

// Full 64x64 -> 128-bit unsigned multiplication
fn mul64Full(a: u64, b: u64) -> U128Pair {
    const a0: u64 = a & 0xFFFFFFFF;
    const a1: u64 = a >> 32;
    const b0: u64 = b & 0xFFFFFFFF;
    const b1: u64 = b >> 32;

    const p00: u64 = a0 * b0;
    const p01: u64 = a0 * b1;
    const p10: u64 = a1 * b0;
    const p11: u64 = a1 * b1;

    const mid: u64 = (p00 >> 32) + (p01 & 0xFFFFFFFF) + (p10 & 0xFFFFFFFF);
    const lo: u64 = ((mid & 0xFFFFFFFF) << 32) | (p00 & 0xFFFFFFFF);
    const hi: u64 = p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);
    return U128Pair { hi: hi, lo: lo };
}

// 128-bit wrapping multiplication (mod 2^128)
fn wrapMul128(a_hi: u64, a_lo: u64, b_hi: u64, b_lo: u64) -> U128Pair {
    const prod = mul64Full(a_lo, b_lo);
    const cross1: u64 = wrapMul64(a_lo, b_hi);
    const cross2: u64 = wrapMul64(a_hi, b_lo);
    const hi: u64 = wrapAdd64(wrapAdd64(prod.hi, cross1), cross2);
    return U128Pair { hi: hi, lo: prod.lo };
}

fn rotr32(x: u32, r: u32) -> u32 {
    const s: u32 = r & 31;
    if (s == 0) { return x; }
    return (x >> s) | (x << (32 - s));
}

fn rotr64(x: u64, r: u32) -> u64 {
    const s: u6 = (r & 63) as u6;
    if (s == 0) { return x; }
    const inv: u6 = ((64 as u32) - (s as u32)) as u6;
    return (x >> s) | (x << inv);
}

fn splitmix64Next(x_in: u64) -> u64 {
    const gamma: u64 = ((0x9E3779B9 as u64) << 32) | (0x7F4A7C15 as u64);
    const m1: u64 = ((0xBF58476D as u64) << 32) | (0x1CE4E5B9 as u64);
    const m2: u64 = ((0x94D049BB as u64) << 32) | (0x133111EB as u64);
    let z: u64 = wrapAdd64(x_in, gamma);
    z = wrapMul64(z ^ (z >> 30), m1);
    z = wrapMul64(z ^ (z >> 27), m2);
    return z ^ (z >> 31);
}

pub struct Pcg32 {
    pub state: u64,
    pub inc: u64,

    /// Create a PCG32 generator with a 64-bit seed and 63-bit independent stream ID.
    pub static fn init(seed: u64, stream_id: u64) -> Pcg32 {
        const inc: u64 = (stream_id << 1) | 1;
        let rng: Pcg32 = Pcg32 { state: 0, inc: inc };
        * = rng.nextU32();
        rng.state = wrapAdd64(rng.state, seed);
        * = rng.nextU32();
        return rng;
    }

    /// Upstream `rand_pcg::Pcg32::new(state, stream)` constructor.
    pub static fn new(state: u64, stream: u64) -> Pcg32 {
        return Pcg32.init(state, stream);
    }

    /// Upstream `SeedableRng::seed_from_u64` constructor using the canonical default stream.
    pub static fn seedFromU64(seed: u64) -> Pcg32 {
        return Pcg32.init(seed, PCG_DEFAULT_STREAM);
    }

    /// Generate next 32-bit pseudorandom unsigned integer (PCG-XSH-RR).
    pub fn nextU32(*_) -> u32 {
        const old: u64 = _.state;
        _.state = wrapAdd64(wrapMul64(old, PCG_MULT), _.inc);
        const xorshifted: u32 = (((old >> 18) ^ old) >> 27) as u32;
        const rot: u32 = (old >> 59) as u32;
        return rotr32(xorshifted, rot);
    }

    /// Generate next 64-bit pseudorandom unsigned integer by combining two u32 draws.
    pub fn nextU64(*_) -> u64 {
        const hi: u64 = _.nextU32() as u64;
        const lo: u64 = _.nextU32() as u64;
        return (hi << 32) | lo;
    }

    /// Generate a uniform u32 in `[0, bound)` without modulo bias (rejection sampling).
    pub fn nextBoundedU32(*_, bound: u32) -> u32 {
        if (bound <= 1) { return 0; }
        const threshold: u32 = (((0xFFFFFFFF as u32) - bound) + (1 as u32)) % bound;
        while (true) {
            const r: u32 = _.nextU32();
            if (r >= threshold) {
                return r % bound;
            }
        }
        return 0;
    }

    /// Generate a uniform f32 in `[0.0, 1.0)` with 24 bits of precision.
    pub fn nextF32(*_) -> f32 {
        const v: u32 = _.nextU32() >> 8;
        return (v as f32) * (1.0 / 16777216.0);
    }

    /// Generate a uniform f64 in `[0.0, 1.0)` with 53 bits of precision.
    pub fn nextF64(*_) -> f64 {
        const v: u64 = _.nextU64() >> 11;
        return (v as f64) * (1.0 / 9007199254740992.0);
    }

    /// Generate a uniform boolean.
    pub fn nextBool(*_) -> bool {
        return (_.nextU32() & 1) != 0;
    }

    /// Fill `dst` with pseudorandom bytes (little-endian 32-bit chunks).
    pub fn fillBytes(*_, dst: []u8) {
        let i: usize = 0;
        while (i < dst.len) {
            let v: u32 = _.nextU32();
            let k: usize = 0;
            while (k < 4 && i < dst.len) {
                dst[i] = (v & 0xFF) as u8;
                v = v >> 8;
                i += 1;
                k += 1;
            }
        }
    }

    /// Jump the generator state forward by `delta` steps in O(log(delta)) time!
    pub fn advance(*_, delta_in: u64) {
        let cur_mult: u64 = PCG_MULT;
        let cur_plus: u64 = _.inc;
        let acc_mult: u64 = 1;
        let acc_plus: u64 = 0;
        let delta: u64 = delta_in;

        while (delta > 0) {
            if ((delta & 1) != 0) {
                acc_mult = wrapMul64(acc_mult, cur_mult);
                acc_plus = wrapAdd64(wrapMul64(acc_plus, cur_mult), cur_plus);
            }
            cur_plus = wrapMul64(wrapAdd64(cur_mult, 1), cur_plus);
            cur_mult = wrapMul64(cur_mult, cur_mult);
            delta = delta >> 1;
        }
        _.state = wrapAdd64(wrapMul64(acc_mult, _.state), acc_plus);
    }
}

/// PCG64Mcg (MCG-128-XSL-RR): fast 64-bit output PRNG with 128-bit multiplicative congruential state.
/// Modeled on Rust `rand_pcg::Pcg64Mcg` (`Mcg128Xsl64`).
pub struct Pcg64Mcg {
    pub state_hi: u64,
    pub state_lo: u64,

    /// Create a Pcg64Mcg generator from a 128-bit state `(state_hi, state_lo)`.
    /// Lowest bit of `state_lo` is forced to 1 per MCG invariant.
    pub static fn new(state_hi: u64, state_lo: u64) -> Pcg64Mcg {
        return Pcg64Mcg {
            state_hi: state_hi,
            state_lo: state_lo | 1,
        };
    }

    /// Upstream `SeedableRng::seed_from_u64` constructor (expands 64-bit seed to 128-bit state via SplitMix64).
    pub static fn seedFromU64(seed: u64) -> Pcg64Mcg {
        const gamma: u64 = ((0x9E3779B9 as u64) << 32) | (0x7F4A7C15 as u64);
        const lo: u64 = splitmix64Next(seed);
        const hi: u64 = splitmix64Next(wrapAdd64(seed, gamma));
        return Pcg64Mcg.new(hi, lo);
    }

    /// Generate next 64-bit pseudorandom unsigned integer (MCG-128-XSL-RR).
    pub fn nextU64(*_) -> u64 {
        const next = wrapMul128(_.state_hi, _.state_lo, PCG64_MULT_HI, PCG64_MULT_LO);
        _.state_hi = next.hi;
        _.state_lo = next.lo;
        const xorshifted: u64 = _.state_hi ^ _.state_lo;
        const rot: u32 = (_.state_hi >> 58) as u32;
        return rotr64(xorshifted, rot);
    }

    /// Generate next 32-bit pseudorandom unsigned integer from the upper half of `nextU64`.
    pub fn nextU32(*_) -> u32 {
        return (_.nextU64() >> 32) as u32;
    }

    /// Generate a uniform f64 in `[0.0, 1.0)` with 53 bits of precision.
    pub fn nextF64(*_) -> f64 {
        const v: u64 = _.nextU64() >> 11;
        return (v as f64) * (1.0 / 9007199254740992.0);
    }

    /// Fill `dst` with pseudorandom bytes (little-endian 64-bit chunks).
    pub fn fillBytes(*_, dst: []u8) {
        let i: usize = 0;
        while (i < dst.len) {
            let v: u64 = _.nextU64();
            let k: usize = 0;
            while (k < 8 && i < dst.len) {
                dst[i] = (v & 0xFF) as u8;
                v = v >> 8;
                i += 1;
                k += 1;
            }
        }
    }

    /// Jump the 128-bit MCG state forward by `delta` steps in O(log(delta)) time!
    pub fn advance(*_, delta_in: u64) {
        let cur_hi: u64 = PCG64_MULT_HI;
        let cur_lo: u64 = PCG64_MULT_LO;
        let acc_hi: u64 = 0;
        let acc_lo: u64 = 1;
        let delta: u64 = delta_in;

        while (delta > 0) {
            if ((delta & 1) != 0) {
                const a = wrapMul128(acc_hi, acc_lo, cur_hi, cur_lo);
                acc_hi = a.hi;
                acc_lo = a.lo;
            }
            const c = wrapMul128(cur_hi, cur_lo, cur_hi, cur_lo);
            cur_hi = c.hi;
            cur_lo = c.lo;
            delta = delta >> 1;
        }
        const res = wrapMul128(_.state_hi, _.state_lo, acc_hi, acc_lo);
        _.state_hi = res.hi;
        _.state_lo = res.lo;
    }
}
