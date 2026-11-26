// @cap/half — Zero-heap IEEE 754 binary16 (f16) & Google Brain bfloat16 (bf16) Codecs
// Clean-room MIT implementation in pure CP (inspired by Rust `half` [599M downloads]).

// ============================================================================
// IEEE 754 binary16 (f16): 1 sign bit, 5 exponent bits (bias 15), 10 mantissa bits
// ============================================================================

pub struct F16 {
    pub bits: u16,

    pub static fn fromBits(bits: u16) -> F16 {
        return F16 { bits: bits };
    }

    pub fn isNan() -> bool {
        return (_.bits & 0x7FFF) > 0x7C00;
    }

    pub fn isInfinite() -> bool {
        return (_.bits & 0x7FFF) == 0x7C00;
    }

    pub fn isZero() -> bool {
        return (_.bits & 0x7FFF) == 0;
    }

    pub fn isSignNegative() -> bool {
        return (_.bits & 0x8000) != 0;
    }

    pub fn isFinite() -> bool {
        return (_.bits & 0x7C00) != 0x7C00;
    }

    pub fn abs() -> F16 {
        return F16 { bits: _.bits & 0x7FFF };
    }

    pub fn neg() -> F16 {
        return F16 { bits: _.bits ^ 0x8000 };
    }
}

/// Convert IEEE 754 f32 bit pattern (u32) to f16 bit pattern (u16) with round-to-nearest-even.
pub fn f32BitsToF16Bits(f32_bits: u32) -> u16 {
    const sign: u32 = (f32_bits >> 16) & 0x8000;
    const exp: u32 = (f32_bits >> 23) & 0xFF;
    const frac: u32 = f32_bits & 0x007FFFFF;

    // NaN or Infinity
    if (exp == 0xFF) {
        if (frac == 0) {
            return (sign | 0x7C00) as u16;
        }
        // Preserve quiet NaN payload
        const nanPayload: u32 = (frac >> 13) | 0x0200;
        return (sign | 0x7C00 | nanPayload) as u16;
    }

    // Rebias exponent from 127 to 15
    const unbiased: i32 = (exp as i32) - 127;

    // Overflow -> Infinity
    if (unbiased > 15) {
        return (sign | 0x7C00) as u16;
    }

    // Underflow -> subnormal or zero
    if (unbiased < -14) {
        if (unbiased < -25) {
            return sign as u16;
        }
        // Subnormal f16
        const mant: u32 = frac | 0x00800000;
        const shift: u32 = ((-14 - unbiased) + 13) as u32;
        const halfBit: u32 = (1 as u32) << (shift - 1);
        const mask: u32 = ((1 as u32) << shift) - 1;
        const rem: u32 = mant & mask;
        let q: u32 = mant >> shift;
        if (rem > halfBit || (rem == halfBit && (q & 1) != 0)) {
            q += 1;
        }
        return (sign | q) as u16;
    }

    // Normal f16 with round-to-nearest-even
    const hExp: u32 = (unbiased + 15) as u32;
    const roundBit: u32 = frac & 0x1FFF;
    let hMant: u32 = frac >> 13;
    if (roundBit > 0x1000 || (roundBit == 0x1000 && (hMant & 1) != 0)) {
        hMant += 1;
        if (hMant >= 0x0400) {
            hMant = 0;
            if (hExp + 1 >= 31) {
                return (sign | 0x7C00) as u16;
            }
            return (sign | ((hExp + 1) << 10)) as u16;
        }
    }
    return (sign | (hExp << 10) | hMant) as u16;
}

/// Convert IEEE 754 f16 bit pattern (u16) to exact f32 bit pattern (u32).
pub fn f16BitsToF32Bits(h_bits: u16) -> u32 {
    const sign: u32 = ((h_bits & 0x8000) as u32) << 16;
    const exp: u32 = ((h_bits >> 10) & 0x1F) as u32;
    const frac: u32 = (h_bits & 0x03FF) as u32;

    if (exp == 0) {
        if (frac == 0) {
            return sign;
        }
        // Normalize subnormal f16 into normal f32
        let m: u32 = frac;
        let e: i32 = -14;
        while ((m & 0x0400) == 0) {
            m = m << 1;
            e -= 1;
        }
        m = m & 0x03FF;
        const f32Exp: u32 = (e + 127) as u32;
        return sign | (f32Exp << 23) | (m << 13);
    }

    if (exp == 31) {
        if (frac == 0) {
            return sign | 0x7F800000;
        }
        return sign | 0x7FC00000 | (frac << 13);
    }

    const f32Exp: u32 = (exp + 112) as u32;
    return sign | (f32Exp << 23) | (frac << 13);
}

// ============================================================================
// Google Brain bfloat16 (bf16): 1 sign bit, 8 exponent bits (bias 127), 7 mantissa bits
// ============================================================================

pub struct Bf16 {
    pub bits: u16,

    pub static fn fromBits(bits: u16) -> Bf16 {
        return Bf16 { bits: bits };
    }

    pub fn isNan() -> bool {
        return (_.bits & 0x7FFF) > 0x7F80;
    }

    pub fn isInfinite() -> bool {
        return (_.bits & 0x7FFF) == 0x7F80;
    }

    pub fn isZero() -> bool {
        return (_.bits & 0x7FFF) == 0;
    }

    pub fn isSignNegative() -> bool {
        return (_.bits & 0x8000) != 0;
    }

    pub fn abs() -> Bf16 {
        return Bf16 { bits: _.bits & 0x7FFF };
    }

    pub fn neg() -> Bf16 {
        return Bf16 { bits: _.bits ^ 0x8000 };
    }
}

/// Convert IEEE 754 f32 bit pattern (u32) to bf16 bit pattern (u16) with round-to-nearest-even.
pub fn f32BitsToBf16Bits(f32_bits: u32) -> u16 {
    // NaN: preserve quiet NaN
    if ((f32_bits & 0x7FFFFFFF) > 0x7F800000) {
        return (((f32_bits >> 16) as u16) | 0x0040);
    }
    const lsb: u32 = (f32_bits >> 16) & 1;
    const roundingBias: u32 = (0x7FFF as u32) + lsb;
    return ((f32_bits + roundingBias) >> 16) as u16;
}

/// Convert bf16 bit pattern (u16) to exact f32 bit pattern (u32).
pub fn bf16BitsToF32Bits(b_bits: u16) -> u32 {
    return (b_bits as u32) << 16;
}
