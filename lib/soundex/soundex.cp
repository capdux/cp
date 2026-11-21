// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Phonetic Coding Suite: NARA Soundex, Metaphone 1990, NYSIIS & Cologne Phonetics)

/// Fixed output length of American Soundex codes (1 letter + 3 digits).
pub const SOUNDEX_LEN: usize = 4;

/// Maximum internal cleaned letter buffer length for zero-alloc phonetic transforms.
pub const MAX_PHONETIC_WORD_LEN: usize = 256;

fn toUpperAscii(ch: u8): u8? {
    if (ch >= 0x41 && ch <= 0x5A) {
        return ch;
    }
    if (ch >= 0x61 && ch <= 0x7A) {
        return ch - 32;
    }
    return null;
}

fn isVowelAEIOU(ch: u8): bool {
    return ch == 0x41 || ch == 0x45 || ch == 0x49 || ch == 0x4F || ch == 0x55;
}

fn isVowelAEIOUY(ch: u8): bool {
    return isVowelAEIOU(ch) || ch == 0x59;
}

fn cleanUpperLetters(word: []const u8, buf: []u8): usize {
    let n: usize = 0;
    let i: usize = 0;
    while (i < word.len && n < buf.len) {
        if (up := toUpperAscii(word[i])) {
            buf[n] = up;
            n += 1;
        }
        i += 1;
    }
    return n;
}

fn soundexDigit(up: u8): u8 {
    switch (up) {
        0x42, 0x46, 0x50, 0x56 => return 0x31,                         // B, F, P, V -> '1'
        0x43, 0x47, 0x4A, 0x4B, 0x51, 0x53, 0x58, 0x5A => return 0x32, // C, G, J, K, Q, S, X, Z -> '2'
        0x44, 0x54 => return 0x33,                                     // D, T -> '3'
        0x4C => return 0x34,                                           // L -> '4'
        0x4D, 0x4E => return 0x35,                                     // M, N -> '5'
        0x52 => return 0x36,                                           // R -> '6'
        else => return 0,
    }
}

/// Computes the NARA / Knuth American Soundex 4-byte code for `word`.
/// Correctly implements H/W transparent adjacency deduplication (e.g. `"Ashcraft"` -> `"A261"`,
/// `"Tymczak"` -> `"T522"`, `"Pfister"` -> `"P236"`, `"Honeyman"` -> `"H555"`).
/// Returns `null` if `word` contains no ASCII letters.
pub fn soundex(word: []const u8): ([4]u8)? {
    let res: [4]u8 = [0x30, 0x30, 0x30, 0x30];
    let have_first = false;
    let out_idx: usize = 1;
    let prev_code: u8 = 0;

    let i: usize = 0;
    while (i < word.len) {
        if (up := toUpperAscii(word[i])) {
            if (!have_first) {
                res[0] = up;
                have_first = true;
                prev_code = soundexDigit(up);
            } else {
                // NARA rule: H and W are transparent and do not reset prev_code.
                if (up == 0x48 || up == 0x57) {
                    i += 1;
                    continue;
                }
                const d = soundexDigit(up);
                if (d == 0) {
                    // Vowels (A, E, I, O, U, Y) act as separators and reset prev_code.
                    prev_code = 0;
                } else if (d != prev_code) {
                    if (out_idx < SOUNDEX_LEN) {
                        res[out_idx] = d;
                        out_idx += 1;
                    }
                    prev_code = d;
                }
            }
        }
        i += 1;
    }

    if (!have_first) {
        return null;
    }
    return res;
}

/// Returns `true` if `a` and `b` both produce valid and identical American Soundex codes.
pub fn soundexEquals(a: []const u8, b: []const u8): bool {
    const sa = soundex(a) ?? return false;
    const sb = soundex(b) ?? return false;
    return sa[0] == sb[0] && sa[1] == sb[1] && sa[2] == sb[2] && sa[3] == sb[3];
}

/// Computes a SQL Server-style Soundex similarity score in `0..4`
/// (number of matching positions between the two 4-byte Soundex codes; `0` if either is invalid).
pub fn soundexDifference(a: []const u8, b: []const u8): u8 {
    const sa = soundex(a) ?? return 0;
    const sb = soundex(b) ?? return 0;
    let score: u8 = 0;
    let i: usize = 0;
    while (i < SOUNDEX_LEN) {
        if (sa[i] == sb[i]) {
            score += 1;
        }
        i += 1;
    }
    return score;
}

