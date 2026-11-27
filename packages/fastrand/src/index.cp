// @cap/fastrand — Zero-allocation WyRand 64-bit PRNG for CP
// Clean-room MIT implementation inspired by Rust `fastrand` (WyRand algorithm).

const WYRAND_INC: u64 = ((0xa0761d64 as u64) << 32) | (0x78bd642f as u64);
const WYRAND_MIX: u64 = ((0xe7037ed1 as u64) << 32) | (0xa0b428db as u64);
const U64_MAX: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
const I64_SIGN_BIT: u64 = (1 as u64) << 63;
const ALPHANUM: string = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

// ---------------------------------------------------------------------------
// Overflow-Safe 64-bit / 128-bit Arithmetic Helpers
// ---------------------------------------------------------------------------

fn addWrap64(a: u64, b: u64) -> u64 {
    const space = U64_MAX - b;
    if (a > space) {
        return (a - space) - 1;
    }
    return a + b;
}

/// Exact 64x64 -> 128-bit multiplication returning `lo ^ hi` (WyRand core mixer).
/// Implemented with 32-bit limbs so it never overflows `u64` in debug or release builds.
fn mulWide64Xor(a: u64, b: u64) -> u64 {
    const a_lo = a & 0xFFFFFFFF;
    const a_hi = a >> 32;
    const b_lo = b & 0xFFFFFFFF;
    const b_hi = b >> 32;

    const p0 = a_lo * b_lo;
    const p1 = a_lo * b_hi;
    const p2 = a_hi * b_lo;
    const p3 = a_hi * b_hi;

    const mid = (p0 >> 32) + (p1 & 0xFFFFFFFF) + (p2 & 0xFFFFFFFF);
    const lo = (p0 & 0xFFFFFFFF) | ((mid & 0xFFFFFFFF) << 32);
    const hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
    return lo ^ hi;
}

fn i64ToOrderedU64(v: i64) -> u64 {
    if (v >= 0) {
        return (v as u64) + I64_SIGN_BIT;
    }
    const mag = ((0 - (v + 1)) as u64) + 1;
    return I64_SIGN_BIT - mag;
}

fn orderedU64ToI64(u: u64) -> i64 {
    if (u >= I64_SIGN_BIT) {
        return (u - I64_SIGN_BIT) as i64;
    }
    const mag = I64_SIGN_BIT - u;
    return -((mag - 1) as i64) - 1;
}

// ---------------------------------------------------------------------------
// Public WyRand PRNG Struct
// ---------------------------------------------------------------------------

pub struct Rng {
    pub state: u64,

    /// Creates a deterministic `Rng` seeded with `seed`.
    pub static fn withSeed(seed: u64) -> Rng {
        return Rng{ state: seed };
    }

    /// Generates the next uniform 64-bit unsigned integer via WyRand.
    pub fn nextU64(*_) -> u64 {
        _.state = addWrap64(_.state, WYRAND_INC);
        return mulWide64Xor(_.state, _.state ^ WYRAND_MIX);
    }

    /// Generates the next uniform 32-bit unsigned integer.
    pub fn nextU32(*_) -> u32 {
        return (_.nextU64() >> 32) as u32;
    }

    /// Generates a uniform boolean (`true` / `false`).
    pub fn nextBool(*_) -> bool {
        return (_.nextU64() & 1) != 0;
    }

    /// Derives an independent child `Rng` stream from the current generator.
    pub fn fork(*_) -> Rng {
        return Rng.withSeed(_.nextU64());
    }

    /// Generates an unbiased `u64` in `[0, bound)` via rejection sampling.
    pub fn u64Below(*_, bound: u64) -> u64 {
        if (bound <= 1) {
            return 0;
        }
        // threshold = 2^64 % bound without overflow
        const threshold = (((U64_MAX - bound) % bound) + 1) % bound;
        while (true) {
            const r = _.nextU64();
            if (r >= threshold) {
                return r % bound;
            }
        }
        return 0;
    }

