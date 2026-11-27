// @cap/half-f16 — Zero-heap IEEE 754 binary16 (f16) & Google Brain Float 16 (bf16) in pure CP
// MIT License — Clean-room implementation inspired by Rust `half`

/// Reinterpret an `f32` value as its raw IEEE 754 `u32` bit pattern.
pub fn f32ToBits(x: f32): u32 {
    const b: u32 = @bitCast(x);
    return b;
}

/// Reinterpret a raw IEEE 754 `u32` bit pattern as `f32`.
pub fn bitsToF32(b: u32): f32 {
    const x: f32 = @bitCast(b);
    return x;
}

/// IEEE 754 binary16 half-precision float (1-bit sign, 5-bit exp bias 15, 10-bit mantissa).
pub struct F16 {
    pub bits: u16,

    /// Construct from raw `u16` IEEE 754 binary16 bit representation.
    pub static fn fromBits(b: u16): F16 {
        return F16 { bits: b };
    }

    /// Decode from two little-endian bytes `(b0, b1)`.
    pub static fn fromLeBytes(b0: u8, b1: u8): F16 {
        return F16 { bits: (b0 as u16) | ((b1 as u16) << 8) };
    }

    /// Convert `f32` to IEEE 754 `F16` using round-to-nearest-even (RTNE).
    /// Handles subnormals, overflow to ±Inf, NaN payload preservation, and ±0 sign preservation.
    pub static fn fromF32(x: f32): F16 {
        const bits: u32 = f32ToBits(x);
        const sign: u16 = ((bits >> 16) & 0x8000) as u16;
        const exp: u32 = (bits >> 23) & 0xFF;
        const mant: u32 = bits & 0x7FFFFF;

        // Inf or NaN
        if (exp == 0xFF) {
            if (mant == 0) {
                return F16 { bits: sign | 0x7C00 };
            }
            const payload: u16 = (mant >> 13) as u16;
            return F16 { bits: sign | 0x7E00 | payload };
        }

        // f32 zero or f32 subnormal -> signed f16 zero
        if (exp == 0) {
            return F16 { bits: sign };
        }

        const h_exp: i32 = (exp as i32) - 112; // (exp - 127) + 15

        // Overflow to ±Inf
        if (h_exp >= 31) {
            return F16 { bits: sign | 0x7C00 };
        }

        // f16 subnormal or underflow
        if (h_exp <= 0) {
            if (h_exp < -10) {
                return F16 { bits: sign };
            }
            const full_mant: u32 = mant | 0x800000;
            const shift: u5 = (14 - h_exp) as u5;
            const half_shift: u5 = shift - 1;
            const half_bit: u32 = (1 as u32) << half_shift;
            const rem_mask: u32 = ((1 as u32) << shift) - 1;
            let rounded: u32 = full_mant >> shift;
            const rem: u32 = full_mant & rem_mask;
            if (rem > half_bit || (rem == half_bit && (rounded & 1) != 0)) {
                rounded += 1;
            }
            return F16 { bits: sign | (rounded as u16) };
        }

        // Normal f16 with round-to-nearest-even
        const rem: u32 = mant & 0x1FFF;
        let h_mant: u32 = mant >> 13;
        if (rem > 0x1000 || (rem == 0x1000 && (h_mant & 1) != 0)) {
            h_mant += 1;
        }
        let final_exp: u32 = h_exp as u32;
        if (h_mant == 0x0400) {
            h_mant = 0;
            final_exp += 1;
        }
        if (final_exp >= 31) {
            return F16 { bits: sign | 0x7C00 };
        }
        return F16 {
            bits: sign | ((final_exp << 10) as u16) | (h_mant as u16),
        };
    }

    /// Convert `F16` losslessly to IEEE 754 `f32`.
    pub fn toF32(): f32 {
        const sign: u32 = ((_.bits & 0x8000) as u32) << 16;
        const exp: u32 = ((_.bits >> 10) & 0x1F) as u32;
        const mant: u32 = (_.bits & 0x03FF) as u32;

        if (exp == 0) {
            if (mant == 0) {
                return bitsToF32(sign);
            }
            // Subnormal f16 -> normalized f32
            let m: u32 = mant;
            let e: u32 = 113; // 127 - 14
            while ((m & 0x0400) == 0) {
                m = m << 1;
                e -= 1;
            }
            m = m & 0x03FF;
            return bitsToF32(sign | (e << 23) | (m << 13));
        }

        if (exp == 31) {
            if (mant == 0) {
                return bitsToF32(sign | 0x7F800000);
            }
            return bitsToF32(sign | 0x7FC00000 | (mant << 13));
        }

        const f_exp: u32 = exp + 112; // exp - 15 + 127
        return bitsToF32(sign | (f_exp << 23) | (mant << 13));
    }

