// @cap/toidentifier — Convert a string of words to a valid JS/CP identifier
// Clean-room MIT CP implementation of npm `toidentifier` (90M+/wk, dependency of `http-errors`)
// Zero-heap, single-pass ASCII word tokenizer reusing `cap:ascii`.

import { isAlpha, isAlnum, isDigit, isSpace, toUpperByte, toLowerByte } from "cap:ascii";

fn tidIsIdentChar(c: u8): bool {
    return isAlnum(c) || c == 95;
}

/// Computes the exact byte length `toIdentifier` will emit for `input`.
pub fn toIdentifierLen(input: string): usize {
    let len: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        let c = input[i];
        if (!isSpace(c) && tidIsIdentChar(c)) {
            len = len + 1;
        }
        i = i + 1;
    }
    return len;
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
        if (isSpace(c)) {
            at_word_start = true;
        } else {
            let ch = if (at_word_start) { toUpperByte(c) } else { c };
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

/// Checked variant of `toIdentifier`: returns `null` if `out` is too small.
pub fn toIdentifierChecked(input: string, out: []u8): usize? {
    let req = toIdentifierLen(input);
    if (req > out.len) { return null; }
    return toIdentifier(input, out);
}

/// Converts any phrase into a guaranteed valid JS/CP identifier (`^[A-Za-z_][A-Za-z0-9_]*$`).
/// Prefixes `_` when the converted identifier would begin with a digit, and emits `"_"` for
/// empty or all-punctuation inputs. Returns written byte count, or 0 if `out` is empty.
pub fn toSafeIdentifier(input: string, out: []u8): usize {
    if (out.len == 0) { return 0; }
    let req = toIdentifierLen(input);
    if (req == 0) {
        out[0] = 95; // '_'
        return 1;
    }
    let n = toIdentifier(input, out);
    if (n > 0 && isDigit(out[0])) {
        // Shift right by 1 to prepend '_'
        let max_keep = if (n < out.len) { n } else { out.len - 1 };
        let k = max_keep;
        while (k > 0) {
            out[k] = out[k - 1];
            k = k - 1;
        }
        out[0] = 95;
        return max_keep + 1;
    }
    return n;
}

/// Converts a phrase into a lowerCamelCase identifier (`"badRequest"`, `"imATeapot"`).
pub fn toCamelIdentifier(input: string, out: []u8): usize {
    let n = toIdentifier(input, out);
    if (n > 0) {
        out[0] = toLowerByte(out[0]);
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
        if (isSpace(c) || c == 45) {
            if (pos > 0) { need_sep = true; }
        } else if (tidIsIdentChar(c)) {
            if (need_sep) {
                if (pos < out.len) { out[pos] = 95; }
                pos = pos + 1;
                need_sep = false;
            }
            if (pos < out.len) { out[pos] = toUpperByte(c); }
            pos = pos + 1;
        }
        i = i + 1;
    }
    if (pos > out.len) { return out.len; }
    return pos;
}

/// Converts a phrase into a `snake_case` identifier (`"bad_request"`).
pub fn toSnakeIdentifier(input: string, out: []u8): usize {
    let n = toConstIdentifier(input, out);
    let i: usize = 0;
    while (i < n) {
        out[i] = toLowerByte(out[i]);
        i = i + 1;
    }
    return n;
}

/// Returns true if `s` is a valid non-empty ASCII identifier (`^[A-Za-z_][A-Za-z0-9_]*$`).
pub fn isValidIdentifier(s: string): bool {
    if (s.len == 0) { return false; }
    if (!(isAlpha(s[0]) || s[0] == 95)) { return false; }
    let i: usize = 1;
    while (i < s.len) {
        if (!tidIsIdentChar(s[i])) { return false; }
        i = i + 1;
    }
    return true;
}

/// Returns true if `s` is a valid non-empty ASCII JavaScript identifier (`^[A-Za-z_$][A-Za-z0-9_$]*$`).
pub fn isValidJsIdentifier(s: string): bool {
    if (s.len == 0) { return false; }
    if (!(isAlpha(s[0]) || s[0] == 95 || s[0] == 36)) { return false; }
    let i: usize = 1;
    while (i < s.len) {
        let c = s[i];
        if (!(tidIsIdentChar(c) || c == 36)) { return false; }
        i = i + 1;
    }
    return true;
}
