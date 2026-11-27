// escape-string-regexp — Escape RegExp special characters
// Clean-room MIT CP implementation of npm `escape-string-regexp` (350M+/wk)
// Zero-heap, single-pass scanner; escapes `[|\\{}()[\]^$+*?.]` with `\` and `-` as `\x2d`.
// Accelerated with comptime 256-entry LUT, 16-lane @Vector(16, u8) SIMD scanning, and @memcpy.

type EsrVec16 = @Vector(16, u8);

fn esrBuildClassLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    lut[124] = 1; // '|'
    lut[92] = 1;  // '\'
    lut[123] = 1; // '{'
    lut[125] = 1; // '}'
    lut[40] = 1;  // '('
    lut[41] = 1;  // ')'
    lut[91] = 1;  // '['
    lut[93] = 1;  // ']'
    lut[94] = 1;  // '^'
    lut[36] = 1;  // '$'
    lut[43] = 1;  // '+'
    lut[42] = 1;  // '*'
    lut[63] = 1;  // '?'
    lut[46] = 1;  // '.'
    lut[45] = 2;  // '-'
    return lut;
}

const ESR_CLASS_LUT: [256]u8 = esrBuildClassLut();

fn esrFindBackslash(s: string, start: usize): usize {
    let i: usize = start;
    const len = s.len;
    const v_bs: EsrVec16 = @splat(92 as u8);
    while (i + 16 <= len) {
        const v: EsrVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_bs)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == 92) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (s[i] == 92) {
            return i;
        }
        i += 1;
    }
    return len;
}

fn esrFindCharClassMeta(s: string, start: usize): usize {
    let i: usize = start;
    const len = s.len;
    const v_dash: EsrVec16 = @splat(45 as u8);
    const v_bs: EsrVec16 = @splat(92 as u8);
    const v_rb: EsrVec16 = @splat(93 as u8);
    const v_ct: EsrVec16 = @splat(94 as u8);
    while (i + 16 <= len) {
        const v: EsrVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_dash) ||
            @reduce(.Or, v == v_bs) ||
            @reduce(.Or, v == v_rb) ||
            @reduce(.Or, v == v_ct))
        {
            let k: usize = 0;
            while (k < 16) {
                const c = s[i + k];
                if (c == 45 || c == 92 || c == 93 || c == 94) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const c = s[i];
        if (c == 45 || c == 92 || c == 93 || c == 94) {
            return i;
        }
        i += 1;
    }
    return len;
}

pub fn esrIsSpecial(c: u8): bool {
    return ESR_CLASS_LUT[c as usize] == 1;
}

/// Returns true if byte `c` is any RegExp metacharacter (`|\\{}()[\]^$+*?.`) or `-`.
pub fn isRegexpMeta(c: u8): bool {
    return ESR_CLASS_LUT[c as usize] != 0;
}

/// Returns true if `input` contains any RegExp metacharacter or `-`.
pub fn hasRegexpMeta(input: string): bool {
    let i: usize = 0;
    while (i < input.len) {
        if (ESR_CLASS_LUT[input[i] as usize] != 0) { return true; }
        i = i + 1;
    }
    return false;
}

/// Alias for `hasRegexpMeta`: returns true if `input` requires regex escaping.
pub fn needsRegexEscape(input: string): bool {
    return hasRegexpMeta(input);
}

/// Computes the exact byte length required by `escapeStringRegexp(input, out)`.
pub fn escapedRegexpLen(input: string): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const cls = ESR_CLASS_LUT[input[i] as usize];
        if (cls == 0) {
            total = total + 1;
        } else if (cls == 1) {
            total = total + 2; // `\c`
        } else {
            total = total + 4; // `\x2d`
        }
        i = i + 1;
    }
    return total;
}

/// Alias for `escapedRegexpLen`: computes the exact escaped byte length.
pub fn escapedLen(input: string): usize {
    return escapedRegexpLen(input);
}

/// Escapes RegExp special characters matching npm `escape-string-regexp` v5:
/// - `[|\\{}()[\]^$+*?.]` -> `\<ch>`
/// - `-` -> `\x2d` (safe inside character classes too)
/// Writes into `out` and returns written byte count.
pub fn escapeStringRegexp(input: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        let j = i;
        while (j < input.len && ESR_CLASS_LUT[input[j] as usize] == 0) {
            j = j + 1;
        }
        if (j > i) {
            const seg_len = j - i;
            const avail = out.len - pos;
            if (seg_len > avail) {
                if (avail > 0) {
                    @memcpy(out[pos .. pos + avail], input[i .. i + avail]);
                    pos = pos + avail;
                }
                break;
            }
            @memcpy(out[pos .. pos + seg_len], input[i..j]);
            pos = pos + seg_len;
            i = j;
            if (i >= input.len) { break; }
        }
        const c = input[i];
        const cls = ESR_CLASS_LUT[c as usize];
        if (cls == 2) {
            if (pos + 4 > out.len) { break; }
            out[pos] = 92;      // '\'
            out[pos + 1] = 120; // 'x'
            out[pos + 2] = 50;  // '2'
            out[pos + 3] = 100; // 'd'
            pos = pos + 4;
        } else {
            if (pos + 2 > out.len) { break; }
            out[pos] = 92;
            out[pos + 1] = c;
            pos = pos + 2;
        }
        i = i + 1;
    }
    return pos;
}

