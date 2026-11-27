// @cap/pad-str — Zero-allocation UTF-8 Codepoint-Aware String Padding & Repeat Utilities
// Clean-room MIT implementation in pure CP.

/// Counts the number of UTF-8 Unicode scalar codepoints in `s`.
pub fn countChars(s: string) -> usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        // Count non-continuation UTF-8 bytes: (b & 0xC0) != 0x80
        if ((s[i] & 0xC0) != 0x80) {
            count += 1;
        }
        i += 1;
    }
    return count;
}

fn copyBytes(dst: []u8, pos: usize, src: string) {
    let i: usize = 0;
    while (i < src.len) {
        dst[pos + i] = src[i];
        i += 1;
    }
}

fn fillByte(dst: []u8, pos: usize, count: usize, ch: u8) {
    let i: usize = 0;
    while (i < count) {
        dst[pos + i] = ch;
        i += 1;
    }
}

/// Left-pads `s` with `pad_char` until it reaches `target_chars` UTF-8 codepoints.
/// Copies `s` verbatim if already `>= target_chars`. Returns bytes written, or `0` on overflow.
pub fn padStart(s: string, target_chars: usize, pad_char: u8, out: []u8) -> usize {
    const chars = countChars(s);
    let pad_n: usize = 0;
    if (target_chars > chars) {
        pad_n = target_chars - chars;
    }
    const needed = pad_n + s.len;
    if (needed > out.len) {
        return 0;
    }
    fillByte(out, 0, pad_n, pad_char);
    copyBytes(out, pad_n, s);
    return needed;
}

/// Right-pads `s` with `pad_char` until it reaches `target_chars` UTF-8 codepoints.
/// Copies `s` verbatim if already `>= target_chars`. Returns bytes written, or `0` on overflow.
pub fn padEnd(s: string, target_chars: usize, pad_char: u8, out: []u8) -> usize {
    const chars = countChars(s);
    let pad_n: usize = 0;
    if (target_chars > chars) {
        pad_n = target_chars - chars;
    }
    const needed = s.len + pad_n;
    if (needed > out.len) {
        return 0;
    }
    copyBytes(out, 0, s);
    fillByte(out, s.len, pad_n, pad_char);
    return needed;
}

/// Center-pads `s` with `pad_char` to `target_chars` UTF-8 codepoints.
/// When total padding is odd, the extra pad byte is placed on the right.
/// Returns bytes written, or `0` on overflow.
pub fn padCenter(s: string, target_chars: usize, pad_char: u8, out: []u8) -> usize {
    const chars = countChars(s);
    let pad_n: usize = 0;
    if (target_chars > chars) {
        pad_n = target_chars - chars;
    }
    const needed = pad_n + s.len;
    if (needed > out.len) {
        return 0;
    }
    const left_pad = pad_n / 2;
    const right_pad = pad_n - left_pad;
    fillByte(out, 0, left_pad, pad_char);
    copyBytes(out, left_pad, s);
    fillByte(out, left_pad + s.len, right_pad, pad_char);
    return needed;
}

/// Repeats `s` `count` times into `out`.
/// Returns total bytes written, or `0` if `s` is empty, `count == 0`, or `out` is too small.
pub fn repeatStr(s: string, count: usize, out: []u8) -> usize {
    if (s.len == 0 || count == 0) {
        return 0;
    }
    if (count > out.len / s.len) {
        return 0;
    }
    let w: usize = 0;
    let c: usize = 0;
    while (c < count) {
        copyBytes(out, w, s);
        w += s.len;
        c += 1;
    }
    return w;
}

/// Options for `pad` and `padCenterWith`, matching npm `pad` options (`char`, `strip`/truncate, direction).
pub struct PadOptions {
    pub pad_char: u8,
    pub pad_str: string,
    pub truncate: bool,
    pub left: bool,

    pub static fn defaults() -> PadOptions {
        return PadOptions {
            pad_char: 32,
            pad_str: "",
            truncate: false,
            left: false,
        };
    }
}

/// Zero-copy UTF-8 codepoint-aware truncation of `s` to at most `max_chars` Unicode scalar codepoints.
pub fn truncateChars(s: string, max_chars: usize) -> string {
    if (max_chars == 0 || s.len == 0) {
        return "";
    }
    let chars: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if ((s[i] & 0xC0) != 0x80) {
            if (chars == max_chars) {
                return s[0..i];
            }
            chars += 1;
        }
        i += 1;
    }
    return s;
}

