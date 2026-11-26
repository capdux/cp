// @cap/pcg — Zero-heap PCG32 (PCG-XSH-RR) PRNG with Independent Streams & O(log N) Jump-Ahead
// Clean-room MIT implementation in pure CP (inspired by Rust `rand_pcg` [295M downloads]).

const PCG_MULT: u64 = 6364136223846793005;

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

fn rotr32(x: u32, r: u32) -> u32 {
    const s: u32 = r & 31;
    if (s == 0) { return x; }
    return (x >> s) | (x << (32 - s));
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

    /// Generate a uniform f64 in `[0.0, 1.0)` with 53 bits of precision.
    pub fn nextF64(*_) -> f64 {
        const v: u64 = _.nextU64() >> 11;
        return (v as f64) * (1.0 / 9007199254740992.0);
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
