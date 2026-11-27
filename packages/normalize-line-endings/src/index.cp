// normalize-line-endings — Zero-heap CRLF/CR <-> LF line ending normalizer in pure CP
// Clean-room Apache-2.0 implementation of crates.io `normalize-line-endings` (#875, 127.2M downloads).

pub enum LineEnding {
    None,
    Lf,
    Crlf,
    Cr,
    Mixed,
}

/// Returns the canonical byte sequence for `le` (`"\n"`, `"\r\n"`, `"\r"`, or `""`).
pub fn lineEndingStr(le: LineEnding): string {
    return switch (le) {
        .Lf => "\n",
        .Crlf => "\r\n",
        .Cr => "\r",
        .None => "",
        .Mixed => "",
    };
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

/// Counts the number of line breaks (`\r\n` counted as 1, lone `\r` or `\n` each counted as 1).
pub fn countLineBreaks(input: string): usize {
    let breaks: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (input[i] == 13) {
            breaks += 1;
            if (i + 1 < input.len && input[i + 1] == 10) {
                i += 2;
            } else {
                i += 1;
            }
        } else if (input[i] == 10) {
            breaks += 1;
            i += 1;
        } else {
            i += 1;
        }
    }
    return breaks;
}

/// Counts logical lines in `input` (matching Rust `str::lines().count()` semantics):
/// empty string -> 0; trailing line break does not add an extra empty line.
pub fn countLines(input: string): usize {
    if (input.len == 0) {
        return 0;
    }
    let lines: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        lines += 1;
        while (i < input.len && input[i] != 10 && input[i] != 13) {
            i += 1;
        }
        if (i < input.len) {
            if (input[i] == 13 && i + 1 < input.len && input[i + 1] == 10) {
                i += 2;
            } else {
                i += 1;
            }
        }
    }
    return lines;
}

/// Returns true if `input` contains any line break that differs from `target`.
pub fn needsNormalization(input: string, target: LineEnding): bool {
    const detected = detectLineEnding(input);
    if (detected == .None) {
        return false;
    }
    return detected != target;
}

/// Computes the exact output byte length for normalizing `input` to `target`.
pub fn normalizedLen(input: string, target: LineEnding): usize {
    const eol_len: usize = switch (target) {
        .Crlf => 2,
        .Lf => 1,
        .Cr => 1,
        .None => 0,
        .Mixed => 1,
    };
    let total: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (input[i] == 13) {
            total += eol_len;
            if (i + 1 < input.len && input[i + 1] == 10) {
                i += 2;
            } else {
                i += 1;
            }
        } else if (input[i] == 10) {
            total += eol_len;
            i += 1;
        } else {
            total += 1;
            i += 1;
        }
    }
    return total;
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

/// Normalizes all `\r\n` and `\n` line endings in `input` to classic Mac `\r` (`CR`) into `out`.
/// Returns bytes written.
pub fn normalizeToCr(input: string, out: []u8): usize {
    let p: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (input[i] == 13) {
            if (p < out.len) {
                out[p] = 13;
                p += 1;
            }
            if (i + 1 < input.len && input[i + 1] == 10) {
                i += 2;
            } else {
                i += 1;
            }
        } else if (input[i] == 10) {
            if (p < out.len) {
                out[p] = 13;
                p += 1;
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

/// Strips all line endings (`\r\n`, `\n`, `\r`) from `input` into `out`.
fn stripLineEndings(input: string, out: []u8): usize {
    let p: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (input[i] == 13 || input[i] == 10) {
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

/// Normalizes all line endings in `input` to the requested `target` (`Lf`, `Crlf`, `Cr`, or `None` to strip).
pub fn normalizeLineEndings(input: string, target: LineEnding, out: []u8): usize {
    return switch (target) {
        .Lf => normalizeToLf(input, out),
        .Crlf => normalizeToCrlf(input, out),
        .Cr => normalizeToCr(input, out),
        .None => stripLineEndings(input, out),
        .Mixed => normalizeToLf(input, out),
    };
}

export {
    LineEnding,
    lineEndingStr,
    detectLineEnding,
    countLineBreaks,
    countLines,
    needsNormalization,
    normalizedLen,
    normalizeToLf,
    normalizeToCrlf,
    normalizeToCr,
    normalizeLineEndings,
};
