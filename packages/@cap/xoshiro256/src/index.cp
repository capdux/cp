// @cap/xoshiro256 — Zero-allocation Blackman & Vigna xoshiro256** & SplitMix64 PRNG
// Clean-room MIT implementation in pure CP.

pub struct Xoshiro256 {
    pub s: [4]u64,
}

fn makeU64(hi: u32, lo: u32) -> u64 {
    return ((hi as u64) << 32) | (lo as u64);
}

fn rotl7(x: u64) -> u64 {
    return (x << 7) | (x >> 57);
}

fn rotl45(x: u64) -> u64 {
    return (x << 45) | (x >> 19);
}

/// Advances a SplitMix64 state and returns the next 64-bit pseudorandom value.
pub fn splitmix64Next(state: *u64) -> u64 {
    const inc = makeU64(0x9E3779B9, 0x7F4A7C15);
    const m1 = makeU64(0xBF58476D, 0x1CE4E5B9);
    const m2 = makeU64(0x94D049BB, 0x133111EB);

    state.* = state.* +% inc;
    let z = state.*;
    z = (z ^ (z >> 30)) *% m1;
    z = (z ^ (z >> 27)) *% m2;
    return z ^ (z >> 31);
}

/// Initializes a 256-bit `Xoshiro256` state from a 64-bit `seed` using 4 SplitMix64 steps.
/// Even `seed == 0` yields a well-mixed non-zero state.
pub fn seedFromU64(seed: u64) -> Xoshiro256 {
    let sm = seed;
    const s0 = splitmix64Next(&sm);
    const s1 = splitmix64Next(&sm);
    const s2 = splitmix64Next(&sm);
    const s3 = splitmix64Next(&sm);
    return Xoshiro256 { s: [s0, s1, s2, s3] };
}

/// Generates the next 64-bit pseudorandom unsigned integer using `xoshiro256**`.
pub fn nextU64(rng: *Xoshiro256) -> u64 {
    const result = rotl7(rng.s[1] *% 5) *% 9;
    const t = rng.s[1] << 17;

    rng.s[2] ^= rng.s[0];
    rng.s[3] ^= rng.s[1];
    rng.s[1] ^= rng.s[2];
    rng.s[0] ^= rng.s[3];
    rng.s[2] ^= t;
    rng.s[3] = rotl45(rng.s[3]);

    return result;
}

/// Generates a uniformly distributed IEEE-754 `f64` in `[0.0, 1.0)` with 53 bits of precision.
pub fn nextF64(rng: *Xoshiro256) -> f64 {
    return ((nextU64(rng) >> 11) as f64) * (1.0 / 9007199254740992.0);
}

/// Generates an unbiased random integer in `[0, upper_exclusive)` using rejection sampling.
/// Returns `0` when `upper_exclusive <= 1`.
pub fn nextRange(rng: *Xoshiro256, upper_exclusive: u64) -> u64 {
    if (upper_exclusive <= 1) {
        return 0;
    }
    const u64_max = makeU64(0xFFFFFFFF, 0xFFFFFFFF);
    const threshold = (u64_max - upper_exclusive + 1) % upper_exclusive;
    while (true) {
        const r = nextU64(rng);
        if (r >= threshold) {
            return r % upper_exclusive;
        }
    }
}

/// Advances the generator state by `2^128` steps (official Blackman & Vigna jump polynomial),
/// producing non-overlapping parallel substreams.
pub fn jump(rng: *Xoshiro256) {
    const j0 = makeU64(0x180EC6D3, 0x3CFD0ABA);
    const j1 = makeU64(0xD5A61266, 0xF0C9392C);
    const j2 = makeU64(0xA9582618, 0xE03FC9AA);
    const j3 = makeU64(0x39ABDC45, 0x29B1661C);
    const coeffs: [4]u64 = [j0, j1, j2, j3];

    let s0: u64 = 0;
    let s1: u64 = 0;
    let s2: u64 = 0;
    let s3: u64 = 0;

    let i: usize = 0;
    while (i < 4) {
        let b: u32 = 0;
        while (b < 64) {
            if ((coeffs[i] & ((1 as u64) << (b as u6))) != 0) {
                s0 ^= rng.s[0];
                s1 ^= rng.s[1];
                s2 ^= rng.s[2];
                s3 ^= rng.s[3];
            }
            * = nextU64(rng);
            b += 1;
        }
        i += 1;
    }

    rng.s[0] = s0;
    rng.s[1] = s1;
    rng.s[2] = s2;
    rng.s[3] = s3;
}

/// In-place Fisher-Yates shuffle of `data` using unbiased `nextRange`.
pub fn shuffleU32(rng: *Xoshiro256, data: []u32) {
    if (data.len < 2) {
        return;
    }
    let i = data.len - 1;
    while (i > 0) {
        const j = nextRange(rng, (i + 1) as u64) as usize;
        const tmp = data[i];
        data[i] = data[j];
        data[j] = tmp;
        i -= 1;
    }
}
