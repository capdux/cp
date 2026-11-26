// packages/@cap/slugify/src/index.cp
// Zero-allocation URL slug generator with UTF-8 Latin diacritic folding.
// MIT clean-room pure CP implementation.

fn slToLower(c: u8): u8 {
    if (c >= ('A' as u8) && c <= ('Z' as u8)) {
        return c + (32 as u8);
    }
    return c;
}

fn slIsAsciiAlnum(c: u8): bool {
    return (c >= ('a' as u8) && c <= ('z' as u8)) || (c >= ('A' as u8) && c <= ('Z' as u8)) || (c >= ('0' as u8) && c <= ('9' as u8));
}

/// Fold a Latin-1 Supplement or Latin Extended-A Unicode code point (`U+00C0..U+017F`)
/// into 1 or 2 ASCII characters written to `buf`. Returns number of ASCII bytes (0 if unmapped).
fn slFoldLatin(cp: u32, lowercase: bool, buf: []u8): usize {
    // Special 2-char ligatures & German sharp s.
    if (cp == (0x00DF as u32)) {
        buf[0] = 's' as u8;
        buf[1] = 's' as u8;
        return 2;
    }
    if (cp == (0x00C6 as u32)) {
        if (lowercase) { buf[0] = 'a' as u8; buf[1] = 'e' as u8; } else { buf[0] = 'A' as u8; buf[1] = 'E' as u8; }
        return 2;
    }
    if (cp == (0x00E6 as u32)) {
        buf[0] = 'a' as u8;
        buf[1] = 'e' as u8;
        return 2;
    }
    if (cp == (0x0152 as u32)) {
        if (lowercase) { buf[0] = 'o' as u8; buf[1] = 'e' as u8; } else { buf[0] = 'O' as u8; buf[1] = 'E' as u8; }
        return 2;
    }
    if (cp == (0x0153 as u32)) {
        buf[0] = 'o' as u8;
        buf[1] = 'e' as u8;
        return 2;
    }

    let ch: u8 = 0 as u8;
    // Latin-1 Supplement uppercase
    if (cp >= (0x00C0 as u32) && cp <= (0x00C5 as u32)) { ch = 'A' as u8; }
    else if (cp == (0x00C7 as u32)) { ch = 'C' as u8; }
    else if (cp >= (0x00C8 as u32) && cp <= (0x00CB as u32)) { ch = 'E' as u8; }
    else if (cp >= (0x00CC as u32) && cp <= (0x00CF as u32)) { ch = 'I' as u8; }
    else if (cp == (0x00D0 as u32)) { ch = 'D' as u8; }
    else if (cp == (0x00D1 as u32)) { ch = 'N' as u8; }
    else if ((cp >= (0x00D2 as u32) && cp <= (0x00D6 as u32)) || cp == (0x00D8 as u32)) { ch = 'O' as u8; }
    else if (cp >= (0x00D9 as u32) && cp <= (0x00DC as u32)) { ch = 'U' as u8; }
    else if (cp == (0x00DD as u32)) { ch = 'Y' as u8; }
    // Latin-1 Supplement lowercase
    else if (cp >= (0x00E0 as u32) && cp <= (0x00E5 as u32)) { ch = 'a' as u8; }
    else if (cp == (0x00E7 as u32)) { ch = 'c' as u8; }
    else if (cp >= (0x00E8 as u32) && cp <= (0x00EB as u32)) { ch = 'e' as u8; }
    else if (cp >= (0x00EC as u32) && cp <= (0x00EF as u32)) { ch = 'i' as u8; }
    else if (cp == (0x00F0 as u32)) { ch = 'd' as u8; }
    else if (cp == (0x00F1 as u32)) { ch = 'n' as u8; }
    else if ((cp >= (0x00F2 as u32) && cp <= (0x00F6 as u32)) || cp == (0x00F8 as u32)) { ch = 'o' as u8; }
    else if (cp >= (0x00F9 as u32) && cp <= (0x00FC as u32)) { ch = 'u' as u8; }
    else if (cp == (0x00FD as u32) || cp == (0x00FF as u32)) { ch = 'y' as u8; }
    // Latin Extended-A common ranges (even = uppercase, odd = lowercase for most pairs)
    else if (cp >= (0x0100 as u32) && cp <= (0x0105 as u32)) { ch = 'a' as u8; }
    else if (cp >= (0x0106 as u32) && cp <= (0x010D as u32)) { ch = 'c' as u8; }
    else if (cp >= (0x010E as u32) && cp <= (0x0111 as u32)) { ch = 'd' as u8; }
    else if (cp >= (0x0112 as u32) && cp <= (0x011B as u32)) { ch = 'e' as u8; }
    else if (cp >= (0x011C as u32) && cp <= (0x0123 as u32)) { ch = 'g' as u8; }
    else if (cp >= (0x0124 as u32) && cp <= (0x0127 as u32)) { ch = 'h' as u8; }
    else if (cp >= (0x0128 as u32) && cp <= (0x0131 as u32)) { ch = 'i' as u8; }
    else if (cp >= (0x0134 as u32) && cp <= (0x0135 as u32)) { ch = 'j' as u8; }
    else if (cp >= (0x0136 as u32) && cp <= (0x0138 as u32)) { ch = 'k' as u8; }
    else if (cp >= (0x0139 as u32) && cp <= (0x0142 as u32)) { ch = 'l' as u8; }
    else if (cp >= (0x0143 as u32) && cp <= (0x014B as u32)) { ch = 'n' as u8; }
    else if (cp >= (0x014C as u32) && cp <= (0x0151 as u32)) { ch = 'o' as u8; }
    else if (cp >= (0x0154 as u32) && cp <= (0x0159 as u32)) { ch = 'r' as u8; }
    else if (cp >= (0x015A as u32) && cp <= (0x0161 as u32)) { ch = 's' as u8; }
    else if (cp >= (0x0162 as u32) && cp <= (0x0167 as u32)) { ch = 't' as u8; }
    else if (cp >= (0x0168 as u32) && cp <= (0x0173 as u32)) { ch = 'u' as u8; }
    else if (cp >= (0x0174 as u32) && cp <= (0x0175 as u32)) { ch = 'w' as u8; }
    else if (cp >= (0x0176 as u32) && cp <= (0x0178 as u32)) { ch = 'y' as u8; }
    else if (cp >= (0x0179 as u32) && cp <= (0x017E as u32)) { ch = 'z' as u8; }

    if (ch == (0 as u8)) {
        return 0;
    }
    if (lowercase) {
        buf[0] = slToLower(ch);
    } else {
        buf[0] = ch;
    }
    return 1;
}