fn pushByte(out: []u8, len: usize, ch: u8): usize? {
    if (len >= out.len) {
        return null;
    }
    out[len] = ch;
    return len + 1;
}

/// Computes the Lawrence Philips (1990) Metaphone phonetic key for `word` into `out` (zero-alloc).
/// Supports initial silent pairs (`KN`/`GN`/`PN`/`AE`/`WR`), trailing `MB`, `CH`/`SH`/`TH` (`'0'`)
/// /`PH`/`GH`/`CK`/`DG`/`TCH`/`WH`/`X` (`"KS"`), etc.
/// Returns bytes written, or `null` if `word` has no ASCII letters or `out` is too small.
pub fn metaphone(word: []const u8, out: []u8): usize? {
    let raw: [MAX_PHONETIC_WORD_LEN]u8 = [0; MAX_PHONETIC_WORD_LEN];
    const n = cleanUpperLetters(word, raw[0..]);
    if (n == 0) {
        return null;
    }

    let out_len: usize = 0;
    let i: usize = 0;

    // Initial two-letter exceptions
    if (n >= 2) {
        const c0 = raw[0];
        const c1 = raw[1];
        if (
            (c0 == 0x4B && c1 == 0x4E) || // KN
            (c0 == 0x47 && c1 == 0x4E) || // GN
            (c0 == 0x50 && c1 == 0x4E) || // PN
            (c0 == 0x41 && c1 == 0x45) || // AE
            (c0 == 0x57 && c1 == 0x52)    // WR
        ) {
            i = 1;
        } else if (c0 == 0x57 && c1 == 0x48) {
            // WH -> W
            out_len = pushByte(out, out_len, 0x57) ?? return null;
            i = 2;
        } else if (c0 == 0x58) {
            // Initial X -> S
            out_len = pushByte(out, out_len, 0x53) ?? return null;
            i = 1;
        }
    } else if (raw[0] == 0x58) {
        out_len = pushByte(out, out_len, 0x53) ?? return null;
        return out_len;
    }

    while (i < n) {
        const c = raw[i];
        const prev: u8 = if (i > 0) raw[i - 1] else 0;
        const next: u8 = if (i + 1 < n) raw[i + 1] else 0;
        const next2: u8 = if (i + 2 < n) raw[i + 2] else 0;

        // Deduplicate adjacent identical letters except 'C'
        if (i > 0 && c == prev && c != 0x43) {
            i += 1;
            continue;
        }

        if (isVowelAEIOU(c)) {
            if (i == 0) {
                out_len = pushByte(out, out_len, c) ?? return null;
            }
        } else if (c == 0x42) {
            // B: silent after M at end of word
            if (!(prev == 0x4D && i + 1 == n)) {
                out_len = pushByte(out, out_len, 0x42) ?? return null;
            }
        } else if (c == 0x43) {
            // C
            if (prev == 0x53 && (next == 0x45 || next == 0x49 || next == 0x59)) {
                // SCE, SCI, SCY: silent C
            } else if (next == 0x49 && next2 == 0x41) {
                // CIA -> X
                out_len = pushByte(out, out_len, 0x58) ?? return null;
            } else if (next == 0x45 || next == 0x49 || next == 0x59) {
                // CE, CI, CY -> S
                out_len = pushByte(out, out_len, 0x53) ?? return null;
            } else if (next == 0x48) {
                // CH: SCH -> K; otherwise X
                if (prev == 0x53) {
                    out_len = pushByte(out, out_len, 0x4B) ?? return null;
                } else {
                    out_len = pushByte(out, out_len, 0x58) ?? return null;
                }
            } else {
                out_len = pushByte(out, out_len, 0x4B) ?? return null;
            }
        } else if (c == 0x44) {
            // D: DGE, DGI, DGY -> J; else T
            if (next == 0x47 && (next2 == 0x45 || next2 == 0x49 || next2 == 0x59)) {
                out_len = pushByte(out, out_len, 0x4A) ?? return null;
            } else {
                out_len = pushByte(out, out_len, 0x54) ?? return null;
            }
        } else if (c == 0x46) {
            out_len = pushByte(out, out_len, 0x46) ?? return null;
        } else if (c == 0x47) {
            // G
            const silent_gh = next == 0x48 && i + 2 < n && !isVowelAEIOU(next2);
            const silent_gn = next == 0x4E && (i + 2 == n || (i + 4 == n && next2 == 0x45 && raw[i + 3] == 0x44));
            if (!silent_gh && !silent_gn) {
                if (next == 0x45 || next == 0x49 || next == 0x59) {
                    out_len = pushByte(out, out_len, 0x4A) ?? return null;
                } else {
                    out_len = pushByte(out, out_len, 0x4B) ?? return null;
                }
            }
        } else if (c == 0x48) {
            // H: silent after C, S, P, T, G or after vowel when not followed by vowel
            const after_csptg = prev == 0x43 || prev == 0x53 || prev == 0x50 || prev == 0x54 || prev == 0x47;
            if (!after_csptg && !(isVowelAEIOU(prev) && !isVowelAEIOU(next))) {
                out_len = pushByte(out, out_len, 0x48) ?? return null;
            }
        } else if (c == 0x4A) {
            out_len = pushByte(out, out_len, 0x4A) ?? return null;
        } else if (c == 0x4B) {
            if (prev != 0x43) {
                out_len = pushByte(out, out_len, 0x4B) ?? return null;
            }
        } else if (c == 0x4C || c == 0x4D || c == 0x4E || c == 0x52) {
            out_len = pushByte(out, out_len, c) ?? return null;
        } else if (c == 0x50) {
            if (next == 0x48) {
                out_len = pushByte(out, out_len, 0x46) ?? return null;
            } else {
                out_len = pushByte(out, out_len, 0x50) ?? return null;
            }
        } else if (c == 0x51) {
            out_len = pushByte(out, out_len, 0x4B) ?? return null;
        } else if (c == 0x53) {
            if (next == 0x48 || (next == 0x49 && (next2 == 0x4F || next2 == 0x41))) {
                out_len = pushByte(out, out_len, 0x58) ?? return null;
            } else {
                out_len = pushByte(out, out_len, 0x53) ?? return null;
            }
        } else if (c == 0x54) {
            if (next == 0x49 && (next2 == 0x4F || next2 == 0x41)) {
                out_len = pushByte(out, out_len, 0x58) ?? return null;
            } else if (next == 0x48) {
                out_len = pushByte(out, out_len, 0x30) ?? return null; // '0' (theta)
            } else if (next == 0x43 && next2 == 0x48) {
                // TCH: silent T
            } else {
                out_len = pushByte(out, out_len, 0x54) ?? return null;
            }
        } else if (c == 0x56) {
            out_len = pushByte(out, out_len, 0x46) ?? return null;
        } else if (c == 0x57 || c == 0x59) {
            if (isVowelAEIOU(next)) {
                out_len = pushByte(out, out_len, c) ?? return null;
            }
        } else if (c == 0x58) {
            out_len = pushByte(out, out_len, 0x4B) ?? return null;
            out_len = pushByte(out, out_len, 0x53) ?? return null;
        } else if (c == 0x5A) {
            out_len = pushByte(out, out_len, 0x53) ?? return null;
        }

        i += 1;
    }

    return out_len;
}