    /// Return true if NaN (exp == 31 and mantissa != 0).
    pub fn isNan(): bool {
        return (_.bits & 0x7FFF) > 0x7C00;
    }

    /// Return true if +Inf or -Inf.
    pub fn isInfinite(): bool {
        return (_.bits & 0x7FFF) == 0x7C00;
    }

    /// Return true if +0.0 or -0.0.
    pub fn isZero(): bool {
        return (_.bits & 0x7FFF) == 0;
    }

    /// Negate by flipping the sign bit.
    pub fn neg(): F16 {
        return F16 { bits: _.bits ^ 0x8000 };
    }

    /// Absolute value by clearing the sign bit.
    pub fn abs(): F16 {
        return F16 { bits: _.bits & 0x7FFF };
    }

    /// Encode as two little-endian bytes `[low, high]`.
    pub fn toLeBytes(): [2]u8 {
        let out: [2]u8 = undefined;
        out[0] = (_.bits & 0xFF) as u8;
        out[1] = ((_.bits >> 8) & 0xFF) as u8;
        return out;
    }
}

/// Google Brain Float 16 (1-bit sign, 8-bit exp bias 127, 7-bit mantissa).
pub struct Bf16 {
    pub bits: u16,

    /// Construct from raw `u16` bfloat16 bit representation.
    pub static fn fromBits(b: u16): Bf16 {
        return Bf16 { bits: b };
    }

    /// Convert `f32` to `Bf16` using round-to-nearest-even on the dropped low 16 bits.
    pub static fn fromF32(x: f32): Bf16 {
        const bits: u32 = f32ToBits(x);
        if ((bits & 0x7FFFFFFF) > 0x7F800000) {
            // Preserve NaN as quiet NaN
            return Bf16 { bits: ((bits >> 16) as u16) | 0x0040 };
        }
        const lsb: u32 = (bits >> 16) & 1;
        const rounding_bias: u32 = lsb + 0x7FFF;
        const rounded: u32 = bits +% rounding_bias;
        return Bf16 { bits: (rounded >> 16) as u16 };
    }

    /// Convert `Bf16` losslessly to `f32` by zero-extending the low 16 bits.
    pub fn toF32(): f32 {
        return bitsToF32((_.bits as u32) << 16);
    }

    /// Return true if NaN.
    pub fn isNan(): bool {
        return (_.bits & 0x7FFF) > 0x7F80;
    }

    /// Return true if +Inf or -Inf.
    pub fn isInfinite(): bool {
        return (_.bits & 0x7FFF) == 0x7F80;
    }

    /// Negate by flipping the sign bit.
    pub fn neg(): Bf16 {
        return Bf16 { bits: _.bits ^ 0x8000 };
    }

    /// Absolute value by clearing the sign bit.
    pub fn abs(): Bf16 {
        return Bf16 { bits: _.bits & 0x7FFF };
    }
}

/// Batch convert `f32` slice into raw `u16` IEEE 754 f16 bits. Returns number of elements converted.
pub fn f32SliceToF16(src: []const f32, dst: []u16): usize {
    const n: usize = if (src.len < dst.len) src.len else dst.len;
    let i: usize = 0;
    while (i < n) {
        dst[i] = F16.fromF32(src[i]).bits;
        i += 1;
    }
    return n;
}

/// Batch convert raw `u16` IEEE 754 f16 bits into `f32` slice. Returns number of elements converted.
pub fn f16SliceToF32(src: []const u16, dst: []f32): usize {
    const n: usize = if (src.len < dst.len) src.len else dst.len;
    let i: usize = 0;
    while (i < n) {
        const h: F16 = F16.fromBits(src[i]);
        dst[i] = h.toF32();
        i += 1;
    }
    return n;
}
