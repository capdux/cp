// Origin: Pure-CP Stdlib (AI Low-Precision Floats: IEEE-754 FP16, BF16 & FP8 E4M3 Kernels) | Created: 2026-11-22
// Zero-heap-allocation bit-exact low-precision AI floating-point conversion & bulk decode/encode kernels:
//   - IEEE-754 Binary16 (FP16): full normal, subnormal, +-0, +-Inf, NaN & Round-to-Nearest-Even (RNE)
//   - Brain Float16 (BF16): exact expansion & unbiased RNE rounding
//   - OCP / NVIDIA FP8 E4M3FN (bias=7, max=448.0, 0x7F/0xFF=NaN, saturating encode, scaled bulk decode)

const std = @import("std");

/// Converts an IEEE-754 Binary16 (`u16`) bit pattern to `f32`.
/// Handles normals, subnormals, `+-0`, `+-Inf`, and quiet NaNs bit-accurately.
pub fn f16BitsToF32(bits: u16): f32 {
    const sign: u32 = ((bits & 0x8000) as u32) << 16;
    const exp: u32 = ((bits >> 10) & 0x1F) as u32;
    const mant: u32 = (bits & 0x03FF) as u32;

    if (exp == 0) {
        if (mant == 0) {
            return @bitCast(sign);
        }
        // Subnormal: value = (-1)^s * mant * 2^-24
        let m: u32 = mant;
        let f32_exp: u32 = 127 - 14;
        while ((m & 0x0400) == 0) {
            m <<= 1;
            f32_exp -= 1;
        }
        m &= 0x03FF;
        const out_bits: u32 = sign | (f32_exp << 23) | (m << 13);
        return @bitCast(out_bits);
    }

    if (exp == 31) {
        if (mant == 0) {
            const inf_bits: u32 = sign | 0x7F800000;
            return @bitCast(inf_bits);
        }
        const nan_bits: u32 = sign | 0x7FC00000 | (mant << 13);
        return @bitCast(nan_bits);
    }

    const f32_exp: u32 = exp + 112; // 127 - 15
    const out_bits: u32 = sign | (f32_exp << 23) | (mant << 13);
    return @bitCast(out_bits);
}

/// Converts an `f32` to an IEEE-754 Binary16 (`u16`) bit pattern using Round-to-Nearest-Even (RNE).
/// Preserves `+-0`, `+-Inf`, NaNs, and accurately rounds subnormal values.
pub fn f32ToF16Bits(val: f32): u16 {
    const f32_bits: u32 = @bitCast(val);
    const sign: u16 = ((f32_bits >> 16) & 0x8000) as u16;
    const exp: u32 = (f32_bits >> 23) & 0xFF;
    const mant: u32 = f32_bits & 0x007FFFFF;

    if (exp == 0xFF) {
        if (mant == 0) {
            return sign | 0x7C00; // +-Inf
        }
        const q_mant: u16 = ((mant >> 13) as u16) | 0x0200;
        return sign | 0x7C00 | q_mant; // quiet NaN
    }

    const unbiased: i32 = (exp as i32) - 127;
    if (unbiased > 15) {
        return sign | 0x7C00; // overflow -> +-Inf
    }

    if (unbiased >= -14) {
        let h_exp: u32 = (unbiased + 15) as u32;
        const lsb: u32 = (mant >> 13) & 1;
        const rounded: u32 = mant + 0x00000FFF + lsb;
        let h_mant: u32 = 0;
        if ((rounded & 0x00800000) != 0) {
            h_exp += 1;
            if (h_exp >= 31) {
                return sign | 0x7C00;
            }
        } else {
            h_mant = (rounded >> 13) & 0x03FF;
        }
        return sign | ((h_exp as u16) << 10) | (h_mant as u16);
    }

    if (unbiased >= -25) {
        const full_mant: u32 = (0x00800000 as u32) | mant;
        const shift_u32: u32 = (-1 - unbiased) as u32;
        const shift: u5 = @intCast(shift_u32); // range 14..24
        const low_mask: u32 = ((1 as u32) << shift) - 1;
        const half: u32 = (1 as u32) << (shift - 1);
        const rem: u32 = full_mant & low_mask;
        let q: u32 = full_mant >> shift;
        if (rem > half || (rem == half && (q & 1) != 0)) {
            q += 1;
        }
        if (q >= 0x0400) {
            return sign | 0x0400; // rounded up to smallest normal
        }
        return sign | (q as u16);
    }

    return sign; // underflow -> +-0
}

