// @cap/decamelize core — Convert a camelCase / PascalCase string into a separated lowercase string
// Clean-room MIT CP implementation of npm `decamelize`

fn dcIsUpper(c: u8): bool {
    return c >= 65 && c <= 90;
}

fn dcIsLower(c: u8): bool {
    return c >= 97 && c <= 122;
}

fn dcIsDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

fn dcToLower(c: u8): u8 {
    if (c >= 65 && c <= 90) { return c + 32; }
    return c;
}

fn dcPushSep(sep: string, out: []u8, pos: usize): usize {
    let p = pos;
    let j: usize = 0;
    while (j < sep.len && p < out.len) {
        out[p] = sep[j];
        p = p + 1;
        j = j + 1;
    }
    return p;
}

/// Converts a camelized string into a lowercased string with `separator` (e.g. `"_"` or `"-"`).
/// When `preserve_consecutive_uppercase` is `true`, runs of >= 2 uppercase letters (like `"URL"` in `"testURL"`)
/// remain uppercase (matching npm `decamelize(str, { separator, preserveConsecutiveUppercase })`).
pub fn decamelizeWithOptions(
    input: string,
    separator: string,
    preserve_consecutive_uppercase: bool,
    out: []u8
): usize {
    let pos: usize = 0;
    let i: usize = 0;
    let n = input.len;

    while (i < n) {
        let c = input[i];
        let prev_is_lower_or_digit = (i > 0) && (dcIsLower(input[i - 1]) || dcIsDigit(input[i - 1]));
        let next_is_lower = (i + 1 < n) && dcIsLower(input[i + 1]);
        let prev_is_upper = (i > 0) && dcIsUpper(input[i - 1]);
        let next_is_upper = (i + 1 < n) && dcIsUpper(input[i + 1]);

        if (dcIsUpper(c)) {
            if (prev_is_lower_or_digit || (prev_is_upper && next_is_lower)) {
                pos = dcPushSep(separator, out, pos);
            }
            let in_acronym = (prev_is_upper && !next_is_lower) || (!prev_is_upper && next_is_upper);
            let emit = if (preserve_consecutive_uppercase && in_acronym) { c } else { dcToLower(c) };
            if (pos < out.len) {
                out[pos] = emit;
            }
            pos = pos + 1;
        } else {
            if (pos < out.len) {
                out[pos] = c;
            }
            pos = pos + 1;
        }
        i = i + 1;
    }

    if (pos > out.len) { return out.len; }
    return pos;
}

/// Default `decamelize(input, separator)` with `preserve_consecutive_uppercase = false`.
pub fn decamelize(input: string, separator: string, out: []u8): usize {
    return decamelizeWithOptions(input, separator, false, out);
}

export {
    decamelizeWithOptions,
    decamelize,
};
