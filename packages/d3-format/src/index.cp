// @cap/d3-format — Zero-heap D3/Python number format specifier parser & formatter
// Clean-room ISC implementation inspired by npm:d3-format (d3/d3-format).

pub struct FormatSpec {
    valid: bool,
    fill: u8,
    @"align": u8,
    sign: u8,
    symbol: u8,
    zero: bool,
    width: usize,
    comma: bool,
    precision: i32,
    trim: bool,
    ty: u8,
}

fn d3fIsAlign(ch: u8): bool {
    return ch == '<' || ch == '>' || ch == '^' || ch == '=';
}

fn d3fIsType(ch: u8): bool {
    return ch == 'f' || ch == 'e' || ch == 'E' || ch == 'g' || ch == 'G' ||
        ch == 'r' || ch == 's' || ch == '%' || ch == 'p' || ch == 'd' ||
        ch == 'x' || ch == 'X' || ch == 'b' || ch == 'o' || ch == 'c';
}

/// Parse a D3 / Python-style format specifier:
/// `[[fill]align][sign][symbol][0][width][,][.precision][~][type]`
pub fn parseFormatSpec(spec: string): FormatSpec {
    let res = FormatSpec{
        valid: true,
        fill: ' ',
        @"align": '>',
        sign: '-',
        symbol: 0,
        zero: false,
        width: 0,
        comma: false,
        precision: -1,
        trim: false,
        ty: 0,
    };

    const n = spec.len;
    let i: usize = 0;

    // 1. [[fill]align]
    if (i + 1 < n && d3fIsAlign(spec[i + 1])) {
        res.fill = spec[i];
        res.@"align" = spec[i + 1];
        i += 2;
    } else if (i < n && d3fIsAlign(spec[i])) {
        res.@"align" = spec[i];
        i += 1;
    }

    // 2. [sign]
    if (i < n && (spec[i] == '+' || spec[i] == '-' || spec[i] == ' ')) {
        res.sign = spec[i];
        i += 1;
    }

    // 3. [symbol]
    if (i < n && (spec[i] == '#' || spec[i] == '$')) {
        res.symbol = spec[i];
        i += 1;
    }

    // 4. [0]
    if (i < n && spec[i] == '0') {
        res.zero = true;
        i += 1;
    }

    // 5. [width]
    while (i < n && spec[i] >= '0' && spec[i] <= '9') : (i += 1) {
        res.width = res.width * 10 + usize(spec[i] - '0');
    }

    // 6. [,]
    if (i < n && spec[i] == ',') {
        res.comma = true;
        i += 1;
    }

    // 7. [.precision]
    if (i < n && spec[i] == '.') {
        i += 1;
        if (i >= n || spec[i] < '0' || spec[i] > '9') {
            res.valid = false;
            return res;
        }
        let prec: i32 = 0;
        while (i < n && spec[i] >= '0' && spec[i] <= '9') : (i += 1) {
            prec = prec * 10 + i32(spec[i] - '0');
        }
        res.precision = prec;
    }

    // 8. [~]
    if (i < n && spec[i] == '~') {
        res.trim = true;
        i += 1;
    }

    // 9. [type]
    if (i < n) {
        if (d3fIsType(spec[i])) {
            res.ty = spec[i];
            i += 1;
        } else {
            res.valid = false;
            return res;
        }
    }

    if (i != n) {
        res.valid = false;
    }
    return res;
}

fn d3fPow10F64(exp: i32): f64 {
    let r: f64 = 1.0;
    let k: i32 = 0;
    while (k < exp) : (k += 1) {
        r *= 10.0;
    }
    return r;
}

fn d3fPow10U64(exp: i32): u64 {
    let r: u64 = 1;
    let k: i32 = 0;
    while (k < exp) : (k += 1) {
        r *= 10;
    }
    return r;
}

fn d3fWriteU64Base(val: u64, base: u64, upper: bool, comma: bool, out: []u8): usize {
    let rev: [64]u8 = undefined;
    let n: usize = 0;
    let v = val;
    const digits: string = if (upper) "0123456789ABCDEF" else "0123456789abcdef";

    if (v == 0) {
        rev[0] = '0';
        n = 1;
    } else {
        while (v > 0 && n < 64) {
            const d = usize(v % base);
            rev[n] = digits[d];
            n += 1;
            v /= base;
        }
    }

    let o: usize = 0;
    let i: usize = n;
    while (i > 0) {
        i -= 1;
        if (o >= out.len) {
            return o;
        }
        out[o] = rev[i];
        o += 1;
        if (comma && base == 10 && i > 0 && (i % 3) == 0) {
            if (o >= out.len) {
                return o;
            }
            out[o] = ',';
            o += 1;
        }
    }
    return o;
}

