// @cap/toidentifier — Convert a string of words to a valid identifier
// Clean-room MIT CP implementation of npm `toidentifier` (90M+/wk, dependency of `http-errors`)
// Zero-heap, single-pass ASCII word tokenizer.

fn tidIsAlpha(c: u8): bool {
    return (c >= 65 && c <= 90) || (c >= 97 && c <= 122);
}

fn tidIsDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

fn tidIsIdentChar(c: u8): bool {
    return tidIsAlpha(c) || tidIsDigit(c) || c == 95;
}

fn tidIsSpace(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13;
}

fn tidToUpper(c: u8): u8 {
    if (c >= 97 && c <= 122) { return c - 32; }
    return c;
}

fn tidToLower(c: u8): u8 {
    if (c >= 65 && c <= 90) { return c + 32; }
    return c;
}

/// Converts a phrase such as `"Bad Request"` or `"I'm a teapot"` into a
/// PascalCase identifier (`"BadRequest"`, `"ImATeapot"`), matching npm `toidentifier`.
/// Splits on whitespace, capitalizes the first character of each word, and strips
/// non-`[A-Za-z0-9_]` characters. Writes into `out` and returns written byte count.
pub fn toIdentifier(input: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    let at_word_start = true;
    while (i < input.len) {
        let c = input[i];
        if (tidIsSpace(c)) {
            at_word_start = true;
        } else {
            let ch = if (at_word_start) { tidToUpper(c) } else { c };
            at_word_start = false;
            if (tidIsIdentChar(ch)) {
                if (pos < out.len) {
                    out[pos] = ch;
                }
                pos = pos + 1;
            }
        }
        i = i + 1;
    }
    if (pos > out.len) { return out.len; }
    return pos;
}

/// Converts a phrase into a lowerCamelCase identifier (`"badRequest"`, `"imATeapot"`).
pub fn toCamelIdentifier(input: string, out: []u8): usize {
    let n = toIdentifier(input, out);
    if (n > 0) {
        out[0] = tidToLower(out[0]);
    }
    return n;
}

/// Converts a phrase into a SCREAMING_SNAKE_CASE identifier (`"BAD_REQUEST"`).
pub fn toConstIdentifier(input: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    let need_sep = false;
    while (i < input.len) {
        let c = input[i];
        if (tidIsSpace(c) || c == 45) {
            if (pos > 0) { need_sep = true; }
        } else if (tidIsIdentChar(c)) {
            if (need_sep) {
                if (pos < out.len) { out[pos] = 95; }
                pos = pos + 1;
                need_sep = false;
            }
            if (pos < out.len) { out[pos] = tidToUpper(c); }
            pos = pos + 1;
        }
        i = i + 1;
    }
    if (pos > out.len) { return out.len; }
    return pos;
}

/// Returns true if `s` is a valid non-empty ASCII identifier (`^[A-Za-z_][A-Za-z0-9_]*$`).
pub fn isValidIdentifier(s: string): bool {
    if (s.len == 0) { return false; }
    if (!(tidIsAlpha(s[0]) || s[0] == 95)) { return false; }
    let i: usize = 1;
    while (i < s.len) {
        if (!tidIsIdentChar(s[i])) { return false; }
        i = i + 1;
    }
    return true;
}