/// Bulk-decodes little-endian FP16 bytes into `dst` (`f32`).
/// Returns `false` if `src_bytes.len` is odd or `dst.len < src_bytes.len / 2`.
pub fn decodeF16SliceLe(src_bytes: []const u8, dst: *[]f32): bool {
    if ((src_bytes.len & 1) != 0) return false;
    const n = src_bytes.len / 2;
    if (dst.len < n) return false;

    let i: usize = 0;
    while (i < n) : (i += 1) {
        const b0 = src_bytes[i * 2] as u16;
        const b1 = src_bytes[i * 2 + 1] as u16;
        dst[i] = f16BitsToF32(b0 | (b1 << 8));
    }
    return true;
}

/// Bulk-encodes `f32` values into little-endian FP16 bytes in `dst_bytes`.
/// Returns `false` if `dst_bytes.len < src.len * 2`.
pub fn encodeF16SliceLe(src: []const f32, dst_bytes: *[]u8): bool {
    if (src.len > std.math.maxInt(usize) / 2) return false;
    if (dst_bytes.len < src.len * 2) return false;

    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        const bits = f32ToF16Bits(src[i]);
        dst_bytes[i * 2] = (bits & 0xFF) as u8;
        dst_bytes[i * 2 + 1] = ((bits >> 8) & 0xFF) as u8;
    }
    return true;
}

/// Converts a Brain Float16 (`u16`) bit pattern to `f32` by zero-extending the lower 16 bits.
pub fn bf16BitsToF32(bits: u16): f32 {
    const f32_bits: u32 = (bits as u32) << 16;
    return @bitCast(f32_bits);
}

/// Converts an `f32` to a Brain Float16 (`u16`) bit pattern using Round-to-Nearest-Even (RNE).
/// Preserves NaNs (setting quiet bit), `+-Inf`, and `+-0` without systematic truncation bias.
pub fn f32ToBf16Bits(val: f32): u16 {
    const f32_bits: u32 = @bitCast(val);
    if ((f32_bits & 0x7FFFFFFF) > 0x7F800000) {
        return ((f32_bits >> 16) as u16) | 0x0040; // quiet NaN
    }
    const lsb: u32 = (f32_bits >> 16) & 1;
    const rounded: u32 = f32_bits + 0x00007FFF + lsb;
    return (rounded >> 16) as u16;
}

/// Bulk-decodes little-endian BF16 bytes into `dst` (`f32`).
/// Returns `false` if `src_bytes.len` is odd or `dst.len < src_bytes.len / 2`.
pub fn decodeBf16SliceLe(src_bytes: []const u8, dst: *[]f32): bool {
    if ((src_bytes.len & 1) != 0) return false;
    const n = src_bytes.len / 2;
    if (dst.len < n) return false;

    let i: usize = 0;
    while (i < n) : (i += 1) {
        const b0 = src_bytes[i * 2] as u16;
        const b1 = src_bytes[i * 2 + 1] as u16;
        dst[i] = bf16BitsToF32(b0 | (b1 << 8));
    }
    return true;
}

/// Bulk-encodes `f32` values into little-endian BF16 bytes in `dst_bytes`.
/// Returns `false` if `dst_bytes.len < src.len * 2`.
pub fn encodeBf16SliceLe(src: []const f32, dst_bytes: *[]u8): bool {
    if (src.len > std.math.maxInt(usize) / 2) return false;
    if (dst_bytes.len < src.len * 2) return false;

    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        const bits = f32ToBf16Bits(src[i]);
        dst_bytes[i * 2] = (bits & 0xFF) as u8;
        dst_bytes[i * 2 + 1] = ((bits >> 8) & 0xFF) as u8;
    }
    return true;
}

