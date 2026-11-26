// @cap/titleize — Zero-allocation string title-case utility for CP
// Clean-room MIT implementation with parity to npm `titleize` v3.

fn titIsLower(c: u8): bool {
    return c >= 'a' && c <= 'z';
}

fn titIsUpper(c: u8): bool {
    return c >= 'A' && c <= 'Z';
}

fn titToUpper(c: u8): u8 {
    if (titIsLower(c)) {
        return c - 32;
    }
    return c;
}

fn titToLower(c: u8): u8 {
    if (titIsUpper(c)) {
        return c + 32;
    }
    return c;
}

fn titIsWordChar(c: u8): bool {
    return titIsLower(c) || titIsUpper(c) || (c >= '0' && c <= '9') || c == '\'';
}

/// Capitalizes the first character of `input` (if ASCII lowercase) while keeping
/// the rest of the string unchanged. Writes into `out` and returns bytes written.
pub fn upperCaseFirst(input: string, out: []u8): usize {
    if (input.len == 0 || out.len == 0) {
        return 0;
    }
    out[0] = titToUpper(input[0]);
    let i: usize = 1;
    while (i < input.len && i < out.len) : (i += 1) {
        out[i] = input[i];
    }
    return i;
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
                out[i] = titToUpper(c);
                at_word_start = false;
            } else {
                out[i] = titToLower(c);
            }
        } else {
            out[i] = c;
            at_word_start = true;
        }
    }
    return i;
}
