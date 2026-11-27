// escape-string-regexp — Escape RegExp special characters
// Clean-room MIT CP implementation of npm `escape-string-regexp` (350M+/wk)
// Zero-heap, single-pass scanner; escapes `[|\\{}()[\]^$+*?.]` with `\` and `-` as `\x2d`.

pub fn esrIsSpecial(c: u8): bool {
    return c == 124 || c == 92 || c == 123 || c == 125 || c == 40 || c == 41
        || c == 91 || c == 93 || c == 94 || c == 36 || c == 43 || c == 42
        || c == 63 || c == 46;
}

/// Returns true if byte `c` is any RegExp metacharacter (`|\\{}()[\]^$+*?.`) or `-`.
pub fn isRegexpMeta(c: u8): bool {
    return esrIsSpecial(c) || c == 45;
}

/// Returns true if `input` contains any RegExp metacharacter or `-`.
pub fn hasRegexpMeta(input: string): bool {
    let i: usize = 0;
    while (i < input.len) {
        if (isRegexpMeta(input[i])) { return true; }
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
        let c = input[i];
        if (c == 45) {
            total = total + 4; // `\x2d`
        } else if (esrIsSpecial(c)) {
            total = total + 2; // `\c`
        } else {
            total = total + 1;
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
        let c = input[i];
        if (c == 45) {
            if (pos + 4 > out.len) { break; }
            out[pos] = 92;      // '\'
            out[pos + 1] = 120; // 'x'
            out[pos + 2] = 50;  // '2'
            out[pos + 3] = 100; // 'd'
            pos = pos + 4;
        } else if (esrIsSpecial(c)) {
            if (pos + 2 > out.len) { break; }
            out[pos] = 92;
            out[pos + 1] = c;
            pos = pos + 2;
        } else {
            if (pos >= out.len) { break; }
            out[pos] = c;
            pos = pos + 1;
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
        const c = input[i];
        if (c == 45) {
            if (pos + 4 > out.len) { break; }
            out[pos] = 92;
            out[pos + 1] = 120;
            out[pos + 2] = 50;
            out[pos + 3] = 100;
            pos = pos + 4;
        } else if (c == 92 || c == 93 || c == 94) {
            if (pos + 2 > out.len) { break; }
            out[pos] = 92;
            out[pos + 1] = c;
            pos = pos + 2;
        } else {
            if (pos >= out.len) { break; }
            out[pos] = c;
            pos = pos + 1;
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
        if (escaped[i] == 92 && i + 1 < escaped.len) {
            if (isHex2d(escaped, i)) {
                total = total + 1;
                i = i + 4;
            } else if (esrIsSpecial(escaped[i + 1]) || escaped[i + 1] == 45) {
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
        if (escaped[i] == 92 && i + 1 < escaped.len) {
            if (isHex2d(escaped, i)) {
                out[pos] = 45; // '-'
                pos = pos + 1;
                i = i + 4;
            } else if (esrIsSpecial(escaped[i + 1]) || escaped[i + 1] == 45) {
                out[pos] = escaped[i + 1];
                pos = pos + 1;
                i = i + 2;
            } else {
                out[pos] = escaped[i];
                pos = pos + 1;
                i = i + 1;
            }
        } else {
            out[pos] = escaped[i];
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
