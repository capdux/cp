// @cap/pcg32 — Zero-heap Melissa O'Neill PCG-XSH-RR 32-bit PRNG
// MIT License (clean-room CP implementation)

fn pcgMult() -> u64 {
    // 6364136223846793005 == 0x5851F42D4C957F2D
    return ((0x5851F42D as u64) << 32) | (0x4C957F2D as u64);
}

fn defaultStream() -> u64 {
    // 0xDA3E39CB94B95BDB
    return ((0xDA3E39CB as u64) << 32) | (0x94B95BDB as u64);
}

fn add64(a: u64, b: u64) -> u64 {
    const lo: u64 = (a & 0xFFFFFFFF) + (b & 0xFFFFFFFF);
    const carry: u64 = lo >> 32;
    const hi: u64 = ((a >> 32) + (b >> 32) + carry) & 0xFFFFFFFF;
    return (hi << 32) | (lo & 0xFFFFFFFF);
}

fn mul64(a: u64, b: u64) -> u64 {
    const a0: u64 = a & 0xFFFFFFFF;
    const a1: u64 = a >> 32;
    const b0: u64 = b & 0xFFFFFFFF;
    const b1: u64 = b >> 32;

    const p00: u64 = a0 * b0;
    const p01: u64 = (a0 * b1) & 0xFFFFFFFF;
    const p10: u64 = (a1 * b0) & 0xFFFFFFFF;
    const hi: u64 = ((p00 >> 32) + p01 + p10) & 0xFFFFFFFF;
    return (hi << 32) | (p00 & 0xFFFFFFFF);
}

fn rotr32(x: u32, rot: u32) -> u32 {
    const r: u32 = rot & 31;
    if (r == 0) {
        return x;
    }
    return (x >> (r as u5)) | (x << ((32 - r) as u5));
}

pub struct Pcg32 {
    pub state: u64,
    pub inc: u64,

    pub static fn new(seed: u64, stream: u64) -> Pcg32 {
        let rng = Pcg32 {
            state: 0,
            inc: (stream << 1) | 1,
        };
        * = rng.nextU32();
        rng.state = add64(rng.state, seed);
        * = rng.nextU32();
        return rng;
    }

    pub static fn fromSeed(seed: u64) -> Pcg32 {
        return Pcg32.new(seed, defaultStream());
    }

    pub fn nextU32(*_) -> u32 {
        const oldstate: u64 = _.state;
        _.state = add64(mul64(oldstate, pcgMult()), _.inc);
        const xorshifted: u32 = (((oldstate >> 18) ^ oldstate) >> 27) as u32;
        const rot: u32 = (oldstate >> 59) as u32;
        return rotr32(xorshifted, rot);
    }

    pub fn nextU64(*_) -> u64 {
        const hi: u64 = (_.nextU32() as u64) << 32;
        const lo: u64 = _.nextU32() as u64;
        return hi | lo;
    }

    pub fn nextF64(*_) -> f64 {
        const bits: u64 = _.nextU64() >> 11;
        return (bits as f64) / 9007199254740992.0;
    }

    pub fn nextBounded(*_, bound: u32) -> u32 {
        if (bound <= 1) {
            return 0;
        }
        const neg_b: u32 = ((0xFFFFFFFF as u32) - bound) + 1;
        const threshold: u32 = neg_b % bound;
        while (true) {
            const r: u32 = _.nextU32();
            if (r >= threshold) {
                return r % bound;
            }
        }
        return 0;
    }

    pub fn advance(*_, delta: u64) {
        let cur_mult: u64 = pcgMult();
        let cur_plus: u64 = _.inc;
        let acc_mult: u64 = 1;
        let acc_plus: u64 = 0;
        let d: u64 = delta;

        while (d > 0) {
            if ((d & 1) != 0) {
                acc_mult = mul64(acc_mult, cur_mult);
                acc_plus = add64(mul64(acc_plus, cur_mult), cur_plus);
            }
            cur_plus = mul64(add64(cur_mult, 1), cur_plus);
            cur_mult = mul64(cur_mult, cur_mult);
            d = d >> 1;
        }
        _.state = add64(mul64(acc_mult, _.state), acc_plus);
    }

    pub fn fillBytes(*_, out: []u8) {
        let i: usize = 0;
        while (i < out.len) {
            let v: u32 = _.nextU32();
            let k: usize = 0;
            while (k < 4 && i < out.len) {
                out[i] = (v & 0xFF) as u8;
                v = v >> 8;
                i += 1;
                k += 1;
            }
        }
    }
}
