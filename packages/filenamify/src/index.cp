// @cap/filenamify — Zero-allocation cross-platform safe filename sanitizer for CP
// Clean-room MIT implementation with parity to npm `filenamify` v6.

const FNM_WORK_CAP: usize = 2048;
const FNM_DEFAULT_MAX_LEN: usize = 100;

fn fnmIsInvalidChar(ch: u8): bool {
    if (ch <= (0x1F as u8) || ch == (0x7F as u8)) {
        return true;
    }
    return ch == (60 as u8)   // '<'
        || ch == (62 as u8)   // '>'
        || ch == (58 as u8)   // ':'
        || ch == (34 as u8)   // '"'
        || ch == (47 as u8)   // '/'
        || ch == (92 as u8)   // '\'
        || ch == (124 as u8)  // '|'
        || ch == (63 as u8)   // '?'
        || ch == (42 as u8);  // '*'
}

fn fnmHasInvalidChar(s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (fnmIsInvalidChar(s[i])) {
            return true;
        }
    }
    return false;
}

fn fnmToLowerAscii(ch: u8): u8 {
    if (ch >= (65 as u8) && ch <= (90 as u8)) {
        return ch + (32 as u8);
    }
    return ch;
}

fn fnmEq3IgnoreCase(s: string, a: u8, b: u8, c: u8): bool {
    return fnmToLowerAscii(s[0]) == a
        && fnmToLowerAscii(s[1]) == b
        && fnmToLowerAscii(s[2]) == c;
}

fn fnmMatchAt(buf: string, pos: usize, pat: string): bool {
    if (pat.len == 0 || pos + pat.len > buf.len) {
        return false;
    }
    let i: usize = 0;
    while (i < pat.len) : (i += 1) {
        if (buf[pos + i] != pat[i]) {
            return false;
        }
    }
    return true;
}

fn fnmEndsWith(buf: string, len: usize, pat: string): bool {
    if (pat.len == 0 || len < pat.len) {
        return false;
    }
    return fnmMatchAt(buf[0..len], len - pat.len, pat);
}

fn fnmAppendStr(dst: []u8, pos: usize, src: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < src.len && p < dst.len) : (i += 1) {
        dst[p] = src[i];
        p += 1;
    }
    return p;
}

fn fnmUtf8SafeLen(s: string, max_len: usize): usize {
    if (max_len == 0 || s.len <= max_len) {
        return s.len;
    }
    let cut = max_len;
    // Back up over UTF-8 continuation bytes (0b10xxxxxx) so we never split a codepoint
    while (cut > 0 && (s[cut] & (0xC0 as u8)) == (0x80 as u8)) {
        cut -= 1;
    }
    return cut;
}

/// Check whether `name` is a Windows reserved device filename
/// (`CON`, `PRN`, `AUX`, `NUL`, `COM1`..`COM9`, `LPT1`..`LPT9`),
/// case-insensitively and including filenames with extensions (e.g. `con.txt`).
pub fn isReservedFilename(name: string): bool {
    if (name.len < 3) {
        return false;
    }
    let base_end: usize = name.len;
    let i: usize = 0;
    while (i < name.len) : (i += 1) {
        if (name[i] == (46 as u8)) {
            base_end = i;
            break;
        }
    }
    if (base_end == 3) {
        const base = name[0..3];
        if (fnmEq3IgnoreCase(base, 99 as u8, 111 as u8, 110 as u8)) { return true; }  // con
        if (fnmEq3IgnoreCase(base, 112 as u8, 114 as u8, 110 as u8)) { return true; } // prn
        if (fnmEq3IgnoreCase(base, 97 as u8, 117 as u8, 120 as u8)) { return true; }  // aux
        if (fnmEq3IgnoreCase(base, 110 as u8, 117 as u8, 108 as u8)) { return true; } // nul
        return false;
    }
    if (base_end == 4) {
        const base = name[0..4];
        const d = base[3];
        if (d < (49 as u8) || d > (57 as u8)) {
            return false;
        }
        if (fnmEq3IgnoreCase(base, 99 as u8, 111 as u8, 109 as u8)) { return true; }  // com1..9
        if (fnmEq3IgnoreCase(base, 108 as u8, 112 as u8, 116 as u8)) { return true; } // lpt1..9
        return false;
    }
    return false;
}