    /// Returns `true` with probability `numerator / denominator`.
    pub fn nextChance(*_, numerator: u32, denominator: u32) -> bool {
        if (denominator == 0 || numerator == 0) {
            return false;
        }
        if (numerator >= denominator) {
            return true;
        }
        return _.u64Below(denominator as u64) < (numerator as u64);
    }

    /// Generates an unbiased `u64` in `[lo, hi_exclusive)`.
    pub fn u64Range(*_, lo: u64, hi_exclusive: u64) -> u64 {
        if (hi_exclusive <= lo) {
            return lo;
        }
        return lo + _.u64Below(hi_exclusive - lo);
    }

    /// Generates an unbiased `i64` in `[lo, hi_inclusive]` (overflow-safe across full `i64` range).
    pub fn i64Range(*_, lo: i64, hi_inclusive: i64) -> i64 {
        if (hi_inclusive <= lo) {
            return lo;
        }
        const lo_u = i64ToOrderedU64(lo);
        const hi_u = i64ToOrderedU64(hi_inclusive);
        const diff = hi_u - lo_u;
        if (diff == U64_MAX) {
            return orderedU64ToI64(_.nextU64());
        }
        const offset = _.u64Below(diff + 1);
        return orderedU64ToI64(lo_u + offset);
    }

    /// Generates an unbiased `usize` in `[lo, hi_exclusive)`.
    pub fn usizeRange(*_, lo: usize, hi_exclusive: usize) -> usize {
        if (hi_exclusive <= lo) {
            return lo;
        }
        return _.u64Range(lo as u64, hi_exclusive as u64) as usize;
    }

    /// Generates a uniform 53-bit precision `f64` strictly in `[0.0, 1.0)`.
    pub fn f64Unit(*_) -> f64 {
        const bits = _.nextU64() >> 11;
        return (bits as f64) * (1.0 / 9007199254740992.0);
    }

    /// Fills `buf` in-place with pseudorandom bytes.
    pub fn fillBytes(*_, buf: []u8) {
        let i: usize = 0;
        while (i + 8 <= buf.len) {
            const v = _.nextU64();
            buf[i] = (v & 0xFF) as u8;
            buf[i + 1] = ((v >> 8) & 0xFF) as u8;
            buf[i + 2] = ((v >> 16) & 0xFF) as u8;
            buf[i + 3] = ((v >> 24) & 0xFF) as u8;
            buf[i + 4] = ((v >> 32) & 0xFF) as u8;
            buf[i + 5] = ((v >> 40) & 0xFF) as u8;
            buf[i + 6] = ((v >> 48) & 0xFF) as u8;
            buf[i + 7] = ((v >> 56) & 0xFF) as u8;
            i += 8;
        }
        if (i < buf.len) {
            let v = _.nextU64();
            while (i < buf.len) {
                buf[i] = (v & 0xFF) as u8;
                v >>= 8;
                i += 1;
            }
        }
    }

    /// Shuffles `slice` in-place using the unbiased Fisher-Yates algorithm.
    pub fn shuffleU64(*_, slice: []u64) {
        if (slice.len <= 1) {
            return;
        }
        let i = slice.len - 1;
        while (i > 0) {
            const j = _.usizeRange(0 as usize, i + 1);
            const tmp = slice[i];
            slice[i] = slice[j];
            slice[j] = tmp;
            i -= 1;
        }
    }

    /// Fills `out` with random ASCII alphanumeric characters (`0-9A-Za-z`) and returns it as `string`.
    pub fn alphanumeric(*_, out: []u8) -> string {
        let i: usize = 0;
        while (i < out.len) {
            const idx = _.u64Below(ALPHANUM.len as u64) as usize;
            out[i] = ALPHANUM[idx];
            i += 1;
        }
        return out[0..out.len];
    }
}