/// Converts an OCP / NVIDIA FP8 E4M3FN byte (`u8`) to `f32`.
/// Exponent bias is 7, max finite magnitude is `448.0` (`0x7E`), and `0x7F`/`0xFF` decode to NaN (no Inf).
pub fn fp8E4m3ToF32(bits: u8): f32 {
    const sign: u32 = ((bits & 0x80) as u32) << 24;
    const exp: u32 = ((bits >> 3) & 0x0F) as u32;
    const mant: u32 = (bits & 0x07) as u32;

    if (exp == 15 && mant == 7) {
        const nan_bits: u32 = sign | 0x7FC00000;
        return @bitCast(nan_bits);
    }

    if (exp == 0) {
        if (mant == 0) {
            return @bitCast(sign);
        }
        // Subnormal: value = (-1)^s * mant * 2^-9
        let m: u32 = mant;
        let f32_exp: u32 = 127 - 6;
        while ((m & 0x08) == 0) {
            m <<= 1;
            f32_exp -= 1;
        }
        m &= 0x07;
        const out_bits: u32 = sign | (f32_exp << 23) | (m << 20);
        return @bitCast(out_bits);
    }

    const f32_exp: u32 = exp + 120; // 127 - 7
    const out_bits: u32 = sign | (f32_exp << 23) | (mant << 20);
    return @bitCast(out_bits);
}

/// Converts an `f32` to an OCP / NVIDIA FP8 E4M3FN byte (`u8`) with RNE rounding.
/// NaNs map to `0x7F`/`0xFF`; finite values exceeding `448.0` and `+-Inf` saturate to `+-448.0` (`0x7E`/`0xFE`).
pub fn f32ToFp8E4m3(val: f32): u8 {
    const f32_bits: u32 = @bitCast(val);
    const sign: u8 = ((f32_bits >> 24) & 0x80) as u8;
    const exp: u32 = (f32_bits >> 23) & 0xFF;
    const mant: u32 = f32_bits & 0x007FFFFF;

    if (exp == 0xFF) {
        if (mant != 0) {
            return sign | 0x7F; // NaN
        }
        return sign | 0x7E; // +-Inf saturates to +-448.0
    }

    const unbiased: i32 = (exp as i32) - 127;
    if (unbiased > 8) {
        return sign | 0x7E; // saturate to +-448.0
    }

    if (unbiased >= -6) {
        let e_out: u32 = (unbiased + 7) as u32;
        const lsb: u32 = (mant >> 20) & 1;
        const rounded: u32 = mant + 0x0007FFFF + lsb;
        let m_out: u32 = 0;
        if ((rounded & 0x00800000) != 0) {
            e_out += 1;
        } else {
            m_out = (rounded >> 20) & 0x07;
        }
        if (e_out > 15 || (e_out == 15 && m_out >= 7)) {
            return sign | 0x7E; // saturate to +-448.0
        }
        return sign | ((e_out as u8) << 3) | (m_out as u8);
    }

    if (unbiased >= -10) {
        const full_mant: u32 = (0x00800000 as u32) | mant;
        const shift_u32: u32 = (14 - unbiased) as u32;
        const shift: u5 = @intCast(shift_u32); // range 21..24
        const low_mask: u32 = ((1 as u32) << shift) - 1;
        const half: u32 = (1 as u32) << (shift - 1);
        const rem: u32 = full_mant & low_mask;
        let q: u32 = full_mant >> shift;
        if (rem > half || (rem == half && (q & 1) != 0)) {
            q += 1;
        }
        if (q >= 8) {
            return sign | 0x08; // rounded up to smallest normal
        }
        return sign | (q as u8);
    }

    return sign; // underflow -> +-0
}

/// Bulk-decodes FP8 E4M3FN bytes with a per-tensor dequantization `scale`:
/// `dst[i] = fp8E4m3ToF32(src_bytes[i]) * scale`.
/// Returns `false` if `scale` is NaN/Inf or `dst.len < src_bytes.len`.
pub fn decodeFp8E4m3Slice(src_bytes: []const u8, scale: f32, dst: *[]f32): bool {
    if (std.math.isNan(scale) || std.math.isInf(scale)) return false;
    if (dst.len < src_bytes.len) return false;

    let i: usize = 0;
    while (i < src_bytes.len) : (i += 1) {
        dst[i] = fp8E4m3ToF32(src_bytes[i]) * scale;
    }
    return true;
}
