// @cap/lexical-parse-float — Zero-allocation pure CP strict numeric string parser
// Parity with Alexhuszagh/rust-lexical crates.io:lexical-parse-float / lexical-parse-integer / lexical-util (MIT)

/// Result of parsing a floating-point string.
pub struct ParseFloatResult {
    ok: bool;
    value: f64;
    consumed: usize;
}

/// Result of parsing a signed integer string in a given radix.
pub struct ParseIntResult {
    ok: bool;
    value: i64;
    consumed: usize;
}

const LEXP_POW10: [23]f64 = @as([23]f64, [
    1.0, 1.0e1, 1.0e2, 1.0e3, 1.0e4, 1.0e5, 1.0e6, 1.0e7,
    1.0e8, 1.0e9, 1.0e10, 1.0e11, 1.0e12, 1.0e13, 1.0e14, 1.0e15,
    1.0e16, 1.0e17, 1.0e18, 1.0e19, 1.0e20, 1.0e21, 1.0e22,
]);

fn lexpToLower(c: u8): u8 {
    if (c >= ('A' as u8) && c <= ('Z' as u8)) {
        return c + (32 as u8);
    }
    return c;
}

fn lexpEqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (lexpToLower(a[i]) != lexpToLower(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn lexpDecodeDigit(c: u8): i32 {
    if (c >= ('0' as u8) && c <= ('9' as u8)) {
        return (c - ('0' as u8)) as i32;
    }
    if (c >= ('a' as u8) && c <= ('z' as u8)) {
        return ((c - ('a' as u8)) as i32) + 10;
    }
    if (c >= ('A' as u8) && c <= ('Z' as u8)) {
        return ((c - ('A' as u8)) as i32) + 10;
    }
    return -1;
}

fn lexpScalePow10(val_in: f64, exp10: i32): f64 {
    let val: f64 = val_in;
    if (exp10 == 0) {
        return val;
    }
    if (exp10 > 0) {
        let rem: i32 = exp10;
        while (rem > 22) {
            val = val * 1.0e22;
            rem -= 22;
        }
        val = val * LEXP_POW10[rem as usize];
        return val;
    }
    let rem: i32 = -exp10;
    while (rem > 22) {
        val = val / 1.0e22;
        rem -= 22;
    }
    val = val / LEXP_POW10[rem as usize];
    return val;
}

/// Strictly parse an IEEE-754 `f64` from `s`.
/// Supports optional `+`/`-` sign, fractional part, `e`/`E` scientific exponent,
/// `inf`/`infinity`/`nan` (case-insensitive), and rejects empty strings or trailing garbage.
pub fn parseFloat(s: string): ParseFloatResult {
    const fail = ParseFloatResult{ ok: false, value: 0.0, consumed: 0 };
    if (s.len == 0) {
        return fail;
    }

    let pos: usize = 0;
    let neg: bool = false;
    if (s[pos] == ('-' as u8)) {
        neg = true;
        pos += 1;
    } else if (s[pos] == ('+' as u8)) {
        pos += 1;
    }

    if (pos >= s.len) {
        return fail;
    }

    const rest = s[pos..s.len];
    if (lexpEqIgnoreCase(rest, "inf") || lexpEqIgnoreCase(rest, "infinity")) {
        const infVal: f64 = std.math.inf(f64);
        const outVal: f64 = if (neg) -infVal else infVal;
        return ParseFloatResult{ ok: true, value: outVal, consumed: s.len };
    }
    if (lexpEqIgnoreCase(rest, "nan")) {
        return ParseFloatResult{ ok: true, value: std.math.nan(f64), consumed: s.len };
    }

    let mantissa: u64 = 0;
    let sigDigits: usize = 0;
    let totalDigits: usize = 0;
    let expAdjust: i32 = 0;
    let rounded: bool = false;

    // Integer part
    while (pos < s.len) {
        const c: u8 = s[pos];
        if (c < ('0' as u8) || c > ('9' as u8)) {
            break;
        }
        const d: u64 = (c - ('0' as u8)) as u64;
        totalDigits += 1;
        if (sigDigits == 0 && d == 0) {
            // Leading zero: skip for significand precision
        } else if (sigDigits < 19) {
            mantissa = mantissa * 10 + d;
            sigDigits += 1;
        } else {
            if (!rounded) {
                if (d >= 5) {
                    mantissa += 1;
                }
                rounded = true;
            }
            expAdjust += 1;
        }
        pos += 1;
    }

    // Fractional part
    if (pos < s.len && s[pos] == ('.' as u8)) {
        pos += 1;
        while (pos < s.len) {
            const c: u8 = s[pos];
            if (c < ('0' as u8) || c > ('9' as u8)) {
                break;
            }
            const d: u64 = (c - ('0' as u8)) as u64;
            totalDigits += 1;
            if (sigDigits == 0 && d == 0) {
                expAdjust -= 1;
            } else if (sigDigits < 19) {
                mantissa = mantissa * 10 + d;
                sigDigits += 1;
                expAdjust -= 1;
            } else {
                if (!rounded) {
                    if (d >= 5) {
                        mantissa += 1;
                    }
                    rounded = true;
                }
            }
            pos += 1;
        }
    }

    if (totalDigits == 0) {
        return fail;
    }

    // Optional exponent part
    let expVal: i32 = 0;
    if (pos < s.len && (s[pos] == ('e' as u8) || s[pos] == ('E' as u8))) {
        pos += 1;
        let expNeg: bool = false;
        if (pos < s.len && s[pos] == ('-' as u8)) {
            expNeg = true;
            pos += 1;
        } else if (pos < s.len && s[pos] == ('+' as u8)) {
            pos += 1;
        }
        let expDigits: usize = 0;
        while (pos < s.len) {
            const c: u8 = s[pos];
            if (c < ('0' as u8) || c > ('9' as u8)) {
                break;
            }
            expDigits += 1;
            if (expVal < 10000) {
                expVal = expVal * 10 + ((c - ('0' as u8)) as i32);
            }
            pos += 1;
        }
        if (expDigits == 0) {
            return fail;
        }
        if (expNeg) {
            expVal = -expVal;
        }
    }

    // Reject trailing garbage
    if (pos != s.len) {
        return fail;
    }

    if (mantissa == 0) {
        const zeroVal: f64 = if (neg) -0.0 else 0.0;
        return ParseFloatResult{ ok: true, value: zeroVal, consumed: s.len };
    }

    const totalExp: i32 = expVal + expAdjust;
    if (totalExp > 309) {
        const infVal: f64 = std.math.inf(f64);
        const outVal: f64 = if (neg) -infVal else infVal;
        return ParseFloatResult{ ok: true, value: outVal, consumed: s.len };
    }
    if (totalExp < -325) {
        const zeroVal: f64 = if (neg) -0.0 else 0.0;
        return ParseFloatResult{ ok: true, value: zeroVal, consumed: s.len };
    }

    let val: f64 = lexpScalePow10(mantissa as f64, totalExp);
    if (neg) {
        val = -val;
    }
    return ParseFloatResult{ ok: true, value: val, consumed: s.len };
}

/// Strictly parse a signed 64-bit integer in base `radix` (2..=36) with overflow checking.
pub fn parseIntRadix(s: string, radix: u32): ParseIntResult {
    const fail = ParseIntResult{ ok: false, value: 0, consumed: 0 };
    if (radix < 2 || radix > 36 || s.len == 0) {
        return fail;
    }

    let pos: usize = 0;
    let neg: bool = false;
    if (s[pos] == ('-' as u8)) {
        neg = true;
        pos += 1;
    } else if (s[pos] == ('+' as u8)) {
        pos += 1;
    }

    if (pos >= s.len) {
        return fail;
    }

    const maxPos: u64 = 9223372036854775807;
    const maxNegAbs: u64 = ((0x80000000 as u64) << 32);
    const limit: u64 = if (neg) maxNegAbs else maxPos;
    const r64: u64 = radix as u64;

    let acc: u64 = 0;
    while (pos < s.len) {
        const d: i32 = lexpDecodeDigit(s[pos]);
        if (d < 0 || (d as u32) >= radix) {
            return fail;
        }
        const d64: u64 = d as u64;
        if (acc > (limit - d64) / r64) {
            return fail;
        }
        acc = acc * r64 + d64;
        pos += 1;
    }

    let outVal: i64 = 0;
    if (neg) {
        if (acc == maxNegAbs) {
            outVal = (-9223372036854775807 as i64) - (1 as i64);
        } else {
            const posVal: i64 = acc as i64;
            outVal = -posVal;
        }
    } else {
        outVal = acc as i64;
    }
    return ParseIntResult{ ok: true, value: outVal, consumed: s.len };
}
