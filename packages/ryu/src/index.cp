// @cap/ryu-float — Pure CP zero-allocation fast float-to-string formatter
// Clean-room MIT implementation (Rust `ryu` v1 parity).
// Reuses @cap/itoa (`formatU64`) for all decimal integer mantissa and exponent rendering.

import { formatU64 } from "../../itoa/src/index.cp";

struct RyuDecimal {
    sig: u64,
    exp10_msb: i32,
}

fn ryuPow10U64(n: u8): u64 {
    let v: u64 = 1;
    let i: u8 = 0;
    while (i < n) {
        v = v * 10;
        i = i + 1;
    }
    return v;
}

fn ryuScalePow10(x: f64, p_in: i32): f64 {
    if (p_in == 0) {
        return x;
    }
    let val = x;
    let p = p_in;
    while (p > 300) {
        val = val * 1.0e150;
        p = p - 150;
    }
    while (p < -300) {
        val = val * 1.0e-150;
        p = p + 150;
    }
    if (p > 0) {
        return val * std.math.pow(f64, 10.0, p as f64);
    }
    return val / std.math.pow(f64, 10.0, (-p) as f64);
}

fn ryuStripTrailingZeros(sig_in: u64): u64 {
    let s = sig_in;
    while (s > 0 && (s % 10) == 0) {
        s = s / 10;
    }
    return s;
}

fn ryuRoundSig(m17: u64, drop_digits: u8): u64 {
    if (drop_digits == 0) {
        return m17;
    }
    let div = ryuPow10U64(drop_digits);
    let q = m17 / div;
    let rem = m17 % div;
    let half = div / 2;
    if (rem > half || (rem == half && (q & 1) == 1)) {
        q = q + 1;
    }
    return q;
}

fn ryuExtractShortestF64(abs_val: f64): RyuDecimal {
    let d: i32 = @intFromFloat(std.math.floor(std.math.log10(abs_val)));
    let scaled = ryuScalePow10(abs_val, -d);
    while (scaled >= 10.0) {
        scaled = scaled / 10.0;
        d = d + 1;
    }
    while (scaled < 1.0 && scaled > 0.0) {
        scaled = scaled * 10.0;
        d = d - 1;
    }

    let raw17 = ryuScalePow10(abs_val, 16 - d);
    let m17: u64 = @intFromFloat(std.math.round(raw17));
    if (m17 >= 100000000000000000) {
        m17 = m17 / 10;
        d = d + 1;
    } else if (m17 < 10000000000000000) {
        m17 = m17 * 10;
        d = d - 1;
    }

    let k: u8 = 1;
    while (k <= 17) {
        let drop: u8 = (17 as u8) - k;
        let q = ryuRoundSig(m17, drop);
        let q_exp = d - ((k as i32) - 1);
        let cand = ryuScalePow10(q as f64, q_exp);
        if (cand == abs_val) {
            let eff_d = d;
            if (q >= ryuPow10U64(k)) {
                eff_d = eff_d + 1;
            }
            return RyuDecimal { sig: ryuStripTrailingZeros(q), exp10_msb: eff_d };
        }
        k = k + 1;
    }

    // Fallback: check m17 - 1 and m17 + 1 for 1-ULP boundary cases
    if (m17 > 0 && ryuScalePow10((m17 - 1) as f64, d - 16) == abs_val) {
        return RyuDecimal { sig: ryuStripTrailingZeros(m17 - 1), exp10_msb: d };
    }
    if (ryuScalePow10((m17 + 1) as f64, d - 16) == abs_val) {
        return RyuDecimal { sig: ryuStripTrailingZeros(m17 + 1), exp10_msb: d };
    }
    return RyuDecimal { sig: ryuStripTrailingZeros(m17), exp10_msb: d };
}