fn d3fWriteFixed(abs_val: f64, prec_in: i32, comma: bool, trim: bool, out: []u8): usize {
    let prec = prec_in;
    if (prec < 0) {
        prec = 6;
    }
    if (prec > 12) {
        prec = 12;
    }

    let int_part: u64 = 0;
    let frac_part: u64 = 0;

    if (prec == 0) {
        const rounded = @round(abs_val);
        if (rounded >= 1.8e19) {
            int_part = std.math.maxInt(u64);
        } else {
            int_part = @as(u64, @intFromFloat(rounded));
        }
    } else {
        const p10f = d3fPow10F64(prec);
        const scaled = @round(abs_val * p10f);
        if (scaled < 9.0e18) {
            const p10u = d3fPow10U64(prec);
            const total = @as(u64, @intFromFloat(scaled));
            int_part = total / p10u;
            frac_part = total % p10u;
        } else {
            const trunc_v = @trunc(abs_val);
            if (trunc_v >= 1.8e19) {
                int_part = std.math.maxInt(u64);
            } else {
                int_part = @as(u64, @intFromFloat(trunc_v));
            }
            const rem_f = @round((abs_val - trunc_v) * p10f);
            frac_part = @as(u64, @intFromFloat(rem_f));
        }
    }

    let o = d3fWriteU64Base(int_part, 10, false, comma, out);
    if (prec > 0) {
        let fbuf: [16]u8 = undefined;
        let k: usize = usize(prec);
        let fv = frac_part;
        while (k > 0) {
            k -= 1;
            fbuf[k] = u8('0' + (fv % 10));
            fv /= 10;
        }
        let flen: usize = usize(prec);
        if (trim) {
            while (flen > 0 && fbuf[flen - 1] == '0') {
                flen -= 1;
            }
        }
        if (flen > 0) {
            if (o < out.len) {
                out[o] = '.';
                o += 1;
            }
            let j: usize = 0;
            while (j < flen && o < out.len) : (j += 1) {
                out[o] = fbuf[j];
                o += 1;
            }
        }
    }
    return o;
}

fn d3fAppendSiSuffix(exp3: i32, out: []u8, pos: usize): usize {
    if (exp3 == 0) {
        return pos;
    }
    if (exp3 == -6) {
        // UTF-8 'µ' (U+00B5: 0xC2 0xB5)
        if (pos + 2 <= out.len) {
            out[pos] = 0xC2;
            out[pos + 1] = 0xB5;
            return pos + 2;
        }
        return pos;
    }
    let ch: u8 = 0;
    if (exp3 == -24) { ch = 'y'; }
    else if (exp3 == -21) { ch = 'z'; }
    else if (exp3 == -18) { ch = 'a'; }
    else if (exp3 == -15) { ch = 'f'; }
    else if (exp3 == -12) { ch = 'p'; }
    else if (exp3 == -9) { ch = 'n'; }
    else if (exp3 == -3) { ch = 'm'; }
    else if (exp3 == 3) { ch = 'k'; }
    else if (exp3 == 6) { ch = 'M'; }
    else if (exp3 == 9) { ch = 'G'; }
    else if (exp3 == 12) { ch = 'T'; }
    else if (exp3 == 15) { ch = 'P'; }
    else if (exp3 == 18) { ch = 'E'; }
    else if (exp3 == 21) { ch = 'Z'; }
    else if (exp3 == 24) { ch = 'Y'; }
    if (ch != 0 && pos < out.len) {
        out[pos] = ch;
        return pos + 1;
    }
    return pos;
}

fn d3fDisplayLen(slice: []const u8): usize {
    // Count UTF-8 codepoints (so 'µ' counts as 1 display column)
    let count: usize = 0;
    let i: usize = 0;
    while (i < slice.len) : (i += 1) {
        if ((slice[i] & 0xC0) != 0x80) {
            count += 1;
        }
    }
    return count;
}

fn d3fCopySlice(dst: []u8, pos: usize, src: []const u8): usize {
    let o = pos;
    let i: usize = 0;
    while (i < src.len && o < dst.len) : (i += 1) {
        dst[o] = src[i];
        o += 1;
    }
    return o;
}

