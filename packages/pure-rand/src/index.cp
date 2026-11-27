// @cap/pure-rand — Zero-allocation pure functional immutable PRNG in pure CP
// Clean-room MIT implementation of npm `pure-rand` (#443, 124M/wk, fast-check core PRNG).

fn prRotl64(x: u64, k: u6): u64 {
    return (x << k) | (x >> ((64 as u64 - (k as u64)) as u6));
}

fn prSplitMix64Step(state: u64): u64 {
    const c1: u64 = ((0xBF58476D as u64) << 32) | (0x1CE4E5B9 as u64);
    const c2: u64 = ((0x94D049BB as u64) << 32) | (0x133111EB as u64);
    let z = state;
    z = (z ^ (z >> 30)) *% c1;
    z = (z ^ (z >> 27)) *% c2;
    return z ^ (z >> 31);
}

/// Immutable Xoroshiro128+ generator state. Every step returns a new state without mutating the old one.
pub struct Xoroshiro128Plus {
    pub s0: u64,
    pub s1: u64,

    pub static fn fromSeed(seed: u64): Xoroshiro128Plus {
        const inc: u64 = ((0x9E3779B9 as u64) << 32) | (0x7F4A7C15 as u64);
        const st1 = seed +% inc;
        const st2 = st1 +% inc;
        let a = prSplitMix64Step(st1);
        let b = prSplitMix64Step(st2);
        if (a == 0 && b == 0) {
            a = 1;
        }
        return Xoroshiro128Plus { s0: a, s1: b };
    }

    pub static fn fromState(s0: u64, s1: u64): Xoroshiro128Plus {
        return Xoroshiro128Plus { s0: s0, s1: s1 };
    }

    /// Pure step returning `(value, next_rng)` without mutating `this`.
    pub fn nextU64(): NextU64Result {
        const s0 = _.s0;
        let s1 = _.s1;
        const result = s0 +% s1;
        s1 = s1 ^ s0;
        const ns0 = prRotl64(s0, 24) ^ s1 ^ (s1 << 16);
        const ns1 = prRotl64(s1, 37);
        return NextU64Result {
            value: result,
            next: Xoroshiro128Plus { s0: ns0, s1: ns1 },
        };
    }

    /// Pure step returning signed 32-bit integer and next generator state.
    pub fn nextI32(): NextI32Result {
        const r = _.nextU64();
        const hi: u32 = ((r.value >> 32) & 0xFFFFFFFF) as u32;
        return NextI32Result {
            value: @as(i32, @bitCast(hi)),
            next: r.next,
        };
    }

    /// Equivalent to 2^64 calls to `nextU64()`, producing an independent non-overlapping substream.
    pub fn jump(): Xoroshiro128Plus {
        const j0: u64 = ((0xDF900294 as u64) << 32) | (0xD8F554A5 as u64);
        const j1: u64 = ((0x170865DF as u64) << 32) | (0x4B3201FC as u64);
        let ns0: u64 = 0;
        let ns1: u64 = 0;
        let cur = Xoroshiro128Plus { s0: _.s0, s1: _.s1 };
        let b: usize = 0;
        while (b < 64) {
            if (((j0 >> (b as u6)) & 1) != 0) {
                ns0 ^= cur.s0;
                ns1 ^= cur.s1;
            }
            cur = cur.nextU64().next;
            b += 1;
        }
        b = 0;
        while (b < 64) {
            if (((j1 >> (b as u6)) & 1) != 0) {
                ns0 ^= cur.s0;
                ns1 ^= cur.s1;
            }
            cur = cur.nextU64().next;
            b += 1;
        }
        return Xoroshiro128Plus { s0: ns0, s1: ns1 };
    }
}

pub struct NextU64Result {
    pub value: u64,
    pub next: Xoroshiro128Plus,
}

pub struct NextI32Result {
    pub value: i32,
    pub next: Xoroshiro128Plus,
}

pub struct NextIntRangeResult {
    pub value: i64,
    pub next: Xoroshiro128Plus,
}

/// Unbiased uniform integer sampling in inclusive `[min, max]` via rejection sampling.
pub fn uniformIntDistribution(min: i64, max: i64, rng: Xoroshiro128Plus): NextIntRangeResult {
    if (min >= max) {
        return NextIntRangeResult { value: min, next: rng };
    }
    const span: u64 = ((max - min) as u64) +% 1;
    if (span == 0) {
        const r = rng.nextU64();
        return NextIntRangeResult { value: @as(i64, @bitCast(r.value)), next: r.next };
    }
    const max_u64: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
    const limit: u64 = max_u64 - ((max_u64 % span) +% 1) % span;
    let cur = rng;
    let guard: usize = 0;
    while (guard < 64) {
        const r = cur.nextU64();
        cur = r.next;
        if (r.value <= limit) {
            const off = r.value % span;
            return NextIntRangeResult { value: min + (off as i64), next: cur };
        }
        guard += 1;
    }
    const fallback = cur.nextU64();
    return NextIntRangeResult { value: min + ((fallback.value % span) as i64), next: fallback.next };
}

export {
    Xoroshiro128Plus,
    NextU64Result,
    NextI32Result,
    NextIntRangeResult,
    uniformIntDistribution,
};