fn appendDedup(buf: []u8, len: usize, ch: u8): usize {
    if (len > 0 && buf[len - 1] == ch) {
        return len;
    }
    if (len < buf.len) {
        buf[len] = ch;
        return len + 1;
    }
    return len;
}

/// Computes the NYSIIS (New York State Identification and Intelligence System) phonetic key
/// for `word` into `out` with zero heap allocation.
/// Applies standard prefix (`MAC->MCC`, `KN->NN`, `K->C`, `PH/PF->FF`, `SCH->SSS`),
/// suffix (`EE/IE->Y`, `DT/RT/RD/NT/ND->D`), inner transforms, deduplication, and trailing cleanup.
/// Returns bytes written, or `null` if `word` has no ASCII letters or `out` is too small.
pub fn nysiis(word: []const u8, out: []u8): usize? {
    let raw: [MAX_PHONETIC_WORD_LEN]u8 = [0; MAX_PHONETIC_WORD_LEN];
    let n = cleanUpperLetters(word, raw[0..]);
    if (n == 0) {
        return null;
    }

    // 1. Prefix transforms
    if (n >= 3 && raw[0] == 0x4D && raw[1] == 0x41 && raw[2] == 0x43) {
        // MAC -> MCC
        raw[1] = 0x43;
    } else if (n >= 2 && raw[0] == 0x4B && raw[1] == 0x4E) {
        // KN -> NN
        raw[0] = 0x4E;
    } else if (raw[0] == 0x4B) {
        // K -> C
        raw[0] = 0x43;
    } else if (n >= 2 && raw[0] == 0x50 && (raw[1] == 0x48 || raw[1] == 0x46)) {
        // PH, PF -> FF
        raw[0] = 0x46;
        raw[1] = 0x46;
    } else if (n >= 3 && raw[0] == 0x53 && raw[1] == 0x43 && raw[2] == 0x48) {
        // SCH -> SSS
        raw[1] = 0x53;
        raw[2] = 0x53;
    }

    // 2. Suffix transforms
    if (n >= 2) {
        const a = raw[n - 2];
        const b = raw[n - 1];
        if ((a == 0x45 || a == 0x49) && b == 0x45) {
            // EE, IE -> Y
            raw[n - 2] = 0x59;
            n -= 1;
        } else if (
            (a == 0x44 && b == 0x54) || // DT
            (a == 0x52 && b == 0x54) || // RT
            (a == 0x52 && b == 0x44) || // RD
            (a == 0x4E && b == 0x54) || // NT
            (a == 0x4E && b == 0x44)    // ND
        ) {
            raw[n - 2] = 0x44;
            n -= 1;
        }
    }

    // 3. First letter preserved + body scan with deduplication
    let key: [MAX_PHONETIC_WORD_LEN]u8 = [0; MAX_PHONETIC_WORD_LEN];
    key[0] = raw[0];
    let klen: usize = 1;

    let i: usize = 1;
    while (i < n) {
        const c = raw[i];
        const prev = raw[i - 1];
        const next: u8 = if (i + 1 < n) raw[i + 1] else 0;
        const next2: u8 = if (i + 2 < n) raw[i + 2] else 0;

        if (c == 0x45 && next == 0x56) {
            // EV -> AF
            klen = appendDedup(key[0..], klen, 0x41);
            klen = appendDedup(key[0..], klen, 0x46);
            i += 2;
        } else if (isVowelAEIOU(c)) {
            klen = appendDedup(key[0..], klen, 0x41);
            i += 1;
        } else if (c == 0x51) {
            klen = appendDedup(key[0..], klen, 0x47);
            i += 1;
        } else if (c == 0x5A) {
            klen = appendDedup(key[0..], klen, 0x53);
            i += 1;
        } else if (c == 0x4D) {
            klen = appendDedup(key[0..], klen, 0x4E);
            i += 1;
        } else if (c == 0x4B) {
            if (next == 0x4E) {
                klen = appendDedup(key[0..], klen, 0x4E);
                i += 2;
            } else {
                klen = appendDedup(key[0..], klen, 0x43);
                i += 1;
            }
        } else if (c == 0x53 && next == 0x43 && next2 == 0x48) {
            klen = appendDedup(key[0..], klen, 0x53);
            i += 3;
        } else if (c == 0x50 && next == 0x48) {
            klen = appendDedup(key[0..], klen, 0x46);
            i += 2;
        } else if (c == 0x48 && (!isVowelAEIOU(prev) || !isVowelAEIOU(next))) {
            // H flanked by non-vowel -> previous char (skip)
            i += 1;
        } else if (c == 0x57 && isVowelAEIOU(prev)) {
            // W after vowel -> previous char (skip)
            i += 1;
        } else {
            klen = appendDedup(key[0..], klen, c);
            i += 1;
        }
    }

    // 4. Trailing cleanup
    if (klen > 1 && key[klen - 1] == 0x53) {
        klen -= 1;
    }
    if (klen > 2 && key[klen - 2] == 0x41 && key[klen - 1] == 0x59) {
        key[klen - 2] = 0x59;
        klen -= 1;
    }
    if (klen > 1 && key[klen - 1] == 0x41) {
        klen -= 1;
    }

    if (out.len < klen) {
        return null;
    }
    let j: usize = 0;
    while (j < klen) {
        out[j] = key[j];
        j += 1;
    }
    return klen;
}

