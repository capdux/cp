// @cap/powerfmt — Zero-heap high-performance integer formatting in pure CP
// MIT License — Clean-room implementation inspired by Rust `powerfmt`

pub const ALIGN_RIGHT: u8 = 0;
pub const ALIGN_LEFT: u8 = 1;
pub const ALIGN_CENTER: u8 = 2;

const I64_MIN_ABS: u64 = (0x80000000 as u64) << 32;
const I64_MIN_PLUS_1: i64 = -9223372036854775807;

pub struct IntFmtSpec {
    pub min_width: usize,
    pub pad_byte: u8,
    pub alignment: u8,
    pub radix: u8,
    pub uppercase: bool,
    pub force_sign: bool,
    pub group_sep: u8,
    pub group_size: usize,

    /// Default decimal right-aligned space-padded format spec.
    pub static fn defaultSpec(): IntFmtSpec {
        return IntFmtSpec {
            min_width: 0,
            pad_byte: 32, // ' '
            alignment: ALIGN_RIGHT,
            radix: 10,
            uppercase: false,
            force_sign: false,
            group_sep: 0,
            group_size: 3,
        };
    }
}

fn digitByte(d: u64, uppercase: bool): u8 {
    if (d < 10) {
        return (d as u8) + 48;
    }
    const base: u8 = if (uppercase) 65 else 97; // 'A' or 'a'
    return ((d - 10) as u8) + base;
}

fn writeByte(out: []u8, pos: usize, b: u8): usize {
    if (pos >= out.len) {
        return pos;
    }
    out[pos] = b;
    return pos + 1;
}

fn writeRepeat(out: []u8, pos: usize, b: u8, count: usize): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < count && p < out.len) {
        out[p] = b;
        p += 1;
        i += 1;
    }
    return p;
}

fn formatMagnitude(mag: u64, sign_byte: u8, spec: IntFmtSpec, out: []u8): usize {
    if (out.len == 0) {
        return 0;
    }
    const radix: u64 = if (spec.radix == 2 || spec.radix == 8 || spec.radix == 10 || spec.radix == 16)
        spec.radix as u64
    else
        10;

    let rev: [128]u8 = undefined;
    let rev_len: usize = 0;
    let digits_in_group: usize = 0;
    const use_group: bool = spec.group_sep != 0 && spec.group_size > 0;

    if (mag == 0) {
        rev[0] = 48;
        rev_len = 1;
    } else {
        let v: u64 = mag;
        while (v > 0) {
            if (use_group && digits_in_group == spec.group_size) {
                rev[rev_len] = spec.group_sep;
                rev_len += 1;
                digits_in_group = 0;
            }
            const d: u64 = v % radix;
            rev[rev_len] = digitByte(d, spec.uppercase);
            rev_len += 1;
            digits_in_group += 1;
            v = v / radix;
        }
    }

    const has_sign: bool = sign_byte != 0;
    const sign_len: usize = if (has_sign) 1 else 0;
    const content_len: usize = rev_len + sign_len;
    const pad_count: usize = if (spec.min_width > content_len) spec.min_width - content_len else 0;
    const pad_ch: u8 = if (spec.pad_byte == 0) 32 else spec.pad_byte;

    let p: usize = 0;

    if (spec.alignment == ALIGN_LEFT) {
        if (has_sign) {
            p = writeByte(out, p, sign_byte);
        }
        let k: usize = rev_len;
        while (k > 0 && p < out.len) {
            k -= 1;
            p = writeByte(out, p, rev[k]);
        }
        p = writeRepeat(out, p, pad_ch, pad_count);
        return p;
    }

    if (spec.alignment == ALIGN_CENTER) {
        const left_pad: usize = pad_count / 2;
        const right_pad: usize = pad_count - left_pad;
        p = writeRepeat(out, p, pad_ch, left_pad);
        if (has_sign) {
            p = writeByte(out, p, sign_byte);
        }
        let k: usize = rev_len;
        while (k > 0 && p < out.len) {
            k -= 1;
            p = writeByte(out, p, rev[k]);
        }
        p = writeRepeat(out, p, pad_ch, right_pad);
        return p;
    }

    // ALIGN_RIGHT: zero-padding places sign before zeros ("-0042"); other pad places sign next to digits ("  -42")
    if (pad_ch == 48) {
        if (has_sign) {
            p = writeByte(out, p, sign_byte);
        }
        p = writeRepeat(out, p, pad_ch, pad_count);
    } else {
        p = writeRepeat(out, p, pad_ch, pad_count);
        if (has_sign) {
            p = writeByte(out, p, sign_byte);
        }
    }

    let k: usize = rev_len;
    while (k > 0 && p < out.len) {
        k -= 1;
        p = writeByte(out, p, rev[k]);
    }
    return p;
}

/// Format an unsigned 64-bit integer according to `spec` into `out`.
pub fn formatU64(val: u64, spec: IntFmtSpec, out: []u8): usize {
    const sign_byte: u8 = if (spec.force_sign) 43 else 0; // '+'
    return formatMagnitude(val, sign_byte, spec, out);
}

/// Format a signed 64-bit integer according to `spec` into `out`.
/// Safely handles `i64::MIN` (-9223372036854775808) without overflow.
pub fn formatI64(val: i64, spec: IntFmtSpec, out: []u8): usize {
    if (val < I64_MIN_PLUS_1) {
        return formatMagnitude(I64_MIN_ABS, 45, spec, out); // '-'
    }
    if (val < 0) {
        return formatMagnitude((-val) as u64, 45, spec, out);
    }
    const sign_byte: u8 = if (spec.force_sign) 43 else 0;
    return formatMagnitude(val as u64, sign_byte, spec, out);
}

/// Format a signed integer in base-10, zero-padded on the left to `width`.
pub fn formatDecPadded(val: i64, width: usize, out: []u8): usize {
    let spec: IntFmtSpec = IntFmtSpec.defaultSpec();
    spec.min_width = width;
    spec.pad_byte = 48; // '0'
    spec.alignment = ALIGN_RIGHT;
    return formatI64(val, spec, out);
}

/// Format an unsigned integer in hexadecimal (lowercase or uppercase).
pub fn formatHex(val: u64, uppercase: bool, out: []u8): usize {
    let spec: IntFmtSpec = IntFmtSpec.defaultSpec();
    spec.radix = 16;
    spec.uppercase = uppercase;
    return formatU64(val, spec, out);
}

/// Format a signed integer in base-10 with thousands comma separators (e.g. `"1,234,567"`, `"-1,000"`).
pub fn formatWithCommas(val: i64, out: []u8): usize {
    let spec: IntFmtSpec = IntFmtSpec.defaultSpec();
    spec.group_sep = 44; // ','
    spec.group_size = 3;
    return formatI64(val, spec, out);
}
