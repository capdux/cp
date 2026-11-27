// @cap/rand-hc — HC-128 eSTREAM cryptographic stream cipher PRNG
// Inspired by crates.io:rand_hc (https://github.com/rust-random/rand)

fn rhcRotr32(x: u32, n: usize): u32 {
    return (x >> n) | (x << (32 - n));
}

fn rhcRotl32(x: u32, n: usize): u32 {
    return (x << n) | (x >> (32 - n));
}

fn rhcF1(x: u32): u32 {
    return rhcRotr32(x, 7) ^ rhcRotr32(x, 18) ^ (x >> 3);
}

fn rhcF2(x: u32): u32 {
    return rhcRotr32(x, 17) ^ rhcRotr32(x, 19) ^ (x >> 10);
}

fn rhcG1(x: u32, y: u32, z: u32): u32 {
    return (rhcRotr32(x, 10) ^ rhcRotr32(z, 23)) +% rhcRotr32(y, 8);
}

fn rhcG2(x: u32, y: u32, z: u32): u32 {
    return (rhcRotl32(x, 10) ^ rhcRotl32(z, 23)) +% rhcRotl32(y, 8);
}

fn rhcReadU32Le(seed: [32]u8, offset: usize): u32 {
    const b0: u32 = u32(seed[offset]);
    const b1: u32 = u32(seed[offset + 1]);
    const b2: u32 = u32(seed[offset + 2]);
    const b3: u32 = u32(seed[offset + 3]);
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

/// HC-128 cryptographic pseudo-random number generator (eSTREAM Profile 1).
pub struct Hc128Rng {
    p: [512]u32,
    q: [512]u32,
    counter: usize,

    fn h1(x: u32): u32 {
        const lo: usize = usize(x & 0xFF);
        const hi: usize = 256 + usize((x >> 16) & 0xFF);
        return _.q[lo] +% _.q[hi];
    }

    fn h2(x: u32): u32 {
        const lo: usize = usize(x & 0xFF);
        const hi: usize = 256 + usize((x >> 16) & 0xFF);
        return _.p[lo] +% _.p[hi];
    }

    /// Initializes HC-128 from a 32-byte seed (16-byte key + 16-byte IV)
    /// using the 1280-word key/IV expansion and 1024-step internal warm-up.
    static fn fromSeed(seed: [32]u8): Hc128Rng {
        let w: [1280]u32 = [0; 1280];

        // Load 128-bit key into W[0..4] and duplicate into W[4..8]
        let i: usize = 0;
        while (i < 4) : (i += 1) {
            const kw: u32 = rhcReadU32Le(seed, i * 4);
            w[i] = kw;
            w[i + 4] = kw;
        }

        // Load 128-bit IV into W[8..12] and duplicate into W[12..16]
        i = 0;
        while (i < 4) : (i += 1) {
            const ivw: u32 = rhcReadU32Le(seed, 16 + i * 4);
            w[i + 8] = ivw;
            w[i + 12] = ivw;
        }

        // Expand W[16..1280]
        i = 16;
        while (i < 1280) : (i += 1) {
            w[i] = rhcF2(w[i - 2]) +% w[i - 7] +% rhcF1(w[i - 15]) +% w[i - 16] +% u32(i);
        }

        let rng: Hc128Rng = Hc128Rng{
            p: [0; 512],
            q: [0; 512],
            counter: 0,
        };

        // Populate P[0..512] = W[256..768], Q[0..512] = W[768..1280]
        i = 0;
        while (i < 512) : (i += 1) {
            rng.p[i] = w[i + 256];
            rng.q[i] = w[i + 768];
        }

        // Run 1024 warm-up steps (512 on P, then 512 on Q)
        i = 0;
        while (i < 512) : (i += 1) {
            const g: u32 = rhcG1(
                rng.p[(i + 509) & 511],
                rng.p[(i + 502) & 511],
                rng.p[(i + 1) & 511],
            );
            const h: u32 = rng.h1(rng.p[(i + 500) & 511]);
            rng.p[i] = (rng.p[i] +% g) ^ h;
        }

        i = 0;
        while (i < 512) : (i += 1) {
            const g: u32 = rhcG2(
                rng.q[(i + 509) & 511],
                rng.q[(i + 502) & 511],
                rng.q[(i + 1) & 511],
            );
            const h: u32 = rng.h2(rng.q[(i + 500) & 511]);
            rng.q[i] = (rng.q[i] +% g) ^ h;
        }

        rng.counter = 0;
        return rng;
    }

    /// Generates the next 32-bit keystream word.
    fn nextU32(*_): u32 {
        const j: usize = _.counter & 511;
        let out: u32 = 0;

        if (_.counter < 512) {
            const g: u32 = rhcG1(
                _.p[(j + 509) & 511],
                _.p[(j + 502) & 511],
                _.p[(j + 1) & 511],
            );
            _.p[j] = _.p[j] +% g;
            out = _.h1(_.p[(j + 500) & 511]) ^ _.p[j];
        } else {
            const g: u32 = rhcG2(
                _.q[(j + 509) & 511],
                _.q[(j + 502) & 511],
                _.q[(j + 1) & 511],
            );
            _.q[j] = _.q[j] +% g;
            out = _.h2(_.q[(j + 500) & 511]) ^ _.q[j];
        }

        _.counter = (_.counter + 1) & 1023;
        return out;
    }

    /// Generates the next 64-bit unsigned integer by combining two little-endian `nextU32()` words.
    fn nextU64(*_): u64 {
        const lo: u64 = u64(_.nextU32());
        const hi: u64 = u64(_.nextU32());
        return (hi << 32) | lo;
    }

    /// Fills `out` with keystream bytes in little-endian word order.
    fn fillBytes(*_, out: []u8) {
        let pos: usize = 0;
        while (pos < out.len) {
            let word: u32 = _.nextU32();
            let b: usize = 0;
            while (b < 4 && pos < out.len) : (b += 1) {
                out[pos] = u8(word & 0xFF);
                word = word >> 8;
                pos += 1;
            }
        }
    }
}
