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

fn oorAdvanceState(st_in: u64, inc: u64, delta_in: u64): u64 {
    let cur_mult: u64 = OOR_MULTIPLIER;
    let cur_plus: u64 = inc;
    let acc_mult: u64 = 1;
    let acc_plus: u64 = 0;
    let delta: u64 = delta_in;

    while (delta > 0) {
        if ((delta & 1) != 0) {
            acc_mult = acc_mult *% cur_mult;
            acc_plus = (acc_plus *% cur_mult) +% cur_plus;
        }
        cur_plus = (cur_mult +% 1) *% cur_plus;
        cur_mult = cur_mult *% cur_mult;
        delta = delta >> 1;
    }
    return (acc_mult *% st_in) +% acc_plus;
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

    /// Restores a `Rand32` directly from a previously captured `(state, inc)` pair.
    static fn fromState(st: u64, inc: u64): Rand32 {
        return Rand32 {
            st: st,
            inc: inc | 1,
        };
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

    /// Uniform boolean.
    fn randBool(*_): bool {
        return (_.randU32() & 1) != 0;
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

    /// Unbiased uniform `i32` in `[start, end)`. Returns `start` if `start >= end`.
    fn randRangeI32(*_, start: i32, end: i32): i32 {
        if (start >= end) {
            return start;
        }
        const ustart: u32 = @as(u32, @bitCast(start));
        const uend: u32 = @as(u32, @bitCast(end));
        const bound: u32 = uend -% ustart;
        const threshold: u32 = ((~bound) +% 1) % bound;
        while (true) {
            const r = _.randU32();
            if (r >= threshold) {
                return @as(i32, @bitCast(ustart +% (r % bound)));
            }
        }
        return start;
    }

    /// Fills `dst` with pseudorandom bytes.
    fn fillBytes(*_, dst: []u8) {
        let i: usize = 0;
        while (i < dst.len) {
            let v: u32 = _.randU32();
            let k: usize = 0;
            while (k < 4 && i < dst.len) {
                dst[i] = (v & 0xFF) as u8;
                v = v >> 8;
                i += 1;
                k += 1;
            }
        }
    }

    /// Jumps the generator forward by `delta` steps in O(log(delta)) time.
    fn advance(*_, delta: u64) {
        _.st = oorAdvanceState(_.st, _.inc, delta);
    }

    fn state(): u64 {
        return _.st;
    }

    /// Returns both internal state words `[state, inc]` for serialization.
    fn statePair(): [2]u64 {
        let out: [2]u64 = [_.st, _.inc];
        return out;
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

    /// Creates a `Rand64` with explicit seeds and increments for both 32-bit streams.
    static fn newInc(seed_lo: u64, seed_hi: u64, inc_lo: u64, inc_hi: u64): Rand64 {
        return Rand64 {
            lo: Rand32.newInc(seed_lo, inc_lo),
            hi: Rand32.newInc(seed_hi, inc_hi),
        };
    }

    fn randU64(*_): u64 {
        const lo_v: u64 = _.lo.randU32() as u64;
        const hi_v: u64 = _.hi.randU32() as u64;
        return (hi_v << 32) | lo_v;
    }

    fn randI64(*_): i64 {
        return @as(i64, @bitCast(_.randU64()));
    }

    /// Uniform `f64` in `[0.0, 1.0)`.
    fn randFloat(*_): f64 {
        const bits: u64 = _.randU64() >> 11;
        return @as(f64, @floatFromInt(bits)) * (1.0 / 9007199254740992.0);
    }

    /// Uniform boolean.
    fn randBool(*_): bool {
        return (_.randU64() & 1) != 0;
    }

    /// Unbiased uniform `u64` in `[start, end)`. Returns `start` if `start >= end`.
    fn randRange(*_, start: u64, end: u64): u64 {
        if (start >= end) {
            return start;
        }
        const bound: u64 = end - start;
        const threshold: u64 = ((~bound) +% 1) % bound;
        while (true) {
            const r = _.randU64();
            if (r >= threshold) {
                return start + (r % bound);
            }
        }
        return start;
    }

    /// Unbiased uniform `i64` in `[start, end)`. Returns `start` if `start >= end`.
    fn randRangeI64(*_, start: i64, end: i64): i64 {
        if (start >= end) {
            return start;
        }
        const ustart: u64 = @as(u64, @bitCast(start));
        const uend: u64 = @as(u64, @bitCast(end));
        const bound: u64 = uend -% ustart;
        const threshold: u64 = ((~bound) +% 1) % bound;
        while (true) {
            const r = _.randU64();
            if (r >= threshold) {
                return @as(i64, @bitCast(ustart +% (r % bound)));
            }
        }
        return start;
    }

    /// Fills `dst` with pseudorandom bytes.
    fn fillBytes(*_, dst: []u8) {
        let i: usize = 0;
        while (i < dst.len) {
            let v: u64 = _.randU64();
            let k: usize = 0;
            while (k < 8 && i < dst.len) {
                dst[i] = (v & 0xFF) as u8;
                v = v >> 8;
                i += 1;
                k += 1;
            }
        }
    }

    /// Jumps both underlying PCG32 streams forward by `delta` steps in O(log(delta)) time.
    fn advance(*_, delta: u64) {
        _.lo.advance(delta);
        _.hi.advance(delta);
    }

    /// Returns the internal state words `[lo.state, hi.state]`.
    fn state(): [2]u64 {
        let out: [2]u64 = [_.lo.state(), _.hi.state()];
        return out;
    }
}
