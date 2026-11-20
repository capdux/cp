// Source: cp/lib/safetensors/float16.cp (IEEE-754 BF16 and FP16 bit-level conversions)

pub fn bf16ToF32(bits: u16): f32 {
    const shifted: u32 = @as(u32, bits) << 16;
    return @bitCast(shifted);
}

pub fn f32ToBf16(val: f32): u16 {
    const bits: u32 = @bitCast(val);
    // Preserve NaN as quiet NaN
    if ((bits & 0x7FFFFFFF) > 0x7F800000) {
        return @as(u16, @truncate(bits >> 16)) | 0x0040;
    }
    // Round-to-nearest-even
    const rounding_bias: u32 = @as(u32, 0x7FFF) + ((bits >> 16) & @as(u32, 1));
    return @truncate((bits + rounding_bias) >> 16);
}

pub fn f16ToF32(bits: u16): f32 {
    const sign: u32 = @as(u32, bits & 0x8000) << 16;
    const exp: u32 = (bits >> 10) & 0x1F;
    let mant: u32 = bits & 0x03FF;

    if (exp == 0) {
        if (mant == 0) {
            return @bitCast(sign);
        }
        // Subnormal binary16 -> normalized binary32
        let shift: u32 = 0;
        while ((mant & 0x0400) == 0) {
            mant <<= 1;
            shift += 1;
        }
        mant &= 0x03FF;
        const new_exp: u32 = @as(u32, 113) - shift;
        return @bitCast(sign | (new_exp << 23) | (mant << 13));
    }
    if (exp == 31) {
        if (mant == 0) {
            return @bitCast(sign | 0x7F800000);
        }
        return @bitCast(sign | 0x7FC00000 | (mant << 13));
    }
    const new_exp: u32 = exp + @as(u32, 112);
    return @bitCast(sign | (new_exp << 23) | (mant << 13));
}

pub fn f32ToF16(val: f32): u16 {
    const bits: u32 = @bitCast(val);
    const sign: u16 = @truncate((bits >> 16) & 0x8000);
    const exp_raw: i32 = @intCast((bits >> 23) & 0xFF);
    const mant: u32 = bits & 0x007FFFFF;

    if (exp_raw == 255) {
        if (mant == 0) {
            return sign | 0x7C00;
        }
        const nan_mant: u16 = @truncate(mant >> 13);
        return sign | 0x7E00 | nan_mant;
    }

    let exp: i32 = exp_raw - 112;
    if (exp >= 31) {
        return sign | 0x7C00;
    }
    if (exp <= 0) {
        if (exp < -10) {
            return sign;
        }
        const m: u32 = mant | 0x00800000;
        const shift: u5 = @intCast(14 - exp);
        const half: u32 = @as(u32, 1) << (shift - 1);
        const rem_mask: u32 = (@as(u32, 1) << shift) - 1;
        const rem: u32 = m & rem_mask;
        let res: u32 = m >> shift;
        if (rem > half or (rem == half and (res & 1) != 0)) {
            res += 1;
        }
        return sign | @as(u16, @truncate(res));
    }

    const round_bit: u32 = (mant >> 12) & 1;
    const rem_low: u32 = mant & 0x0FFF;
    let res_mant: u32 = mant >> 13;
    if (round_bit != 0 and (rem_low != 0 or (res_mant & 1) != 0)) {
        res_mant += 1;
    }
    if (res_mant >= 0x0400) {
        res_mant = 0;
        exp += 1;
        if (exp >= 31) {
            return sign | 0x7C00;
        }
    }
    return sign | @as(u16, @intCast(exp << 10)) | @as(u16, @truncate(res_mant));
}
