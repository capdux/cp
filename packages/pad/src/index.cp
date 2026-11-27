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
    let pad: usize = 0;
    if (target_chars > chars) {
        pad = target_chars - chars;
    }
    const needed = pad + s.len;
    if (needed > out.len) {
        return 0;
    }
    fillByte(out, 0, pad, pad_char);
    copyBytes(out, pad, s);
    return needed;
}

/// Right-pads `s` with `pad_char` until it reaches `target_chars` UTF-8 codepoints.
/// Copies `s` verbatim if already `>= target_chars`. Returns bytes written, or `0` on overflow.
pub fn padEnd(s: string, target_chars: usize, pad_char: u8, out: []u8) -> usize {
    const chars = countChars(s);
    let pad: usize = 0;
    if (target_chars > chars) {
        pad = target_chars - chars;
    }
    const needed = s.len + pad;
    if (needed > out.len) {
        return 0;
    }
    copyBytes(out, 0, s);
    fillByte(out, s.len, pad, pad_char);
    return needed;
}

/// Center-pads `s` with `pad_char` to `target_chars` UTF-8 codepoints.
/// When total padding is odd, the extra pad byte is placed on the right.
/// Returns bytes written, or `0` on overflow.
pub fn padCenter(s: string, target_chars: usize, pad_char: u8, out: []u8) -> usize {
    const chars = countChars(s);
    let pad: usize = 0;
    if (target_chars > chars) {
        pad = target_chars - chars;
    }
    const needed = pad + s.len;
    if (needed > out.len) {
        return 0;
    }
    const left_pad = pad / 2;
    const right_pad = pad - left_pad;
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
