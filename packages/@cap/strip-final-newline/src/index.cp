// @cap/strip-final-newline — Zero-copy trailing LF / CRLF newline stripper
// Matching npm strip-final-newline (250M+/wk, direct dep of execa). Clean-room MIT in pure CP.

// Strips at most ONE trailing `\n` or `\r\n` (strictly matching npm strip-final-newline).
pub fn stripFinalNewline(input: string): string {
    let end = input.len;
    if (end > 0 && input[end - 1] == 10) {
        end = end - 1;
        if (end > 0 && input[end - 1] == 13) {
            end = end - 1;
        }
    }
    return input[0..end];
}

// Strips ALL trailing `\r` and `\n` characters (zero-copy slice).
pub fn stripAllTrailingNewlines(input: string): string {
    let end = input.len;
    while (end > 0 && (input[end - 1] == 10 || input[end - 1] == 13)) {
        end = end - 1;
    }
    return input[0..end];
}

pub fn hasFinalNewline(input: string): bool {
    return input.len > 0 && input[input.len - 1] == 10;
}

pub fn hasFinalCrlf(input: string): bool {
    return input.len >= 2 && input[input.len - 2] == 13 && input[input.len - 1] == 10;
}

// Ensures output ends with `\n` (or `\r\n` if use_crlf=true) without duplicating an existing trailing newline.
pub fn ensureFinalNewline(input: string, use_crlf: bool, out: []u8): usize {
    let base = stripFinalNewline(input);
    let p: usize = 0;
    let i: usize = 0;
    while (i < base.len && p < out.len) {
        out[p] = base[i];
        p = p + 1;
        i = i + 1;
    }
    if (use_crlf) {
        if (p + 2 <= out.len) {
            out[p] = 13;
            out[p + 1] = 10;
            p = p + 2;
        }
    } else {
        if (p + 1 <= out.len) {
            out[p] = 10;
            p = p + 1;
        }
    }
    return p;
}
