// @cap/fixed-point — Zero-allocation signed 64-bit Q32.32 deterministic fixed-point engine for CP
// Clean-room MIT implementation inspired by Rust `fixed` crate.
//
// Representation:
// - `raw: i64` represents `value * 2^32` (FRAC_BITS = 32, resolution ~2.328e-10).
// - Bit-identical across all CPUs/platforms with zero floating-point drift.
// - Overflow-free 128-bit split multiplication (`mul`) and 32-step binary long division (`div`, `fromRatio`).

pub const FRAC_BITS: u32 = 32;
const ONE_RAW: i64 = 4294967296; // 1 << 32
const HALF_RAW: u64 = 2147483648; // 1 << 31
const FRAC_MASK: u64 = 0xFFFFFFFF;
const SCALE_F64: f64 = 4294967296.0;

fn absU64(v: i64) -> u64 {
    if (v < 0) {
        return ((0 - (v + 1)) as u64) + 1;
    }
    return v as u64;
}

/// Computes `floor((a << 32) / b)` for unsigned 64-bit integers with zero intermediate overflow.
fn divShift32U64(a: u64, b: u64) -> u64 {
    const q_hi: u64 = a / b;
    let rem: u64 = a % b;
    let q_lo: u64 = 0;

    let step: u32 = 0;
    while (step < 32) {
        q_lo = q_lo << 1;
        // `rem >= b - rem` is mathematically equivalent to `2 * rem >= b` without overflowing u64
        const gap = b - rem;
        if (rem >= gap) {
            rem = rem - gap;
            q_lo = q_lo | 1;
        } else {
            rem = rem << 1;
        }
        step += 1;
    }

    return (q_hi << 32) | q_lo;
}

/// Signed 64-bit Q32.32 fixed-point number (`raw = value * 2^32`).
pub struct Fixed32 {
    pub raw: i64,

    pub static fn fromInt(v: i32) -> Fixed32 {
        return Fixed32{ raw: (v as i64) * ONE_RAW };
    }

    pub static fn fromRaw(raw: i64) -> Fixed32 {
        return Fixed32{ raw: raw };
    }

    /// Constructs Q32.32 from exact integer ratio `num / den`. Returns 0 if `den == 0`.
    pub static fn fromRatio(num: i64, den: i64) -> Fixed32 {
        if (den == 0 || num == 0) {
            return Fixed32{ raw: 0 };
        }
        const is_neg = (num < 0) != (den < 0);
        const mag = divShift32U64(absU64(num), absU64(den));
        if (is_neg && mag > 0) {
            return Fixed32{ raw: 0 - (mag as i64) };
        }
        return Fixed32{ raw: mag as i64 };
    }

    pub static fn fromF64(v: f64) -> Fixed32 {
        const scaled = v * SCALE_F64;
        if (scaled >= 0.0) {
            return Fixed32{ raw: (scaled + 0.5) as i64 };
        }
        return Fixed32{ raw: (scaled - 0.5) as i64 };
    }

    pub fn rawValue() -> i64 {
        return _.raw;
    }

    pub fn toF64() -> f64 {
        return (_.raw as f64) / SCALE_F64;
    }

    /// Truncates toward zero into an `i32`.
    pub fn toInt() -> i32 {
        if (_.raw >= 0) {
            return ((_.raw as u64) >> 32) as i32;
        }
        const mag: u64 = absU64(_.raw) >> 32;
        return 0 - (mag as i32);
    }

    /// Rounds toward negative infinity into an `i32`.
    pub fn floor() -> i32 {
        const q = _.toInt();
        if (_.raw < 0 && (absU64(_.raw) & FRAC_MASK) != 0) {
            return q - 1;
        }
        return q;
    }

    /// Rounds toward positive infinity into an `i32`.
    pub fn ceil() -> i32 {
        const q = _.toInt();
        if (_.raw > 0 && ((_.raw as u64) & FRAC_MASK) != 0) {
            return q + 1;
        }
        return q;
    }

    /// Rounds to nearest integer (half away from zero).
    pub fn round() -> i32 {
        if (_.raw >= 0) {
            const mag: u64 = (_.raw as u64) + HALF_RAW;
            return (mag >> 32) as i32;
        }
        const mag: u64 = absU64(_.raw) + HALF_RAW;
        return 0 - ((mag >> 32) as i32);
    }