fn ryuExtractShortestF32(abs_f32: f32): RyuDecimal {
    let abs_val: f64 = abs_f32 as f64;
    let d: i32 = @intFromFloat(std.math.floor(std.math.log10(abs_val)));
    let scaled = ryuScalePow10(abs_val, -d);
    while (scaled >= 10.0) {
        scaled = scaled / 10.0;
        d = d + 1;
    }
    while (scaled < 1.0 && scaled > 0.0) {
        scaled = scaled * 10.0;
        d = d - 1;
    }

    let raw10 = ryuScalePow10(abs_val, 9 - d);
    let m10: u64 = @intFromFloat(std.math.round(raw10));
    if (m10 >= 10000000000) {
        m10 = m10 / 10;
        d = d + 1;
    } else if (m10 < 1000000000) {
        m10 = m10 * 10;
        d = d - 1;
    }

    let k: u8 = 1;
    while (k <= 10) {
        let drop: u8 = (10 as u8) - k;
        let q = ryuRoundSig(m10, drop);
        let q_exp = d - ((k as i32) - 1);
        let cand_f32: f32 = ryuScalePow10(q as f64, q_exp) as f32;
        if (cand_f32 == abs_f32) {
            let eff_d = d;
            if (q >= ryuPow10U64(k)) {
                eff_d = eff_d + 1;
            }
            return RyuDecimal { sig: ryuStripTrailingZeros(q), exp10_msb: eff_d };
        }
        k = k + 1;
    }
    return RyuDecimal { sig: ryuStripTrailingZeros(m10), exp10_msb: d };
}

fn ryuCopySlice(dst: []u8, pos: *usize, src: []const u8): bool {
    if (pos.* + src.len > dst.len) {
        return false;
    }
    let i: usize = 0;
    while (i < src.len) {
        dst[pos.* + i] = src[i];
        i = i + 1;
    }
    pos.* = pos.* + src.len;
    return true;
}

fn ryuPushByte(dst: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= dst.len) {
        return false;
    }
    dst[pos.*] = b;
    pos.* = pos.* + 1;
    return true;
}

fn ryuRenderDecimal(neg: bool, dec_val: RyuDecimal, out: []u8): usize {
    let tmp: [48]u8 = [0; 48];
    let pos: usize = 0;

    if (neg) {
        * = ryuPushByte(tmp[0..48], &pos, 0x2D); // '-'
    }

    let dbuf: [24]u8 = [0; 24];
    let nd = formatU64(dec_val.sig, dbuf[0..24]);
    let d = dec_val.exp10_msb;

    if (d >= 16 || d <= -5) {
        // Scientific notation: d0[.d1..dn]e[-]exp
        * = ryuPushByte(tmp[0..48], &pos, dbuf[0]);
        if (nd > 1) {
            * = ryuPushByte(tmp[0..48], &pos, 0x2E); // '.'
            * = ryuCopySlice(tmp[0..48], &pos, dbuf[1..nd]);
        }
        * = ryuPushByte(tmp[0..48], &pos, 0x65); // 'e'
        let abs_e: u64 = 0;
        if (d < 0) {
            * = ryuPushByte(tmp[0..48], &pos, 0x2D); // '-'
            abs_e = (-d) as u64;
        } else {
            abs_e = d as u64;
        }
        let ebuf: [16]u8 = [0; 16];
        let en = formatU64(abs_e, ebuf[0..16]);
        * = ryuCopySlice(tmp[0..48], &pos, ebuf[0..en]);
    } else if (d >= 0) {
        let int_digits: usize = (d + 1) as usize;
        if (nd <= int_digits) {
            * = ryuCopySlice(tmp[0..48], &pos, dbuf[0..nd]);
            let z: usize = 0;
            while (z < int_digits - nd) {
                * = ryuPushByte(tmp[0..48], &pos, 0x30); // '0'
                z = z + 1;
            }
            * = ryuCopySlice(tmp[0..48], &pos, ".0");
        } else {
            * = ryuCopySlice(tmp[0..48], &pos, dbuf[0..int_digits]);
            * = ryuPushByte(tmp[0..48], &pos, 0x2E); // '.'
            * = ryuCopySlice(tmp[0..48], &pos, dbuf[int_digits..nd]);
        }
    } else {
        // -4 <= d <= -1: "0." + (-d - 1) zeros + digits
        * = ryuCopySlice(tmp[0..48], &pos, "0.");
        let zeros: usize = ((-d) - 1) as usize;
        let z: usize = 0;
        while (z < zeros) {
            * = ryuPushByte(tmp[0..48], &pos, 0x30); // '0'
            z = z + 1;
        }
        * = ryuCopySlice(tmp[0..48], &pos, dbuf[0..nd]);
    }

    if (pos > out.len) {
        return 0;
    }
    let out_pos: usize = 0;
    * = ryuCopySlice(out, &out_pos, tmp[0..pos]);
    return pos;
}

