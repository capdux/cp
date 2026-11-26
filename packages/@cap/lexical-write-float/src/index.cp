// @cap/lexical-write-float — Zero-allocation pure CP integer & floating-point formatter
// Parity with Alexhuszagh/rust-lexical crates.io:lexical-write-float / lexical-write-integer (MIT)

const LEXW_POW10_F64: [19]f64 = @as([19]f64, [
    1.0, 1.0e1, 1.0e2, 1.0e3, 1.0e4, 1.0e5, 1.0e6, 1.0e7, 1.0e8, 1.0e9,
    1.0e10, 1.0e11, 1.0e12, 1.0e13, 1.0e14, 1.0e15, 1.0e16, 1.0e17, 1.0e18,
]);

const LEXW_POW10_U64: [19]u64 = @as([19]u64, [
    1,
    10,
    100,
    1000,
    10000,
    100000,
    1000000,
    10000000,
    100000000,
    1000000000,
    10000000000,
    100000000000,
    1000000000000,
    10000000000000,
    100000000000000,
    1000000000000000,
    10000000000000000,
    100000000000000000,
    1000000000000000000,
]);

fn lexwCopyStr(out: []u8, pos: usize, src: string): usize {
    if (pos + src.len > out.len) {
        return 0;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn lexwWritePaddedU64(out: []u8, pos: usize, val: u64, width: usize): usize {
    if (pos + width > out.len) {
        return 0;
    }
    let v: u64 = val;
    let i: usize = width;
    while (i > 0) {
        i -= 1;
        const d: u8 = (v % 10) as u8;
        out[pos + i] = ('0' as u8) + d;
        v = v / 10;
    }
    return pos + width;
}

fn lexwWriteSpecialFloat(val: f64, out: []u8): usize {
    if (std.math.isNan(val)) {
        return lexwCopyStr(out, 0, "NaN");
    }
    if (std.math.isPositiveInf(val)) {
        return lexwCopyStr(out, 0, "inf");
    }
    if (std.math.isNegativeInf(val)) {
        return lexwCopyStr(out, 0, "-inf");
    }
    return 0;
}

/// Format an unsigned 64-bit integer in base `radix` (2..=36).
/// Returns bytes written, or 0 on invalid radix or insufficient buffer.
pub fn writeUintRadix(val: u64, radix: u32, uppercase: bool, out: []u8): usize {
    if (radix < 2 || radix > 36) {
        return 0;
    }
    let tmp: [64]u8 = undefined;
    let count: usize = 0;
    const r64: u64 = radix as u64;
    const alphaBase: u8 = if (uppercase) ('A' as u8) else ('a' as u8);
    if (val == 0) {
        tmp[0] = '0' as u8;
        count = 1;
    } else {
        let v: u64 = val;
        while (v > 0) {
            const rem: u8 = (v % r64) as u8;
            if (rem < (10 as u8)) {
                tmp[count] = ('0' as u8) + rem;
            } else {
                tmp[count] = alphaBase + (rem - (10 as u8));
            }
            count += 1;
            v = v / r64;
        }
    }
    if (out.len < count) {
        return 0;
    }
    let i: usize = 0;
    while (i < count) {
        out[i] = tmp[count - 1 - i];
        i += 1;
    }
    return count;
}

/// Format a signed 64-bit integer in base `radix` (2..=36), safely handling `i64::MIN`.
/// Returns bytes written, or 0 on invalid radix or insufficient buffer.
pub fn writeIntRadix(val: i64, radix: u32, uppercase: bool, out: []u8): usize {
    if (radix < 2 || radix > 36) {
        return 0;
    }
    if (val < 0) {
        if (out.len < 2) {
            return 0;
        }
        const minI64: i64 = (-9223372036854775807 as i64) - (1 as i64);
        let mag: u64 = 0;
        if (val == minI64) {
            mag = ((0x80000000 as u64) << 32);
        } else {
            const posVal: i64 = -val;
            mag = posVal as u64;
        }
        const n = writeUintRadix(mag, radix, uppercase, out[1..out.len]);
        if (n == 0) {
            return 0;
        }
        out[0] = '-' as u8;
        return n + 1;
    }
    return writeUintRadix(val as u64, radix, uppercase, out);
}

/// Format an IEEE-754 `f64` in fixed-point notation with `decimals` fractional digits.
/// Supports `NaN`, `inf`, `-inf`, negative zero (`-0.0`), and half-up rounding.
pub fn writeFloatFixed(val: f64, decimals: u8, out: []u8): usize {
    if (std.math.isNan(val) || std.math.isInf(val)) {
        return lexwWriteSpecialFloat(val, out);
    }

    const isNeg: bool = std.math.signbit(val);
    const absVal: f64 = if (isNeg) -val else val;
    const decCount: usize = if ((decimals as usize) > 18) 18 else (decimals as usize);

    let pos: usize = 0;
    if (isNeg) {
        if (out.len < 1) {
            return 0;
        }
        out[0] = '-' as u8;
        pos = 1;
    }

    const intFloorF: f64 = @floor(absVal);
    const fracPart: f64 = absVal - intFloorF;
    const scaleF: f64 = LEXW_POW10_F64[decCount];

    let intPart: u64 = @as(u64, @intFromFloat(intFloorF));
    let fracRoundedF: f64 = @floor(fracPart * scaleF + 0.5);
    let fracInt: u64 = 0;
    if (fracRoundedF >= scaleF) {
        intPart += 1;
        fracInt = 0;
    } else {
        fracInt = @as(u64, @intFromFloat(fracRoundedF));
    }

    const intWritten = writeUintRadix(intPart, 10, false, out[pos..out.len]);
    if (intWritten == 0) {
        return 0;
    }
    pos += intWritten;

    if (decCount > 0) {
        if (pos >= out.len) {
            return 0;
        }
        out[pos] = '.' as u8;
        pos += 1;
        pos = lexwWritePaddedU64(out, pos, fracInt, decCount);
        if (pos == 0) {
            return 0;
        }
    }

    return pos;
}

/// Format an IEEE-754 `f64` in scientific notation with `precision` fractional digits
/// (e.g. `1.23e+4`, `-5.00e-3`). Supports `NaN`, `inf`, `-inf`, and negative zero.
pub fn writeFloatSci(val: f64, precision: u8, out: []u8): usize {
    if (std.math.isNan(val) || std.math.isInf(val)) {
        return lexwWriteSpecialFloat(val, out);
    }

    const isNeg: bool = std.math.signbit(val);
    let absVal: f64 = if (isNeg) -val else val;
    const prec: usize = if ((precision as usize) > 17) 17 else (precision as usize);

    let pos: usize = 0;
    if (isNeg) {
        if (out.len < 1) {
            return 0;
        }
        out[0] = '-' as u8;
        pos = 1;
    }

    let exp10: i32 = 0;
    let leadDigit: u64 = 0;
    let fracDigits: u64 = 0;

    if (absVal > 0.0) {
        while (absVal >= 1.0e16) {
            absVal = absVal / 1.0e16;
            exp10 += 16;
        }
        while (absVal >= 10.0) {
            absVal = absVal / 10.0;
            exp10 += 1;
        }
        while (absVal < 1.0e-15) {
            absVal = absVal * 1.0e16;
            exp10 -= 16;
        }
        while (absVal < 1.0) {
            absVal = absVal * 10.0;
            exp10 -= 1;
        }

        const scaleF: f64 = LEXW_POW10_F64[prec];
        let scaledF: f64 = @floor(absVal * scaleF + 0.5);
        if (scaledF >= scaleF * 10.0) {
            scaledF = @floor(scaledF / 10.0 + 0.5);
            exp10 += 1;
        }
        const scaledInt: u64 = @as(u64, @intFromFloat(scaledF));
        const scaleU64: u64 = LEXW_POW10_U64[prec];
        leadDigit = scaledInt / scaleU64;
        fracDigits = scaledInt % scaleU64;
    }

    if (pos >= out.len) {
        return 0;
    }
    out[pos] = ('0' as u8) + (leadDigit as u8);
    pos += 1;

    if (prec > 0) {
        if (pos >= out.len) {
            return 0;
        }
        out[pos] = '.' as u8;
        pos += 1;
        pos = lexwWritePaddedU64(out, pos, fracDigits, prec);
        if (pos == 0) {
            return 0;
        }
    }

    if (pos + 2 > out.len) {
        return 0;
    }
    out[pos] = 'e' as u8;
    pos += 1;
    if (exp10 < 0) {
        out[pos] = '-' as u8;
        exp10 = -exp10;
    } else {
        out[pos] = '+' as u8;
    }
    pos += 1;

    const expWritten = writeUintRadix(exp10 as u64, 10, false, out[pos..out.len]);
    if (expWritten == 0) {
        return 0;
    }
    return pos + expWritten;
}