    pub fn add(other: Fixed32) -> Fixed32 {
        return Fixed32{ raw: _.raw + other.raw };
    }

    pub fn sub(other: Fixed32) -> Fixed32 {
        return Fixed32{ raw: _.raw - other.raw };
    }

    pub fn neg() -> Fixed32 {
        return Fixed32{ raw: 0 - _.raw };
    }

    pub fn abs() -> Fixed32 {
        if (_.raw < 0) {
            return Fixed32{ raw: 0 - _.raw };
        }
        return Fixed32{ raw: _.raw };
    }

    /// Exact Q32.32 multiplication using 32-bit high/low splitting to avoid `i64` overflow.
    pub fn mul(other: Fixed32) -> Fixed32 {
        if (_.raw == 0 || other.raw == 0) {
            return Fixed32{ raw: 0 };
        }
        const is_neg = (_.raw < 0) != (other.raw < 0);
        const a = absU64(_.raw);
        const b = absU64(other.raw);

        const a_hi: u64 = a >> 32;
        const a_lo: u64 = a & FRAC_MASK;
        const b_hi: u64 = b >> 32;
        const b_lo: u64 = b & FRAC_MASK;

        const p_hh: u64 = a_hi * b_hi;
        const p_hl: u64 = a_hi * b_lo;
        const p_lh: u64 = a_lo * b_hi;
        const p_ll: u64 = a_lo * b_lo;

        const mag: u64 = (p_hh << 32) + p_hl + p_lh + (p_ll >> 32);
        if (is_neg && mag > 0) {
            return Fixed32{ raw: 0 - (mag as i64) };
        }
        return Fixed32{ raw: mag as i64 };
    }

    /// Exact Q32.32 division using overflow-free 128-bit long division. Returns 0 on division by zero.
    pub fn div(other: Fixed32) -> Fixed32 {
        if (other.raw == 0 || _.raw == 0) {
            return Fixed32{ raw: 0 };
        }
        const is_neg = (_.raw < 0) != (other.raw < 0);
        const mag = divShift32U64(absU64(_.raw), absU64(other.raw));
        if (is_neg && mag > 0) {
            return Fixed32{ raw: 0 - (mag as i64) };
        }
        return Fixed32{ raw: mag as i64 };
    }

    /// Linear interpolation `self + (other - self) * t`.
    pub fn lerp(other: Fixed32, t: Fixed32) -> Fixed32 {
        const diff = other.sub(Fixed32{ raw: _.raw });
        return Fixed32{ raw: _.raw }.add(diff.mul(t));
    }

    /// Clamps value into `[lo, hi]`.
    pub fn clamp(lo: Fixed32, hi: Fixed32) -> Fixed32 {
        if (_.raw < lo.raw) {
            return lo;
        }
        if (_.raw > hi.raw) {
            return hi;
        }
        return Fixed32{ raw: _.raw };
    }

    /// Deterministic Newton-Raphson square root in Q32.32. Returns 0 for non-positive inputs.
    pub fn sqrt() -> Fixed32 {
        if (_.raw <= 0) {
            return Fixed32{ raw: 0 };
        }

        // Choose a good power-of-2 initial guess based on MSB position of `raw`
        const u: u64 = _.raw as u64;
        let msb: u32 = 0;
        let tmp = u;
        while (tmp > 1) {
            tmp = tmp >> 1;
            msb += 1;
        }
        // sqrt(u * 2^32) ~ 2^((msb + 32) / 2)
        const shift: u32 = (msb + 32) / 2;
        let g: u64 = (1 as u64) << (shift as u6);
        if (g == 0) {
            g = 1;
        }

        let iter: u32 = 0;
        while (iter < 16) {
            const q = divShift32U64(u, g);
            // Overflow-free average: (g & q) + ((g ^ q) >> 1)
            const next_g: u64 = (g & q) + ((g ^ q) >> 1);
            if (next_g == g || (next_g > g && next_g - g == 1)) {
                g = next_g;
                break;
            }
            g = next_g;
            iter += 1;
        }

        return Fixed32{ raw: g as i64 };
    }

    /// Three-way comparison returning `-1`, `0`, or `1`.
    pub fn cmp(other: Fixed32) -> i32 {
        if (_.raw < other.raw) {
            return -1;
        }
        if (_.raw > other.raw) {
            return 1;
        }
        return 0;
    }
}