/// Convert `input` into a cross-platform safe filename using custom `replacement` and `max_len`.
/// - Replaces `<>:"/\|?*` and control bytes (`0x00..0x1F`, `0x7F`) with `replacement`
/// - Collapses repeated `replacement` sequences and runs of 2+ dots (`..`)
/// - Strips leading/trailing `replacement` and trailing `.` or space
/// - Appends `replacement` to Windows reserved device names (`CON`, `NUL`, `com1.txt`, etc.)
/// - Truncates to `max_len` bytes on a valid UTF-8 boundary (`max_len == 0` disables truncation)
/// Writes into `out` and returns the number of bytes written.
pub fn filenamifyWithOptions(
    input: string,
    replacement: string,
    max_len: usize,
    out: []u8,
): usize {
    if (input.len == 0) {
        return 0;
    }

    const rep: string = if (fnmHasInvalidChar(replacement)) "!" else replacement;

    let work: [FNM_WORK_CAP]u8 = undefined;
    let wlen: usize = 0;

    // Pass 1: replace invalid chars & multi-dot runs (".."), collapsing consecutive replacements
    let i: usize = 0;
    while (i < input.len) {
        const ch = input[i];
        if (fnmIsInvalidChar(ch)) {
            while (i < input.len && fnmIsInvalidChar(input[i])) {
                i += 1;
            }
            if (rep.len > 0 && !fnmEndsWith(work[0..wlen], wlen, rep)) {
                wlen = fnmAppendStr(work[0..FNM_WORK_CAP], wlen, rep);
            }
        } else if (ch == (46 as u8) && i + 1 < input.len && input[i + 1] == (46 as u8)) {
            // Collapse 2+ consecutive dots ("..", "...") into replacement
            while (i < input.len && input[i] == (46 as u8)) {
                i += 1;
            }
            if (rep.len > 0 && !fnmEndsWith(work[0..wlen], wlen, rep)) {
                wlen = fnmAppendStr(work[0..FNM_WORK_CAP], wlen, rep);
            }
        } else if (rep.len > 0 && fnmMatchAt(input, i, rep)) {
            if (!fnmEndsWith(work[0..wlen], wlen, rep)) {
                wlen = fnmAppendStr(work[0..FNM_WORK_CAP], wlen, rep);
            }
            i += rep.len;
        } else {
            if (wlen < FNM_WORK_CAP) {
                work[wlen] = ch;
                wlen += 1;
            }
            i += 1;
        }
    }

    // Pass 2: strip leading replacement sequences and trailing replacement / '.' / ' '
    let start: usize = 0;
    let end: usize = wlen;

    if (rep.len > 0) {
        while (start + rep.len <= end && fnmMatchAt(work[0..end], start, rep)) {
            start += rep.len;
        }
    }

    let trimmed = true;
    while (trimmed && end > start) {
        trimmed = false;
        if (rep.len > 0 && end - start >= rep.len && fnmMatchAt(work[0..end], end - rep.len, rep)) {
            end -= rep.len;
            trimmed = true;
        } else if (work[end - 1] == (46 as u8) || work[end - 1] == (32 as u8)) {
            end -= 1;
            trimmed = true;
        }
    }

    // Pass 3: copy normalized slice into second working buffer for reserved-name & truncation handling
    let final_buf: [FNM_WORK_CAP]u8 = undefined;
    let flen: usize = 0;

    if (end > start) {
        flen = fnmAppendStr(final_buf[0..FNM_WORK_CAP], 0, work[start..end]);
    } else if (rep.len > 0) {
        // Non-empty input reduced to empty (e.g. ".", "..", "///") -> fallback to replacement
        flen = fnmAppendStr(final_buf[0..FNM_WORK_CAP], 0, rep);
    }

    // Pass 4: guard Windows reserved device names
    if (flen > 0 && isReservedFilename(final_buf[0..flen])) {
        const suffix: string = if (rep.len > 0) rep else "!";
        flen = fnmAppendStr(final_buf[0..FNM_WORK_CAP], flen, suffix);
    }

    // Pass 5: truncate cleanly to max_len on UTF-8 boundary
    if (max_len > 0 && flen > max_len) {
        flen = fnmUtf8SafeLen(final_buf[0..flen], max_len);
    }

    return fnmAppendStr(out, 0, final_buf[0..flen]);
}

/// Convert `input` into a safe filename using default options (`replacement = "!"`, `max_len = 100`).
pub fn filenamify(input: string, out: []u8): usize {
    return filenamifyWithOptions(input, "!", FNM_DEFAULT_MAX_LEN, out);
}