/// Convert `input` into a URL-safe slug using custom separator `sep` and optional `lowercase`.
/// Folds Latin-1 / Latin Extended-A diacritics, collapses consecutive non-alphanumerics into a single `sep`,
/// and strips leading/trailing separators. Writes into `out` and returns bytes written.
pub fn slugifyWithSep(input: string, sep: u8, lowercase: bool, out: []u8): usize {
    if (out.len == 0) {
        return 0;
    }

    let pos: usize = 0;
    let pendingSep = false;
    let folded: [2]u8 = undefined;
    let i: usize = 0;

    while (i < input.len && pos < out.len) {
        const b0 = input[i];

        if (b0 < (0x80 as u8)) {
            if (slIsAsciiAlnum(b0)) {
                if (pendingSep && pos > 0) {
                    if (pos >= out.len) {
                        break;
                    }
                    out[pos] = sep;
                    pos += 1;
                    pendingSep = false;
                }
                if (pos >= out.len) {
                    break;
                }
                if (lowercase) {
                    out[pos] = slToLower(b0);
                } else {
                    out[pos] = b0;
                }
                pos += 1;
            } else {
                if (pos > 0) {
                    pendingSep = true;
                }
            }
            i += 1;
        } else if ((b0 & (0xE0 as u8)) == (0xC0 as u8) && i + 1 < input.len && (input[i + 1] & (0xC0 as u8)) == (0x80 as u8)) {
            // 2-byte UTF-8 sequence.
            const cp: u32 = (((b0 & (0x1F as u8)) as u32) << (6 as u5)) | ((input[i + 1] & (0x3F as u8)) as u32);
            const flen = slFoldLatin(cp, lowercase, folded[0..2]);
            if (flen > 0) {
                if (pendingSep && pos > 0) {
                    if (pos >= out.len) {
                        break;
                    }
                    out[pos] = sep;
                    pos += 1;
                    pendingSep = false;
                }
                let fi: usize = 0;
                while (fi < flen && pos < out.len) {
                    out[pos] = folded[fi];
                    pos += 1;
                    fi += 1;
                }
            } else {
                if (pos > 0) {
                    pendingSep = true;
                }
            }
            i += 2;
        } else if ((b0 & (0xF0 as u8)) == (0xE0 as u8) && i + 2 < input.len) {
            if (pos > 0) {
                pendingSep = true;
            }
            i += 3;
        } else if ((b0 & (0xF8 as u8)) == (0xF0 as u8) && i + 3 < input.len) {
            if (pos > 0) {
                pendingSep = true;
            }
            i += 4;
        } else {
            if (pos > 0) {
                pendingSep = true;
            }
            i += 1;
        }
    }

    // Ensure no trailing separator remains if output buffer ended right after writing sep.
    while (pos > 0 && out[pos - 1] == sep) {
        pos -= 1;
    }
    return pos;
}

/// Convert `input` into a lowercase hyphen-separated URL slug into `out`.
pub fn slugify(input: string, out: []u8): usize {
    return slugifyWithSep(input, '-' as u8, true, out);
}

/// Convert `input` into a lowercase hyphen-separated URL slug truncated to at most `max_len` bytes,
/// guaranteed never to end with `'-'`.
pub fn slugifyTruncate(input: string, max_len: usize, out: []u8): usize {
    let limit = max_len;
    if (limit > out.len) {
        limit = out.len;
    }
    if (limit == 0) {
        return 0;
    }
    let n = slugifyWithSep(input, '-' as u8, true, out[0..limit]);
    while (n > 0 && out[n - 1] == ('-' as u8)) {
        n -= 1;
    }
    return n;
}

/// Validate a canonical lowercase URL slug:
/// non-empty, only `a-z`, `0-9`, `-`, no leading/trailing `-`, no consecutive `--`.
pub fn isValidSlug(s: string): bool {
    if (s.len == 0) {
        return false;
    }
    if (s[0] == ('-' as u8) || s[s.len - 1] == ('-' as u8)) {
        return false;
    }
    let prevDash = false;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == ('-' as u8)) {
            if (prevDash) {
                return false;
            }
            prevDash = true;
        } else if ((c >= ('a' as u8) && c <= ('z' as u8)) || (c >= ('0' as u8) && c <= ('9' as u8))) {
            prevDash = false;
        } else {
            return false;
        }
        i += 1;
    }
    return true;
}

export {
    slugify,
    slugifyWithSep,
    slugifyTruncate,
    isValidSlug,
};