/// Rounds an ASCII digit buffer `digits[0..nd]` to `keep` digits (half-up).
/// Writes `keep` digits into `rounded` and returns `true` if rounding carried past MSB (e.g. 999 -> 1000).
fn ryuRoundAsciiDigits(digits: []const u8, keep: usize, rounded: []u8): bool {
    let i: usize = 0;
    while (i < keep) {
        if (i < digits.len) {
            rounded[i] = digits[i];
        } else {
            rounded[i] = 0x30; // '0'
        }
        i = i + 1;
    }
    let round_up = false;
    if (keep < digits.len && digits[keep] >= 0x35) { // >= '5'
        round_up = true;
    }
    if (!round_up) {
        return false;
    }
    let k: usize = keep;
    while (k > 0) {
        k = k - 1;
        if (rounded[k] < 0x39) {
            rounded[k] = rounded[k] + 1;
            return false;
        }
        rounded[k] = 0x30;
    }
    return true;
}

/// Returns true if `val` is NaN.
pub fn isNanF64(val: f64): bool {
    return std.math.isNan(val);
}

/// Returns true if `val` is +inf or -inf.
pub fn isInfF64(val: f64): bool {
    return std.math.isInf(val);
}

/// Returns true if `val` is finite (neither NaN nor infinite).
pub fn isFiniteF64(val: f64): bool {
    return std.math.isFinite(val);
}

/// Formats `val: f64` using shortest exact decimal / scientific representation (Rust `ryu` parity).
/// Integers retain `.0` (e.g. `1.0`, `0.0`, `-0.0`). Special values emit `"NaN"`, `"inf"`, `"-inf"`.
/// Returns bytes written, or 0 if `out` is too small.
pub fn formatF64(val: f64, out: []u8): usize {
    if (std.math.isNan(val)) {
        if (out.len < 3) { return 0; }
        let p: usize = 0;
        * = ryuCopySlice(out, &p, "NaN");
        return p;
    }
    let neg = std.math.signbit(val);
    if (std.math.isInf(val)) {
        if (neg) {
            if (out.len < 4) { return 0; }
            let p: usize = 0;
            * = ryuCopySlice(out, &p, "-inf");
            return p;
        }
        if (out.len < 3) { return 0; }
        let p: usize = 0;
        * = ryuCopySlice(out, &p, "inf");
        return p;
    }
    if (val == 0.0) {
        if (neg) {
            if (out.len < 4) { return 0; }
            let p: usize = 0;
            * = ryuCopySlice(out, &p, "-0.0");
            return p;
        }
        if (out.len < 3) { return 0; }
        let p: usize = 0;
        * = ryuCopySlice(out, &p, "0.0");
        return p;
    }
    let abs_val = if (neg) { -val } else { val };
    let dec_val = ryuExtractShortestF64(abs_val);
    return ryuRenderDecimal(neg, dec_val, out);
}

/// Formats finite `val: f64`. Returns 0 for NaN, +inf, -inf, or undersized `out`.
pub fn formatF64Finite(val: f64, out: []u8): usize {
    if (!std.math.isFinite(val)) {
        return 0;
    }
    return formatF64(val, out);
}

/// Formats `val: f32` using shortest exact single-precision decimal / scientific representation.
pub fn formatF32(val: f32, out: []u8): usize {
    if (std.math.isNan(val)) {
        if (out.len < 3) { return 0; }
        let p: usize = 0;
        * = ryuCopySlice(out, &p, "NaN");
        return p;
    }
    let neg = std.math.signbit(val);
    if (std.math.isInf(val)) {
        if (neg) {
            if (out.len < 4) { return 0; }
            let p: usize = 0;
            * = ryuCopySlice(out, &p, "-inf");
            return p;
        }
        if (out.len < 3) { return 0; }
        let p: usize = 0;
        * = ryuCopySlice(out, &p, "inf");
        return p;
    }
    if (val == 0.0) {
        if (neg) {
            if (out.len < 4) { return 0; }
            let p: usize = 0;
            * = ryuCopySlice(out, &p, "-0.0");
            return p;
        }
        if (out.len < 3) { return 0; }
        let p: usize = 0;
        * = ryuCopySlice(out, &p, "0.0");
        return p;
    }
    let abs_f32: f32 = if (neg) { -val } else { val };
    let dec_val = ryuExtractShortestF32(abs_f32);
    return ryuRenderDecimal(neg, dec_val, out);
}