fn calcPadBytes(pad_chars: usize, pad_char: u8, pad_str: string) -> usize {
    if (pad_chars == 0) {
        return 0;
    }
    if (pad_str.len == 0) {
        return pad_chars;
    }
    let total_bytes: usize = 0;
    let written_chars: usize = 0;
    let idx: usize = 0;
    while (written_chars < pad_chars) {
        // Advance one full UTF-8 codepoint in pad_str
        const start = idx;
        idx += 1;
        while (idx < pad_str.len && (pad_str[idx] & 0xC0) == 0x80) {
            idx += 1;
        }
        total_bytes += (idx - start);
        written_chars += 1;
        if (idx >= pad_str.len) {
            idx = 0;
        }
    }
    return total_bytes;
}

fn writePadPattern(dst: []u8, pos: usize, pad_chars: usize, pad_char: u8, pad_str: string) -> usize {
    if (pad_chars == 0) {
        return 0;
    }
    if (pad_str.len == 0) {
        fillByte(dst, pos, pad_chars, pad_char);
        return pad_chars;
    }
    let w: usize = 0;
    let written_chars: usize = 0;
    let idx: usize = 0;
    while (written_chars < pad_chars) {
        const start = idx;
        idx += 1;
        while (idx < pad_str.len && (pad_str[idx] & 0xC0) == 0x80) {
            idx += 1;
        }
        let b: usize = start;
        while (b < idx) {
            dst[pos + w] = pad_str[b];
            w += 1;
            b += 1;
        }
        written_chars += 1;
        if (idx >= pad_str.len) {
            idx = 0;
        }
    }
    return w;
}

/// Alias for `padStart`: left-pads `s` with `pad_char` until `target_chars` UTF-8 codepoints.
pub fn padLeft(s: string, target_chars: usize, pad_char: u8, out: []u8) -> usize {
    return padStart(s, target_chars, pad_char, out);
}

/// Alias for `padEnd`: right-pads `s` with `pad_char` until `target_chars` UTF-8 codepoints.
pub fn padRight(s: string, target_chars: usize, pad_char: u8, out: []u8) -> usize {
    return padEnd(s, target_chars, pad_char, out);
}

/// Pads `s` to `target_chars` UTF-8 codepoints according to `opts` (left/right, custom `pad_char` or `pad_str`, optional `truncate`).
/// Returns bytes written, or `0` if `out` is too small (or result is empty).
pub fn pad(s: string, target_chars: usize, opts: PadOptions, out: []u8) -> usize {
    const chars = countChars(s);
    if (chars > target_chars && opts.truncate) {
        const cut = truncateChars(s, target_chars);
        if (cut.len > out.len) {
            return 0;
        }
        copyBytes(out, 0, cut);
        return cut.len;
    }

    let pad_count: usize = 0;
    if (target_chars > chars) {
        pad_count = target_chars - chars;
    }
    const pad_bytes = calcPadBytes(pad_count, opts.pad_char, opts.pad_str);
    const needed = pad_bytes + s.len;
    if (needed > out.len) {
        return 0;
    }

    if (opts.left) {
        const pw = writePadPattern(out, 0, pad_count, opts.pad_char, opts.pad_str);
        copyBytes(out, pw, s);
        return pw + s.len;
    }
    copyBytes(out, 0, s);
    const pw = writePadPattern(out, s.len, pad_count, opts.pad_char, opts.pad_str);
    return s.len + pw;
}

/// Center-pads `s` to `target_chars` UTF-8 codepoints with full `PadOptions` (custom `pad_str`/`pad_char` and `truncate`).
pub fn padCenterWith(s: string, target_chars: usize, opts: PadOptions, out: []u8) -> usize {
    const chars = countChars(s);
    if (chars > target_chars && opts.truncate) {
        const cut = truncateChars(s, target_chars);
        if (cut.len > out.len) {
            return 0;
        }
        copyBytes(out, 0, cut);
        return cut.len;
    }

    let pad_count: usize = 0;
    if (target_chars > chars) {
        pad_count = target_chars - chars;
    }
    const left_pad = pad_count / 2;
    const right_pad = pad_count - left_pad;
    const left_bytes = calcPadBytes(left_pad, opts.pad_char, opts.pad_str);
    const right_bytes = calcPadBytes(right_pad, opts.pad_char, opts.pad_str);
    const needed = left_bytes + s.len + right_bytes;
    if (needed > out.len) {
        return 0;
    }
    const lw = writePadPattern(out, 0, left_pad, opts.pad_char, opts.pad_str);
    copyBytes(out, lw, s);
    const rw = writePadPattern(out, lw + s.len, right_pad, opts.pad_char, opts.pad_str);
    return lw + s.len + rw;
}