/// Computes the Cologne Phonetics (Kölner Phonetik, Postel 1969) digit string for `word` into `out`.
/// E.g. `"Wikipedia"` -> `"3412"`, `"Breschnew"` -> `"17863"`, `"Mueller"` -> `"657"`.
/// Returns bytes written, or `null` if `word` has no ASCII letters or `out` is too small.
pub fn colognePhonetic(word: []const u8, out: []u8): usize? {
    let raw: [MAX_PHONETIC_WORD_LEN]u8 = [0; MAX_PHONETIC_WORD_LEN];
    const n = cleanUpperLetters(word, raw[0..]);
    if (n == 0) {
        return null;
    }

    // Step 1: Map each letter to 0..2 digits with adjacent deduplication.
    let tmp: [512]u8 = [0; 512];
    let tlen: usize = 0;

    let i: usize = 0;
    while (i < n) {
        const c = raw[i];
        const prev: u8 = if (i > 0) raw[i - 1] else 0;
        const next: u8 = if (i + 1 < n) raw[i + 1] else 0;

        if (isVowelAEIOUY(c) || c == 0x4A) {
            // A, E, I, J, O, U, Y -> '0'
            tlen = appendDedup(tmp[0..], tlen, 0x30);
        } else if (c == 0x48) {
            // H -> ignored
        } else if (c == 0x42) {
            tlen = appendDedup(tmp[0..], tlen, 0x31);
        } else if (c == 0x50) {
            const d: u8 = if (next == 0x48) 0x33 else 0x31;
            tlen = appendDedup(tmp[0..], tlen, d);
        } else if (c == 0x44 || c == 0x54) {
            const d: u8 = if (next == 0x43 || next == 0x53 || next == 0x5A) 0x38 else 0x32;
            tlen = appendDedup(tmp[0..], tlen, d);
        } else if (c == 0x46 || c == 0x56 || c == 0x57) {
            tlen = appendDedup(tmp[0..], tlen, 0x33);
        } else if (c == 0x47 || c == 0x4B || c == 0x51) {
            tlen = appendDedup(tmp[0..], tlen, 0x34);
        } else if (c == 0x43) {
            let d: u8 = 0x38;
            if (i == 0) {
                if (
                    next == 0x41 || next == 0x48 || next == 0x4B || next == 0x4C ||
                    next == 0x4F || next == 0x51 || next == 0x52 || next == 0x55 || next == 0x58
                ) {
                    d = 0x34;
                }
            } else if (prev != 0x53 && prev != 0x5A) {
                if (
                    next == 0x41 || next == 0x48 || next == 0x4B ||
                    next == 0x4F || next == 0x51 || next == 0x55 || next == 0x58
                ) {
                    d = 0x34;
                }
            }
            tlen = appendDedup(tmp[0..], tlen, d);
        } else if (c == 0x58) {
            if (prev == 0x43 || prev == 0x4B || prev == 0x51) {
                tlen = appendDedup(tmp[0..], tlen, 0x38);
            } else {
                tlen = appendDedup(tmp[0..], tlen, 0x34);
                tlen = appendDedup(tmp[0..], tlen, 0x38);
            }
        } else if (c == 0x4C) {
            tlen = appendDedup(tmp[0..], tlen, 0x35);
        } else if (c == 0x4D || c == 0x4E) {
            tlen = appendDedup(tmp[0..], tlen, 0x36);
        } else if (c == 0x52) {
            tlen = appendDedup(tmp[0..], tlen, 0x37);
        } else if (c == 0x53 || c == 0x5A) {
            tlen = appendDedup(tmp[0..], tlen, 0x38);
        }

        i += 1;
    }

    // Step 2: Strip all '0' except at index 0.
    let out_len: usize = 0;
    i = 0;
    while (i < tlen) {
        if (i == 0 || tmp[i] != 0x30) {
            out_len = pushByte(out, out_len, tmp[i]) ?? return null;
        }
        i += 1;
    }

    return out_len;
}
