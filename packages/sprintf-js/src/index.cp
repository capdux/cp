// @cap/sprintf-js — Zero-allocation C-style sprintf formatting engine
// Inspired by npm:sprintf-js (https://github.com/alexei/sprintf.js)

pub enum FmtArgKind {
    Int,
    Uint,
    Float,
    Str,
    Char,
    Bool,
}

pub struct FmtArg {
    kind: FmtArgKind,
    int_val: i64,
    uint_val: u64,
    float_val: f64,
    str_val: string,
    char_val: u8,
    bool_val: bool,

    static fn int(v: i64): FmtArg {
        return FmtArg{
            kind: FmtArgKind.Int,
            int_val: v,
            uint_val: 0,
            float_val: 0.0,
            str_val: "",
            char_val: 0,
            bool_val: false,
        };
    }

    static fn uint(v: u64): FmtArg {
        return FmtArg{
            kind: FmtArgKind.Uint,
            int_val: 0,
            uint_val: v,
            float_val: 0.0,
            str_val: "",
            char_val: 0,
            bool_val: false,
        };
    }

    static fn float(v: f64): FmtArg {
        return FmtArg{
            kind: FmtArgKind.Float,
            int_val: 0,
            uint_val: 0,
            float_val: v,
            str_val: "",
            char_val: 0,
            bool_val: false,
        };
    }

    static fn str(v: string): FmtArg {
        return FmtArg{
            kind: FmtArgKind.Str,
            int_val: 0,
            uint_val: 0,
            float_val: 0.0,
            str_val: v,
            char_val: 0,
            bool_val: false,
        };
    }

    static fn ch(v: u8): FmtArg {
        return FmtArg{
            kind: FmtArgKind.Char,
            int_val: 0,
            uint_val: 0,
            float_val: 0.0,
            str_val: "",
            char_val: v,
            bool_val: false,
        };
    }

    static fn boolean(v: bool): FmtArg {
        return FmtArg{
            kind: FmtArgKind.Bool,
            int_val: 0,
            uint_val: 0,
            float_val: 0.0,
            str_val: "",
            char_val: 0,
            bool_val: v,
        };
    }
}

fn spfPutByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn spfPutBytes(out: []u8, pos: usize, s: []const u8): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

fn spfPutRepeat(out: []u8, pos: usize, b: u8, count: usize): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < count && p < out.len) : (i += 1) {
        out[p] = b;
        p += 1;
    }
    return p;
}

fn spfEmitPadded(
    out: []u8,
    pos: usize,
    sign: u8,
    body: []const u8,
    width: usize,
    left_align: bool,
    zero_pad: bool
): usize {
    const sign_len: usize = if (sign != 0) 1 else 0;
    const total: usize = sign_len + body.len;
    const pad: usize = if (width > total) width - total else 0;
    let p: usize = pos;

    if (left_align) {
        if (sign != 0) {
            p = spfPutByte(out, p, sign);
        }
        p = spfPutBytes(out, p, body);
        p = spfPutRepeat(out, p, 0x20, pad);
        return p;
    }

    if (zero_pad) {
        if (sign != 0) {
            p = spfPutByte(out, p, sign);
        }
        p = spfPutRepeat(out, p, 0x30, pad);
        p = spfPutBytes(out, p, body);
        return p;
    }

    p = spfPutRepeat(out, p, 0x20, pad);
    if (sign != 0) {
        p = spfPutByte(out, p, sign);
    }
    p = spfPutBytes(out, p, body);
    return p;
}

fn spfFormatUintRadix(val: u64, base: u64, upper: bool, buf: []u8): usize {
    const digits: string = if (upper) "0123456789ABCDEF" else "0123456789abcdef";
    let tmp: [72]u8 = [0; 72];
    let n: usize = 0;
    let v: u64 = val;

    if (v == 0) {
        tmp[0] = 0x30;
        n = 1;
    } else {
        while (v > 0 && n < 72) {
            const rem: usize = usize(v % base);
            tmp[n] = digits[rem];
            n += 1;
            v = v / base;
        }
    }

    let i: usize = 0;
    while (i < n && i < buf.len) : (i += 1) {
        buf[i] = tmp[n - 1 - i];
    }
    return i;
}

fn spfArgAsSigned(arg: FmtArg): i64 {
    if (arg.kind == FmtArgKind.Int) {
        return arg.int_val;
    }
    if (arg.kind == FmtArgKind.Uint) {
        return i64(arg.uint_val);
    }
    if (arg.kind == FmtArgKind.Char) {
        return i64(arg.char_val);
    }
    if (arg.kind == FmtArgKind.Bool) {
        return if (arg.bool_val) 1 else 0;
    }
    if (arg.kind == FmtArgKind.Float) {
        return @as(i64, @intFromFloat(arg.float_val));
    }
    return 0;
}

