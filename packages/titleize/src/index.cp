// @cap/titleize — Zero-allocation string title-case utility for CP
// Clean-room MIT implementation with parity to npm `titleize` v3.

import { isAlnum, toUpperByte, toLowerByte } from "cap:ascii";

fn titIsWordChar(c: u8): bool {
    return isAlnum(c) || c == '\'';
}

/// Capitalizes the first character of `input` (if ASCII lowercase) while keeping
/// the rest of the string unchanged. Writes into `out` and returns bytes written.
pub fn upperCaseFirst(input: string, out: []u8): usize {
    if (input.len == 0 || out.len == 0) {
        return 0;
    }
    out[0] = toUpperByte(input[0]);
    const n = if (input.len < out.len) input.len else out.len;
    if (n > 1) {
        @memcpy(out[1..n], input[1..n]);
    }
    return n;
}

/// Capitalizes the first letter of every word (after start of string, whitespace,
/// or punctuation) and lowercases interior word letters.
/// Writes into `out` and returns the number of bytes written.
pub fn titleize(input: string, out: []u8): usize {
    if (input.len == 0 || out.len == 0) {
        return 0;
    }
    let at_word_start = true;
    let i: usize = 0;
    while (i < input.len && i < out.len) : (i += 1) {
        const c = input[i];
        if (titIsWordChar(c)) {
            if (at_word_start) {
                out[i] = toUpperByte(c);
                at_word_start = false;
            } else {
                out[i] = toLowerByte(c);
            }
        } else {
            out[i] = c;
            at_word_start = true;
        }
    }
    return i;
}
