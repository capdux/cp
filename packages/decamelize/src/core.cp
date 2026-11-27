// decamelize core — Convert a camelCase / PascalCase string into a separated lowercase string
// Clean-room MIT CP implementation of npm `decamelize`

import { isDigit, toLowerByte, toUpperByte } from "cap:ascii";

pub struct DecamelizeOptions {
    pub separator: string,
    pub preserve_consecutive_uppercase: bool,

    pub static fn defaults(): DecamelizeOptions {
        return DecamelizeOptions {
            separator: "_",
            preserve_consecutive_uppercase: false,
        };
    }
}

pub fn dcIsUpper(c: u8): bool {
    return c >= 65 && c <= 90;
}

pub fn dcIsLower(c: u8): bool {
    return c >= 97 && c <= 122;
}

pub fn dcIsDigit(c: u8): bool {
    return isDigit(c);
}

pub fn dcToLower(c: u8): u8 {
    return toLowerByte(c);
}

pub fn dcPushSep(sep: string, out: []u8, pos: usize): usize {
    let p = pos;
    let j: usize = 0;
    while (j < sep.len && p < out.len) {
        out[p] = sep[j];
        p = p + 1;
        j = j + 1;
    }
    return p;
}

/// Computes the exact byte length produced by `decamelizeWithOptions`.
pub fn decamelizedLen(
    input: string,
    separator: string,
    preserve_consecutive_uppercase: bool
): usize {
    let _unused = preserve_consecutive_uppercase;
    let total: usize = 0;
    let i: usize = 0;
    let n = input.len;

    while (i < n) {
        let c = input[i];
        let prev_is_lower_or_digit = (i > 0) && (dcIsLower(input[i - 1]) || isDigit(input[i - 1]));
        let next_is_lower = (i + 1 < n) && dcIsLower(input[i + 1]);
        let prev_is_upper = (i > 0) && dcIsUpper(input[i - 1]);

        if (dcIsUpper(c)) {
            if (prev_is_lower_or_digit || (prev_is_upper && next_is_lower)) {
                total = total + separator.len;
            }
        }
        total = total + 1;
        i = i + 1;
    }
    return total;
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
        let prev_is_lower_or_digit = (i > 0) && (dcIsLower(input[i - 1]) || isDigit(input[i - 1]));
        let next_is_lower = (i + 1 < n) && dcIsLower(input[i + 1]);
        let prev_is_upper = (i > 0) && dcIsUpper(input[i - 1]);
        let next_is_upper = (i + 1 < n) && dcIsUpper(input[i + 1]);

        if (dcIsUpper(c)) {
            if (prev_is_lower_or_digit || (prev_is_upper && next_is_lower)) {
                pos = dcPushSep(separator, out, pos);
            }
            let in_acronym = (prev_is_upper && !next_is_lower) || (!prev_is_upper && next_is_upper);
            let emit = if (preserve_consecutive_uppercase && in_acronym) { c } else { toLowerByte(c) };
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

/// Converts `input` using a structured `DecamelizeOptions` value.
pub fn decamelizeOpts(input: string, opts: DecamelizeOptions, out: []u8): usize {
    return decamelizeWithOptions(input, opts.separator, opts.preserve_consecutive_uppercase, out);
}

/// Default `decamelize(input, separator)` with `preserve_consecutive_uppercase = false`.
pub fn decamelize(input: string, separator: string, out: []u8): usize {
    return decamelizeWithOptions(input, separator, false, out);
}

/// Converts camelCase/PascalCase to `UPPER_SNAKE_CASE` (`CONSTANT_CASE`).
pub fn toConstantCase(input: string, out: []u8): usize {
    const written = decamelizeWithOptions(input, "_", false, out);
    let i: usize = 0;
    while (i < written) : (i += 1) {
        out[i] = toUpperByte(out[i]);
    }
    return written;
}

export {
    DecamelizeOptions,
    dcIsUpper,
    dcIsLower,
    dcIsDigit,
    dcToLower,
    dcPushSep,
    decamelizedLen,
    decamelizeWithOptions,
    decamelizeOpts,
    decamelize,
    toConstantCase,
};