fn spfArgAsUnsigned(arg: FmtArg): u64 {
    if (arg.kind == FmtArgKind.Uint) {
        return arg.uint_val;
    }
    if (arg.kind == FmtArgKind.Int) {
        if (arg.int_val >= 0) {
            return u64(arg.int_val);
        }
        return @as(u64, @bitCast(arg.int_val));
    }
    if (arg.kind == FmtArgKind.Char) {
        return u64(arg.char_val);
    }
    if (arg.kind == FmtArgKind.Bool) {
        return if (arg.bool_val) 1 else 0;
    }
    if (arg.kind == FmtArgKind.Float) {
        if (arg.float_val < 0.0) {
            return 0;
        }
        return @as(u64, @intFromFloat(arg.float_val));
    }
    return 0;
}

fn spfArgAsFloat(arg: FmtArg): f64 {
    if (arg.kind == FmtArgKind.Float) {
        return arg.float_val;
    }
    if (arg.kind == FmtArgKind.Int) {
        return @as(f64, @floatFromInt(arg.int_val));
    }
    if (arg.kind == FmtArgKind.Uint) {
        return @as(f64, @floatFromInt(arg.uint_val));
    }
    if (arg.kind == FmtArgKind.Bool) {
        return if (arg.bool_val) 1.0 else 0.0;
    }
    return 0.0;
}

fn spfFormatFloat(
    val: f64,
    prec: usize,
    plus_sign: bool,
    width: usize,
    left_align: bool,
    zero_pad: bool,
    out: []u8,
    pos: usize
): usize {
    if (std.math.isNan(val)) {
        return spfEmitPadded(out, pos, 0, "NaN", width, left_align, false);
    }
    if (std.math.isInf(val)) {
        const inf_sign: u8 = if (val < 0.0) 0x2D else if (plus_sign) 0x2B else 0;
        return spfEmitPadded(out, pos, inf_sign, "Inf", width, left_align, false);
    }

    const neg: bool = val < 0.0;
    const abs_v: f64 = if (neg) -val else val;
    const sign: u8 = if (neg) 0x2D else if (plus_sign) 0x2B else 0;
    const p_clamped: usize = if (prec > 12) 12 else prec;

    let int_part = @as(u64, @intFromFloat(abs_v));
    const frac: f64 = abs_v - @as(f64, @floatFromInt(int_part));

    let scale: u64 = 1;
    let k: usize = 0;
    while (k < p_clamped) : (k += 1) {
        scale *= 10;
    }

    let frac_int = @as(u64, @intFromFloat(frac * @as(f64, @floatFromInt(scale)) + 0.5));
    if (p_clamped == 0) {
        if (frac >= 0.5) {
            int_part += 1;
        }
        frac_int = 0;
    } else if (frac_int >= scale) {
        int_part += 1;
        frac_int -= scale;
    }

    let body: [96]u8 = [0; 96];
    let int_buf: [32]u8 = [0; 32];
    const int_len: usize = spfFormatUintRadix(int_part, 10, false, int_buf[0..32]);
    let blen: usize = spfPutBytes(body[0..96], 0, int_buf[0..int_len]);

    if (p_clamped > 0) {
        blen = spfPutByte(body[0..96], blen, 0x2E);
        let frac_buf: [32]u8 = [0; 32];
        const flen: usize = spfFormatUintRadix(frac_int, 10, false, frac_buf[0..32]);
        if (flen < p_clamped) {
            blen = spfPutRepeat(body[0..96], blen, 0x30, p_clamped - flen);
        }
        blen = spfPutBytes(body[0..96], blen, frac_buf[0..flen]);
    }

    return spfEmitPadded(out, pos, sign, body[0..blen], width, left_align, zero_pad);
}

