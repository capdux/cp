// @cap/humanize-string — Zero-allocation humanized sentence converter for CP
// Clean-room MIT implementation with parity to npm `humanize-string` v3.
// Reuses `cap:ascii` and `@cap/decamelize` with zero heap allocation.

import { isSpace, toUpperByte, toLowerByte } from "cap:ascii";
import { decamelize } from "../../decamelize/src/index.cp";

const HSTR_SCRATCH_CAP: usize = 8192;

/// Options controlling `humanizeStringWithOptions`.
pub struct HumanizeOptions {
    /// Uppercase the first ASCII letter of the output (default: `true`).
    pub capitalize_first: bool,
    /// Preserve consecutive all-uppercase acronyms (e.g. `HTTP`, `URL`, `API`) instead of lowercasing (default: `false`).
    pub preserve_acronyms: bool,

    pub static fn defaults(): HumanizeOptions {
        return HumanizeOptions {
            capitalize_first: true,
            preserve_acronyms: false,
        };
    }
}

fn hstrIsSep(c: u8): bool {
    return c == '_' || c == '-' || isSpace(c);
}

fn hstrIsUpper(c: u8): bool {
    return c >= 'A' && c <= 'Z';
}

fn hstrIsLower(c: u8): bool {
    return c >= 'a' && c <= 'z';
}

fn hstrIsDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

/// Converts a camelized, dasherized, underscored, or whitespace-separated string
/// into a humanized string: splits camelCase via `@cap/decamelize`, collapses
/// `_`, `-`, and whitespace runs into single spaces, trims ends, and uppercases
/// the first character. Writes into `out` and returns the number of bytes written.
pub fn humanizeString(input: string, out: []u8): usize {
    if (input.len == 0 || out.len == 0) {
        return 0;
    }

    let scratch: [HSTR_SCRATCH_CAP]u8 = undefined;
    const dec_len = decamelize(input, " ", scratch[0..HSTR_SCRATCH_CAP]);
    const dec_slice = scratch[0..dec_len];

    // Trim leading and trailing separators
    let start: usize = 0;
    let end: usize = dec_slice.len;
    while (start < end && hstrIsSep(dec_slice[start])) : (start += 1) {}
    while (end > start && hstrIsSep(dec_slice[end - 1])) : (end -= 1) {}
    if (start >= end) {
        return 0;
    }

    let pos: usize = 0;
    let prev_sep: bool = false;
    let i: usize = start;
    while (i < end && pos < out.len) : (i += 1) {
        const c = dec_slice[i];
        if (hstrIsSep(c)) {
            if (!prev_sep) {
                out[pos] = ' ';
                pos += 1;
                prev_sep = true;
            }
        } else {
            if (pos == 0) {
                out[pos] = toUpperByte(c);
            } else {
                out[pos] = c;
            }
            pos += 1;
            prev_sep = false;
        }
    }

    return pos;
}

/// Streaming zero-buffer humanizer with configurable `HumanizeOptions` (supports acronym preservation
/// and arbitrary-length inputs without fixed scratch cap).
pub fn humanizeStringWithOptions(input: string, opts: HumanizeOptions, out: []u8): usize {
    if (input.len == 0 || out.len == 0) {
        return 0;
    }
    if (!opts.preserve_acronyms) {
        const n = humanizeString(input, out);
        if (!opts.capitalize_first && n > 0) {
            out[0] = toLowerByte(out[0]);
        }
        return n;
    }

    // Trim leading and trailing separators on raw input
    let start: usize = 0;
    let end: usize = input.len;
    while (start < end && hstrIsSep(input[start])) : (start += 1) {}
    while (end > start && hstrIsSep(input[end - 1])) : (end -= 1) {}
    if (start >= end) {
        return 0;
    }

    let pos: usize = 0;
    let prev_sep: bool = false;
    let i: usize = start;
    while (i < end && pos < out.len) : (i += 1) {
        const c = input[i];
        if (hstrIsSep(c)) {
            if (!prev_sep && pos > 0) {
                out[pos] = ' ';
                pos += 1;
                prev_sep = true;
            }
            continue;
        }

        // Detect camelCase / acronym boundary before `c`
        if (pos > 0 && !prev_sep && hstrIsUpper(c)) {
            const prev = input[i - 1];
            const prev_is_lower_or_digit = hstrIsLower(prev) || hstrIsDigit(prev);
            const next_is_lower = (i + 1 < end) && hstrIsLower(input[i + 1]);
            const acronym_end = hstrIsUpper(prev) && next_is_lower;
            if (prev_is_lower_or_digit || acronym_end) {
                out[pos] = ' ';
                pos += 1;
                if (pos >= out.len) {
                    break;
                }
            }
        }

        // Determine whether `c` is part of an acronym (>=2 uppercase letters in a row,
        // excluding the last uppercase before a lowercase letter like `D` in `HTMLDoc`)
        let keep_upper: bool = false;
        if (hstrIsUpper(c)) {
            const prev_up = (i > start) && hstrIsUpper(input[i - 1]);
            const next_up = (i + 1 < end) && hstrIsUpper(input[i + 1]);
            const next_lo = (i + 1 < end) && hstrIsLower(input[i + 1]);
            if (next_up || (prev_up && !next_lo)) {
                keep_upper = true;
            }
        }

        if (pos == 0) {
            out[pos] = if (opts.capitalize_first || keep_upper) toUpperByte(c) else toLowerByte(c);
        } else if (keep_upper) {
            out[pos] = c;
        } else {
            out[pos] = toLowerByte(c);
        }
        pos += 1;
        prev_sep = false;
    }

    return pos;
}

/// Humanizes `input` and capitalizes the first ASCII letter of every word (Title Case).
pub fn titleizeString(input: string, out: []u8): usize {
    const n = humanizeString(input, out);
    let i: usize = 0;
    let at_word_start: bool = true;
    while (i < n) : (i += 1) {
        if (out[i] == ' ') {
            at_word_start = true;
        } else if (at_word_start) {
            out[i] = toUpperByte(out[i]);
            at_word_start = false;
        }
    }
    return n;
}

/// Computes the exact humanized byte length of `input` without requiring a caller output buffer.
pub fn humanizeLength(input: string): usize {
    if (input.len == 0) {
        return 0;
    }
    let scratch: [HSTR_SCRATCH_CAP]u8 = undefined;
    const dec_len = decamelize(input, " ", scratch[0..HSTR_SCRATCH_CAP]);
    const dec_slice = scratch[0..dec_len];
    let start: usize = 0;
    let end: usize = dec_slice.len;
    while (start < end && hstrIsSep(dec_slice[start])) : (start += 1) {}
    while (end > start && hstrIsSep(dec_slice[end - 1])) : (end -= 1) {}
    if (start >= end) {
        return 0;
    }
    let count: usize = 0;
    let prev_sep: bool = false;
    let i: usize = start;
    while (i < end) : (i += 1) {
        if (hstrIsSep(dec_slice[i])) {
            if (!prev_sep) {
                count += 1;
                prev_sep = true;
            }
        } else {
            count += 1;
            prev_sep = false;
        }
    }
    return count;
}

export {
    HumanizeOptions,
    humanizeString,
    humanizeStringWithOptions,
    titleizeString,
    humanizeLength,
};