/// Checked variant of `escapeStringRegexp`: returns `null` if `out` is too small
/// instead of silently truncating.
pub fn escapeStringRegexpChecked(input: string, out: []u8): usize? {
    const needed = escapedRegexpLen(input);
    if (out.len < needed) {
        return null;
    }
    return escapeStringRegexp(input, out);
}

/// Escapes characters that have special meaning inside a RegExp character class `[...]`:
/// `\`, `]`, `^`, and `-` (as `\x2d`).
pub fn escapeCharClass(input: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const next_meta = esrFindCharClassMeta(input, i);
        if (next_meta > i) {
            const seg_len = next_meta - i;
            const avail = out.len - pos;
            if (seg_len > avail) {
                if (avail > 0) {
                    @memcpy(out[pos .. pos + avail], input[i .. i + avail]);
                    pos = pos + avail;
                }
                break;
            }
            @memcpy(out[pos .. pos + seg_len], input[i..next_meta]);
            pos = pos + seg_len;
            i = next_meta;
            if (i >= input.len) { break; }
        }
        const c = input[i];
        if (c == 45) {
            if (pos + 4 > out.len) { break; }
            out[pos] = 92;
            out[pos + 1] = 120;
            out[pos + 2] = 50;
            out[pos + 3] = 100;
            pos = pos + 4;
        } else {
            if (pos + 2 > out.len) { break; }
            out[pos] = 92;
            out[pos + 1] = c;
            pos = pos + 2;
        }
        i = i + 1;
    }
    return pos;
}

fn isHex2d(escaped: string, i: usize): bool {
    return i + 3 < escaped.len
        && (escaped[i + 1] == 120 || escaped[i + 1] == 88) // 'x' or 'X'
        && escaped[i + 2] == 50                            // '2'
        && (escaped[i + 3] == 100 || escaped[i + 3] == 68); // 'd' or 'D'
}

/// Computes the exact byte length produced by `unescapeStringRegexp(escaped, out)`.
pub fn unescapedLen(escaped: string): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < escaped.len) {
        const next_bs = esrFindBackslash(escaped, i);
        total = total + (next_bs - i);
        i = next_bs;
        if (i >= escaped.len) { break; }
        if (i + 1 < escaped.len) {
            if (isHex2d(escaped, i)) {
                total = total + 1;
                i = i + 4;
            } else if (ESR_CLASS_LUT[escaped[i + 1] as usize] != 0) {
                total = total + 1;
                i = i + 2;
            } else {
                total = total + 1;
                i = i + 1;
            }
        } else {
            total = total + 1;
            i = i + 1;
        }
    }
    return total;
}

/// Reverses `escapeStringRegexp` escaping back to the original literal string.
pub fn unescapeStringRegexp(escaped: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < escaped.len && pos < out.len) {
        const next_bs = esrFindBackslash(escaped, i);
        if (next_bs > i) {
            const seg_len = next_bs - i;
            const avail = out.len - pos;
            const n = if (seg_len <= avail) { seg_len } else { avail };
            @memcpy(out[pos .. pos + n], escaped[i .. i + n]);
            pos = pos + n;
            i = i + n;
            if (i >= escaped.len || pos >= out.len) { break; }
        }
        // escaped[i] == '\\'
        if (i + 1 < escaped.len) {
            if (isHex2d(escaped, i)) {
                out[pos] = 45; // '-'
                pos = pos + 1;
                i = i + 4;
            } else if (ESR_CLASS_LUT[escaped[i + 1] as usize] != 0) {
                out[pos] = escaped[i + 1];
                pos = pos + 1;
                i = i + 2;
            } else {
                out[pos] = 92;
                pos = pos + 1;
                i = i + 1;
            }
        } else {
            out[pos] = 92;
            pos = pos + 1;
            i = i + 1;
        }
    }
    return pos;
}

/// Alias for `unescapeStringRegexp`.
pub fn unescapeRegexp(escaped: string, out: []u8): usize {
    return unescapeStringRegexp(escaped, out);
}
