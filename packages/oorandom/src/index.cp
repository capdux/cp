// @cap/oorandom — Zero-heap PCG32 (XSH RR 64/32) & dual-PCG32 Rand64 PRNG (crates.io:oorandom)

const OOR_MULTIPLIER: u64 = 6364136223846793005;
const OOR_DEFAULT_INC: u64 = 1442695040888963407;
const OOR_SECOND_INC: u64 = 1575024926850110891;
const OOR_HI_MIX: u64 = 0x517C_C1B7_2722_0A95;

fn oorStep(st: u64, inc: u64): u64 {
    return (st *% OOR_MULTIPLIER) +% inc;
}

fn oorOutput(oldstate: u64): u32 {
    const xorshifted: u32 = (((oldstate >> 18) ^ oldstate) >> 27) as u32;
    const rot: u5 = @as(u5, @truncate(oldstate >> 59));
    return std.math.rotr(u32, xorshifted, rot);
}

/// PCG32 (XSH RR 64/32) pseudorandom number generator.
pub struct Rand32 {
    st: u64,
    inc: u64,

    static fn newInc(seed: u64, inc: u64): Rand32 {
        const odd_inc: u64 = (inc << 1) | 1;
        let rng = Rand32{
            st: 0,
            inc: odd_inc,
        };
        * = rng.randU32();
        rng.st = rng.st +% seed;
        * = rng.randU32();
        return rng;
    }

    static fn new(seed: u64): Rand32 {
        return Rand32.newInc(seed, OOR_DEFAULT_INC);
    }

    fn randU32(*_): u32 {
        const oldstate = _.st;
        _.st = oorStep(oldstate, _.inc);
        return oorOutput(oldstate);
    }

    fn randI32(*_): i32 {
        return @as(i32, @bitCast(_.randU32()));
    }

    /// Uniform `f32` in `[0.0, 1.0)`.
    fn randFloat(*_): f32 {
        const bits: u32 = _.randU32() >> 8;
        return @as(f32, @floatFromInt(bits)) * (1.0 / 16777216.0);
    }

    /// Unbiased uniform `u32` in `[start, end)`. Returns `start` if `start >= end`.
    fn randRange(*_, start: u32, end: u32): u32 {
        if (start >= end) {
            return start;
        }
        const bound: u32 = end - start;
        const threshold: u32 = ((~bound) +% 1) % bound;
        while (true) {
            const r = _.randU32();
            if (r >= threshold) {
                return start + (r % bound);
            }
        }
        return start;
    }

    fn state(): u64 {
        return _.st;
    }
}

/// 64-bit PRNG backed by two independent PCG32 streams.
pub struct Rand64 {
    lo: Rand32,
    hi: Rand32,

    static fn new(seed: u64): Rand64 {
        const lo_rng = Rand32.newInc(seed, OOR_DEFAULT_INC);
        const hi_rng = Rand32.newInc(seed ^ OOR_HI_MIX, OOR_SECOND_INC);
        return Rand64{
            lo: lo_rng,
            hi: hi_rng,
        };
    }

    fn randU64(*_): u64 {
        const lo_v: u64 = _.lo.randU32() as u64;
        const hi_v: u64 = _.hi.randU32() as u64;
        return (hi_v << 32) | lo_v;
    }

    /// Uniform `f64` in `[0.0, 1.0)`.
    fn randFloat(*_): f64 {
        const bits: u64 = _.randU64() >> 11;
        return @as(f64, @floatFromInt(bits)) * (1.0 / 9007199254740992.0);
    }
}