/// Formats `val: f64` in fixed-point notation with `decimals` (0..=12) fractional digits,
/// using exact decimal rounding (e.g. `formatFixedF64(9.995, 2)` -> `"10.00"`).
pub fn formatFixedF64(val: f64, decimals: u8, out: []u8): usize {
    if (!std.math.isFinite(val)) {
        return formatF64(val, out);
    }
    let dec_places: u8 = if (decimals > 12) { 12 } else { decimals };
    let neg = std.math.signbit(val);
    let tmp: [64]u8 = [0; 64];
    let pos: usize = 0;

    if (val == 0.0) {
        if (neg) {
            * = ryuPushByte(tmp[0..64], &pos, 0x2D);
        }
        * = ryuPushByte(tmp[0..64], &pos, 0x30);
        if (dec_places > 0) {
            * = ryuPushByte(tmp[0..64], &pos, 0x2E);
            let z: u8 = 0;
            while (z < dec_places) {
                * = ryuPushByte(tmp[0..64], &pos, 0x30);
                z = z + 1;
            }
        }
        if (pos > out.len) { return 0; }
        let op: usize = 0;
        * = ryuCopySlice(out, &op, tmp[0..pos]);
        return pos;
    }

    let abs_val = if (neg) { -val } else { val };
    let dec_val = ryuExtractShortestF64(abs_val);
    let dbuf: [24]u8 = [0; 24];
    let nd = formatU64(dec_val.sig, dbuf[0..24]);
    let d = dec_val.exp10_msb;

    if (neg) {
        * = ryuPushByte(tmp[0..64], &pos, 0x2D);
    }

    // Total significant digits needed = (d + 1) + dec_places
    let total_needed_i32 = d + 1 + (dec_places as i32);
    if (total_needed_i32 <= 0) {
        // Value is smaller than half of the last decimal place (or rounds up to 0.00..1)
        let round_to_one = (total_needed_i32 == 0) && (nd > 0 && dbuf[0] >= 0x35);
        * = ryuPushByte(tmp[0..64], &pos, 0x30);
        if (dec_places > 0) {
            * = ryuPushByte(tmp[0..64], &pos, 0x2E);
            let z: u8 = 0;
            while (z < dec_places) {
                if (round_to_one && z + 1 == dec_places) {
                    * = ryuPushByte(tmp[0..64], &pos, 0x31);
                } else {
                    * = ryuPushByte(tmp[0..64], &pos, 0x30);
                }
                z = z + 1;
            }
        } else if (round_to_one) {
            tmp[pos - 1] = 0x31;
        }
        if (pos > out.len) { return 0; }
        let op: usize = 0;
        * = ryuCopySlice(out, &op, tmp[0..pos]);
        return pos;
    }

    if (total_needed_i32 > 40) {
        // Fallback to standard representation for gigantic numbers
        return formatF64(val, out);
    }

    let keep: usize = total_needed_i32 as usize;
    let rounded: [48]u8 = [0; 48];
    let carry = ryuRoundAsciiDigits(dbuf[0..nd], keep, rounded[0..48]);

    let int_count: i32 = d + 1 + (if (carry) { 1 as i32 } else { 0 as i32 });
    if (int_count <= 0) {
        // 0.000...rounded
        * = ryuPushByte(tmp[0..64], &pos, 0x30);
        if (dec_places > 0) {
            * = ryuPushByte(tmp[0..64], &pos, 0x2E);
            let lead_zeros: usize = (-int_count) as usize;
            let z: usize = 0;
            while (z < lead_zeros) {
                * = ryuPushByte(tmp[0..64], &pos, 0x30);
                z = z + 1;
            }
            if (carry) {
                * = ryuPushByte(tmp[0..64], &pos, 0x31);
                * = ryuCopySlice(tmp[0..64], &pos, rounded[0..(keep - 1)]);
            } else {
                * = ryuCopySlice(tmp[0..64], &pos, rounded[0..keep]);
            }
        }
    } else {
        let ic: usize = int_count as usize;
        if (carry) {
            * = ryuPushByte(tmp[0..64], &pos, 0x31);
            * = ryuCopySlice(tmp[0..64], &pos, rounded[0..(ic - 1)]);
            if (dec_places > 0) {
                * = ryuPushByte(tmp[0..64], &pos, 0x2E);
                * = ryuCopySlice(tmp[0..64], &pos, rounded[(ic - 1)..keep]);
            }
        } else {
            * = ryuCopySlice(tmp[0..64], &pos, rounded[0..ic]);
            if (dec_places > 0) {
                * = ryuPushByte(tmp[0..64], &pos, 0x2E);
                * = ryuCopySlice(tmp[0..64], &pos, rounded[ic..keep]);
            }
        }
    }

    if (pos > out.len) {
        return 0;
    }
    let op: usize = 0;
    * = ryuCopySlice(out, &op, tmp[0..pos]);
    return pos;
}