/// Formats `fmt` with `args` into `out`, returning the number of bytes written.
/// Supports `%%`, `%s`, `%d`, `%i`, `%u`, `%x`, `%X`, `%o`, `%b`, `%f`, `%c`
/// with flags (`-`, `+`, `0`), minimum width (`N`), and precision (`.N`).
pub fn sprintf(fmt: string, args: []const FmtArg, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    let arg_idx: usize = 0;

    while (i < fmt.len) {
        if (fmt[i] != 0x25) {
            pos = spfPutByte(out, pos, fmt[i]);
            i += 1;
            continue;
        }
        i += 1;
        if (i >= fmt.len) {
            break;
        }
        if (fmt[i] == 0x25) {
            pos = spfPutByte(out, pos, 0x25);
            i += 1;
            continue;
        }

        // Parse flags
        let left_align: bool = false;
        let plus_sign: bool = false;
        let zero_pad: bool = false;
        while (i < fmt.len) {
            const c: u8 = fmt[i];
            if (c == 0x2D) {
                left_align = true;
                i += 1;
            } else if (c == 0x2B) {
                plus_sign = true;
                i += 1;
            } else if (c == 0x30) {
                zero_pad = true;
                i += 1;
            } else {
                break;
            }
        }

        // Parse width
        let width: usize = 0;
        while (i < fmt.len && fmt[i] >= 0x30 && fmt[i] <= 0x39) {
            width = width * 10 + usize(fmt[i] - 0x30);
            i += 1;
        }

        // Parse precision
        let has_prec: bool = false;
        let prec: usize = 0;
        if (i < fmt.len && fmt[i] == 0x2E) {
            has_prec = true;
            i += 1;
            while (i < fmt.len && fmt[i] >= 0x30 && fmt[i] <= 0x39) {
                prec = prec * 10 + usize(fmt[i] - 0x30);
                i += 1;
            }
        }

        if (i >= fmt.len) {
            break;
        }
        const spec: u8 = fmt[i];
        i += 1;

        if (arg_idx >= args.len) {
            continue;
        }
        const arg: FmtArg = args[arg_idx];
        arg_idx += 1;

        if (spec == 0x73) {
            // %s
            let num_buf: [72]u8 = [0; 72];
            let s_slice: []const u8 = "";
            if (arg.kind == FmtArgKind.Str) {
                s_slice = arg.str_val;
            } else if (arg.kind == FmtArgKind.Bool) {
                s_slice = if (arg.bool_val) "true" else "false";
            } else if (arg.kind == FmtArgKind.Char) {
                num_buf[0] = arg.char_val;
                s_slice = num_buf[0..1];
            } else if (arg.kind == FmtArgKind.Uint) {
                const n: usize = spfFormatUintRadix(arg.uint_val, 10, false, num_buf[0..72]);
                s_slice = num_buf[0..n];
            } else if (arg.kind == FmtArgKind.Int) {
                let bpos: usize = 0;
                let mag: u64 = 0;
                if (arg.int_val < 0) {
                    num_buf[0] = 0x2D;
                    bpos = 1;
                    mag = u64(-(arg.int_val + 1)) + 1;
                } else {
                    mag = u64(arg.int_val);
                }
                const n: usize = spfFormatUintRadix(mag, 10, false, num_buf[bpos..72]);
                s_slice = num_buf[0..bpos + n];
            }
            if (has_prec && prec < s_slice.len) {
                s_slice = s_slice[0..prec];
            }
            pos = spfEmitPadded(out, pos, 0, s_slice, width, left_align, false);
        } else if (spec == 0x64 || spec == 0x69) {
            // %d / %i
            const v: i64 = spfArgAsSigned(arg);
            let sign: u8 = 0;
            let mag: u64 = 0;
            if (v < 0) {
                sign = 0x2D;
                mag = u64(-(v + 1)) + 1;
            } else {
                if (plus_sign) {
                    sign = 0x2B;
                }
                mag = u64(v);
            }
            let num_buf: [72]u8 = [0; 72];
            const n: usize = spfFormatUintRadix(mag, 10, false, num_buf[0..72]);
            pos = spfEmitPadded(out, pos, sign, num_buf[0..n], width, left_align, zero_pad);
        } else if (spec == 0x75) {
            // %u
            const v: u64 = spfArgAsUnsigned(arg);
            let num_buf: [72]u8 = [0; 72];
            const n: usize = spfFormatUintRadix(v, 10, false, num_buf[0..72]);
            pos = spfEmitPadded(out, pos, 0, num_buf[0..n], width, left_align, zero_pad);
        } else if (spec == 0x78 || spec == 0x58) {
            // %x / %X
            const v: u64 = spfArgAsUnsigned(arg);
            let num_buf: [72]u8 = [0; 72];
            const n: usize = spfFormatUintRadix(v, 16, spec == 0x58, num_buf[0..72]);
            pos = spfEmitPadded(out, pos, 0, num_buf[0..n], width, left_align, zero_pad);
        } else if (spec == 0x6F) {
            // %o
            const v: u64 = spfArgAsUnsigned(arg);
            let num_buf: [72]u8 = [0; 72];
            const n: usize = spfFormatUintRadix(v, 8, false, num_buf[0..72]);
            pos = spfEmitPadded(out, pos, 0, num_buf[0..n], width, left_align, zero_pad);
        } else if (spec == 0x62) {
            // %b
            const v: u64 = spfArgAsUnsigned(arg);
            let num_buf: [72]u8 = [0; 72];
            const n: usize = spfFormatUintRadix(v, 2, false, num_buf[0..72]);
            pos = spfEmitPadded(out, pos, 0, num_buf[0..n], width, left_align, zero_pad);
        } else if (spec == 0x63) {
            // %c
            let ch_buf: [1]u8 = [0];
            if (arg.kind == FmtArgKind.Char) {
                ch_buf[0] = arg.char_val;
            } else {
                ch_buf[0] = u8(spfArgAsUnsigned(arg) & 0xFF);
            }
            pos = spfEmitPadded(out, pos, 0, ch_buf[0..1], width, left_align, false);
        } else if (spec == 0x66) {
            // %f
            const fv: f64 = spfArgAsFloat(arg);
            const fprec: usize = if (has_prec) prec else 6;
            pos = spfFormatFloat(fv, fprec, plus_sign, width, left_align, zero_pad, out, pos);
        }
    }

    return pos;
}