/// Format `value` according to D3/Python format specifier `spec` into `out`.
/// Returns number of bytes written to `out` (0 on invalid spec or empty buffer).
pub fn formatNumber(spec: string, value: f64, out: []u8): usize {
    if (out.len == 0) {
        return 0;
    }
    const s = parseFormatSpec(spec);
    if (!s.valid) {
        return 0;
    }

    // Handle NaN
    if (std.math.isNan(value)) {
        return d3fCopySlice(out, 0, "NaN");
    }

    const is_neg = value < 0.0;
    let abs_val = if (is_neg) -value else value;

    // Build prefix (sign + symbol)
    let pfx: [8]u8 = undefined;
    let pfx_len: usize = 0;
    if (is_neg) {
        pfx[pfx_len] = '-';
        pfx_len += 1;
    } else if (s.sign == '+') {
        pfx[pfx_len] = '+';
        pfx_len += 1;
    } else if (s.sign == ' ') {
        pfx[pfx_len] = ' ';
        pfx_len += 1;
    }

    if (s.symbol == '$') {
        pfx[pfx_len] = '$';
        pfx_len += 1;
    } else if (s.symbol == '#') {
        if (s.ty == 'x') {
            pfx[pfx_len] = '0';
            pfx[pfx_len + 1] = 'x';
            pfx_len += 2;
        } else if (s.ty == 'X') {
            pfx[pfx_len] = '0';
            pfx[pfx_len + 1] = 'X';
            pfx_len += 2;
        } else if (s.ty == 'b') {
            pfx[pfx_len] = '0';
            pfx[pfx_len + 1] = 'b';
            pfx_len += 2;
        } else if (s.ty == 'o') {
            pfx[pfx_len] = '0';
            pfx[pfx_len + 1] = 'o';
            pfx_len += 2;
        }
    }

    // Build body
    let body: [128]u8 = undefined;
    let body_len: usize = 0;

    if (std.math.isInf(value)) {
        body_len = d3fCopySlice(body[0..], 0, "Infinity");
    } else if (s.ty == 'f') {
        body_len = d3fWriteFixed(abs_val, s.precision, s.comma, s.trim, body[0..]);
    } else if (s.ty == '%' || s.ty == 'p') {
        const pct = abs_val * 100.0;
        body_len = d3fWriteFixed(pct, s.precision, s.comma, s.trim, body[0..]);
        if (body_len < body.len) {
            body[body_len] = '%';
            body_len += 1;
        }
    } else if (s.ty == 's') {
        let exp3: i32 = 0;
        let scaled = abs_val;
        if (scaled > 0.0) {
            while (scaled >= 1000.0 && exp3 < 24) {
                scaled /= 1000.0;
                exp3 += 3;
            }
            while (scaled < 1.0 && exp3 > -24) {
                scaled *= 1000.0;
                exp3 -= 3;
            }
        }
        let sig: i32 = if (s.precision < 0) 6 else s.precision;
        if (sig < 1) {
            sig = 1;
        }
        let idig: i32 = 1;
        if (scaled >= 100.0) {
            idig = 3;
        } else if (scaled >= 10.0) {
            idig = 2;
        }
        let frac_d: i32 = if (sig > idig) sig - idig else 0;
        // Handle rounding carry across 1000 boundary (e.g. 999.9 -> 1.0k)
        if (scaled > 0.0 && exp3 < 24) {
            const p10 = d3fPow10F64(frac_d);
            if (@round(scaled * p10) >= 1000.0 * p10) {
                scaled /= 1000.0;
                exp3 += 3;
                idig = 1;
                frac_d = if (sig > 1) sig - 1 else 0;
            }
        }
        body_len = d3fWriteFixed(scaled, frac_d, s.comma, s.trim, body[0..]);
        body_len = d3fAppendSiSuffix(exp3, body[0..], body_len);
    } else if (s.ty == 'd') {
        const rounded = @round(abs_val);
        const iv = @as(u64, @intFromFloat(rounded));
        body_len = d3fWriteU64Base(iv, 10, false, s.comma, body[0..]);
    } else if (s.ty == 'x') {
        const rounded = @round(abs_val);
        const iv = @as(u64, @intFromFloat(rounded));
        body_len = d3fWriteU64Base(iv, 16, false, false, body[0..]);
    } else if (s.ty == 'X') {
        const rounded = @round(abs_val);
        const iv = @as(u64, @intFromFloat(rounded));
        body_len = d3fWriteU64Base(iv, 16, true, false, body[0..]);
    } else if (s.ty == 'b') {
        const rounded = @round(abs_val);
        const iv = @as(u64, @intFromFloat(rounded));
        body_len = d3fWriteU64Base(iv, 2, false, false, body[0..]);
    } else if (s.ty == 'o') {
        const rounded = @round(abs_val);
        const iv = @as(u64, @intFromFloat(rounded));
        body_len = d3fWriteU64Base(iv, 8, false, false, body[0..]);
    } else if (s.ty == 'e' || s.ty == 'E') {
        let exp10: i32 = 0;
        let mant = abs_val;
        if (mant > 0.0) {
            while (mant >= 10.0) {
                mant /= 10.0;
                exp10 += 1;
            }
            while (mant < 1.0) {
                mant *= 10.0;
                exp10 -= 1;
            }
        }
        const prec = if (s.precision < 0) 6 else s.precision;
        body_len = d3fWriteFixed(mant, prec, false, s.trim, body[0..]);
        if (body_len < body.len) {
            body[body_len] = s.ty;
            body_len += 1;
        }
        if (body_len < body.len) {
            body[body_len] = if (exp10 < 0) '-' else '+';
            body_len += 1;
        }
        const aexp: u64 = if (exp10 < 0) u64(-exp10) else u64(exp10);
        if (aexp < 10 && body_len < body.len) {
            body[body_len] = '0';
            body_len += 1;
        }
        body_len += d3fWriteU64Base(aexp, 10, false, false, body[body_len..]);
    } else {
        // Default / empty type or 'g'/'r'
        if (s.precision >= 0) {
            body_len = d3fWriteFixed(abs_val, s.precision, s.comma, s.trim, body[0..]);
        } else if (abs_val == @trunc(abs_val) && abs_val < 1.0e15) {
            const iv = @as(u64, @intFromFloat(abs_val));
            body_len = d3fWriteU64Base(iv, 10, false, s.comma, body[0..]);
        } else {
            body_len = d3fWriteFixed(abs_val, 6, s.comma, true, body[0..]);
        }
    }

    // Apply width, zero-padding, and alignment
    const disp_len = pfx_len + d3fDisplayLen(body[0..body_len]);
    let pad_count: usize = 0;
    if (s.width > disp_len) {
        pad_count = s.width - disp_len;
    }

    let eff_align = s.@"align";
    let eff_fill = s.fill;
    if (s.zero && s.@"align" == '>' && s.fill == ' ') {
        eff_align = '=';
        eff_fill = '0';
    }

    let o: usize = 0;
    if (pad_count == 0) {
        o = d3fCopySlice(out, o, pfx[0..pfx_len]);
        o = d3fCopySlice(out, o, body[0..body_len]);
        return o;
    }

    if (eff_align == '<') {
        o = d3fCopySlice(out, o, pfx[0..pfx_len]);
        o = d3fCopySlice(out, o, body[0..body_len]);
        let p: usize = 0;
        while (p < pad_count && o < out.len) : (p += 1) {
            out[o] = eff_fill;
            o += 1;
        }
    } else if (eff_align == '=') {
        o = d3fCopySlice(out, o, pfx[0..pfx_len]);
        let p: usize = 0;
        while (p < pad_count && o < out.len) : (p += 1) {
            out[o] = eff_fill;
            o += 1;
        }
        o = d3fCopySlice(out, o, body[0..body_len]);
    } else if (eff_align == '^') {
        const left_pad = pad_count / 2;
        const right_pad = pad_count - left_pad;
        let p: usize = 0;
        while (p < left_pad && o < out.len) : (p += 1) {
            out[o] = eff_fill;
            o += 1;
        }
        o = d3fCopySlice(out, o, pfx[0..pfx_len]);
        o = d3fCopySlice(out, o, body[0..body_len]);
        p = 0;
        while (p < right_pad && o < out.len) : (p += 1) {
            out[o] = eff_fill;
            o += 1;
        }
    } else {
        // Right align '>'
        let p: usize = 0;
        while (p < pad_count && o < out.len) : (p += 1) {
            out[o] = eff_fill;
            o += 1;
        }
        o = d3fCopySlice(out, o, pfx[0..pfx_len]);
        o = d3fCopySlice(out, o, body[0..body_len]);
    }

    return o;
}
