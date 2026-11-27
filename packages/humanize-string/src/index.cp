// @cap/humanize-string — Zero-allocation humanized sentence converter for CP
// Clean-room MIT implementation with parity to npm `humanize-string` v3.
// Composes `@cap/decamelize` with zero duplication.

import { decamelize } from "../../decamelize/src/index.cp";

const HSTR_SCRATCH_CAP: usize = 8192;

fn hstrIsSep(c: u8): bool {
    return c == '_' || c == '-' || c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0x0b || c == 0x0c;
}

fn hstrToUpper(c: u8): u8 {
    if (c >= 'a' && c <= 'z') {
        return c - 32;
    }
    return c;
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
    while (start < end && hstrIsSep(dec_slice[start])) {
        start += 1;
    }
    while (end > start && hstrIsSep(dec_slice[end - 1])) {
        end -= 1;
    }
    if (start >= end) {
        return 0;
    }

    let pos: usize = 0;
    let prev_sep = false;
    let i = start;
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
                out[pos] = hstrToUpper(c);
            } else {
                out[pos] = c;
            }
            pos += 1;
            prev_sep = false;
        }
    }

    return pos;
}