/// Formats `val: f64` in scientific notation with `precision` (0..=16) fractional digits
/// (e.g. `formatExpF64(12345.0, 2)` -> `"1.23e4"`, `formatExpF64(0.0015, 1)` -> `"1.5e-3"`).
pub fn formatExpF64(val: f64, precision: u8, out: []u8): usize {
    if (!std.math.isFinite(val)) {
        return formatF64(val, out);
    }
    let prec: u8 = if (precision > 16) { 16 } else { precision };
    let neg = std.math.signbit(val);
    let tmp: [48]u8 = [0; 48];
    let pos: usize = 0;

    if (neg) {
        * = ryuPushByte(tmp[0..48], &pos, 0x2D);
    }

    if (val == 0.0) {
        * = ryuPushByte(tmp[0..48], &pos, 0x30);
        if (prec > 0) {
            * = ryuPushByte(tmp[0..48], &pos, 0x2E);
            let z: u8 = 0;
            while (z < prec) {
                * = ryuPushByte(tmp[0..48], &pos, 0x30);
                z = z + 1;
            }
        }
        * = ryuCopySlice(tmp[0..48], &pos, "e0");
        if (pos > out.len) { return 0; }
        let op: usize = 0;
        * = ryuCopySlice(out, &op, tmp[0..pos]);
        return pos;
    }

    let abs_val = if (neg) { -val } else { val };
    let dec_val = ryuExtractShortestF64(abs_val);
    let dbuf: [24]u8 = [0; 24];
    let nd = formatU64(dec_val.sig, dbuf[0..24]);
    let exp = dec_val.exp10_msb;

    let keep: usize = (prec as usize) + 1;
    let rounded: [24]u8 = [0; 24];
    let carry = ryuRoundAsciiDigits(dbuf[0..nd], keep, rounded[0..24]);
    let final_exp = if (carry) { exp + 1 } else { exp };

    if (carry) {
        * = ryuPushByte(tmp[0..48], &pos, 0x31);
        if (prec > 0) {
            * = ryuPushByte(tmp[0..48], &pos, 0x2E);
            * = ryuCopySlice(tmp[0..48], &pos, rounded[0..(prec as usize)]);
        }
    } else {
        * = ryuPushByte(tmp[0..48], &pos, rounded[0]);
        if (prec > 0) {
            * = ryuPushByte(tmp[0..48], &pos, 0x2E);
            * = ryuCopySlice(tmp[0..48], &pos, rounded[1..keep]);
        }
    }

    * = ryuPushByte(tmp[0..48], &pos, 0x65); // 'e'
    let abs_e: u64 = 0;
    if (final_exp < 0) {
        * = ryuPushByte(tmp[0..48], &pos, 0x2D);
        abs_e = (-final_exp) as u64;
    } else {
        abs_e = final_exp as u64;
    }
    let ebuf: [16]u8 = [0; 16];
    let en = formatU64(abs_e, ebuf[0..16]);
    * = ryuCopySlice(tmp[0..48], &pos, ebuf[0..en]);

    if (pos > out.len) {
        return 0;
    }
    let op: usize = 0;
    * = ryuCopySlice(out, &op, tmp[0..pos]);
    return pos;
}

export {
    formatF64,
    formatF64Finite,
    formatF32,
    formatFixedF64,
    formatExpF64,
    isFiniteF64,
    isNanF64,
    isInfF64
};
