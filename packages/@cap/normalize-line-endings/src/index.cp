// @cap/normalize-line-endings — Zero-heap CRLF/CR <-> LF line ending normalizer in pure CP
// Clean-room Apache-2.0 implementation of crates.io `normalize-line-endings` (#875, 127.2M downloads).

pub enum LineEnding {
    None,
    Lf,
    Crlf,
    Cr,
    Mixed,
}

/// Detects the line ending style used in `input` (`None`, `Lf`, `Crlf`, `Cr`, or `Mixed`).
pub fn detectLineEnding(input: string): LineEnding {
    let has_lf = false;
    let has_crlf = false;
    let has_cr = false;
    let i: usize = 0;
    while (i < input.len) {
        if (input[i] == 13) { // '\r'
            if (i + 1 < input.len && input[i + 1] == 10) {
                has_crlf = true;
                i += 2;
                continue;
            } else {
                has_cr = true;
            }
        } else if (input[i] == 10) { // '\n'
            has_lf = true;
        }
        i += 1;
    }
    let kinds: u8 = 0;
    if (has_lf) { kinds += 1; }
    if (has_crlf) { kinds += 1; }
    if (has_cr) { kinds += 1; }
    if (kinds == 0) { return .None; }
    if (kinds > 1) { return .Mixed; }
    if (has_crlf) { return .Crlf; }
    if (has_cr) { return .Cr; }
    return .Lf;
}

/// Normalizes all `\r\n` and lone `\r` line endings in `input` to Unix `\n` (`LF`) into `out`.
/// Returns bytes written.
pub fn normalizeToLf(input: string, out: []u8): usize {
    let p: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (input[i] == 13) {
            if (p < out.len) {
                out[p] = 10;
                p += 1;
            }
            if (i + 1 < input.len && input[i + 1] == 10) {
                i += 2;
            } else {
                i += 1;
            }
        } else {
            if (p < out.len) {
                out[p] = input[i];
                p += 1;
            }
            i += 1;
        }
    }
    return p;
}

/// Normalizes all `\n`, `\r\n`, and lone `\r` line endings in `input` to Windows `\r\n` (`CRLF`) into `out`.
/// Returns bytes written.
pub fn normalizeToCrlf(input: string, out: []u8): usize {
    let p: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (input[i] == 13) {
            if (p + 1 < out.len) {
                out[p] = 13;
                out[p + 1] = 10;
                p += 2;
            }
            if (i + 1 < input.len && input[i + 1] == 10) {
                i += 2;
            } else {
                i += 1;
            }
        } else if (input[i] == 10) {
            if (p + 1 < out.len) {
                out[p] = 13;
                out[p + 1] = 10;
                p += 2;
            }
            i += 1;
        } else {
            if (p < out.len) {
                out[p] = input[i];
                p += 1;
            }
            i += 1;
        }
    }
    return p;
}

export {
    LineEnding,
    detectLineEnding,
    normalizeToLf,
    normalizeToCrlf,
};
