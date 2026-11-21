// Source: Absorbed from /data/dev/bun/src/ (ISO/IEC 16388 Code 39 Zero-Alloc 3-of-9 Barcode Codec & Mod-43 Verifier)

/// Number of data characters in the ISO/IEC 16388 Code 39 alphabet (`0..9`, `A..Z`, `- . SPACE $ / + %`).
pub const CODE39_ALPHABET_LEN: usize = 43;

/// Start/Stop sentinel character (`'*'`).
pub const CODE39_START_STOP: u8 = 0x2A;

/// Number of alternating bar/space elements per Code 39 character.
pub const CODE39_ELEMENTS_PER_CHAR: usize = 9;

/// Exact number of wide elements per Code 39 character (3-of-9 invariant).
pub const CODE39_WIDE_COUNT: usize = 3;

const CODE39_ALPHABET: []const u8 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%";

/// Maps an ASCII character in the 43-character Code 39 data alphabet to its Mod-43 value (`0..42`).
/// Strictly rejects lowercase letters, `'*'`, and all unsupported characters.
pub fn code39Value(ch: u8): u8? {
    if (ch >= 0x30 && ch <= 0x39) {
        return ch - 0x30; // '0'..'9' -> 0..9
    }
    if (ch >= 0x41 && ch <= 0x5A) {
        return ch - 0x41 + 10; // 'A'..'Z' -> 10..35
    }
    switch (ch) {
        0x2D => return 36, // '-'
        0x2E => return 37, // '.'
        0x20 => return 38, // ' '
        0x24 => return 39, // '$'
        0x2F => return 40, // '/'
        0x2B => return 41, // '+'
        0x25 => return 42, // '%'
        else => return null,
    }
}

/// Maps a Mod-43 value (`0..42`) back to its canonical Code 39 ASCII character.
pub fn code39Char(val: u8): u8? {
    if ((val as usize) >= CODE39_ALPHABET_LEN) {
        return null;
    }
    return CODE39_ALPHABET[val as usize];
}

/// Returns the ISO/IEC 16388 9-bit wide/narrow element pattern for `ch` (including `'*'`).
/// Bit 8 is the leftmost bar, alternating Bar-Space-Bar-Space-Bar-Space-Bar-Space-Bar to bit 0.
/// Every valid pattern has exactly 3 wide bits (`1`) and 6 narrow bits (`0`).
pub fn code39Pattern9(ch: u8): u16? {
    switch (ch) {
        // Digits 0..9 (wide space at S1 / bit 5)
        0x30 => return 0b000110100, // '0'
        0x31 => return 0b100100001, // '1'
        0x32 => return 0b001100001, // '2'
        0x33 => return 0b101100000, // '3'
        0x34 => return 0b000110001, // '4'
        0x35 => return 0b100110000, // '5'
        0x36 => return 0b001110000, // '6'
        0x37 => return 0b000100101, // '7'
        0x38 => return 0b100100100, // '8'
        0x39 => return 0b001100100, // '9'

        // Letters A..J (wide space at S2 / bit 3)
        0x41 => return 0b100001001, // 'A'
        0x42 => return 0b001001001, // 'B'
        0x43 => return 0b101001000, // 'C'
        0x44 => return 0b000011001, // 'D'
        0x45 => return 0b100011000, // 'E'
        0x46 => return 0b001011000, // 'F'
        0x47 => return 0b000001101, // 'G'
        0x48 => return 0b100001100, // 'H'
        0x49 => return 0b001001100, // 'I'
        0x4A => return 0b000011100, // 'J'

        // Letters K..T (wide space at S3 / bit 1)
        0x4B => return 0b100000011, // 'K'
        0x4C => return 0b001000011, // 'L'
        0x4D => return 0b101000010, // 'M'
        0x4E => return 0b000010011, // 'N'
        0x4F => return 0b100010010, // 'O'
        0x50 => return 0b001010010, // 'P'
        0x51 => return 0b000000111, // 'Q'
        0x52 => return 0b100000110, // 'R'
        0x53 => return 0b001000110, // 'S'
        0x54 => return 0b000010110, // 'T'

        // Letters U..Z and '-', '.', ' ', '*' (wide space at S0 / bit 7)
        0x55 => return 0b110000001, // 'U'
        0x56 => return 0b011000001, // 'V'
        0x57 => return 0b111000000, // 'W'
        0x58 => return 0b010010001, // 'X'
        0x59 => return 0b110010000, // 'Y'
        0x5A => return 0b011010000, // 'Z'
        0x2D => return 0b010000101, // '-'
        0x2E => return 0b110000100, // '.'
        0x20 => return 0b011000100, // ' '
        0x2A => return 0b010010100, // '*'

        // Special symbols '$', '/', '+', '%' (3 wide spaces, 0 wide bars)
        0x24 => return 0b010101000, // '$'
        0x2F => return 0b010100010, // '/'
        0x2B => return 0b010001010, // '+'
        0x25 => return 0b000101010, // '%'

        else => return null,
    }
}

/// Reverse-maps a 9-bit Code 39 wide/narrow element pattern back to its ASCII character (including `'*'`).
/// Returns `null` for any invalid or unassigned bit pattern.
pub fn code39CharFromPattern9(pat9: u16): u8? {
    switch (pat9) {
        0b000110100 => return 0x30, // '0'
        0b100100001 => return 0x31, // '1'
        0b001100001 => return 0x32, // '2'
        0b101100000 => return 0x33, // '3'
        0b000110001 => return 0x34, // '4'
        0b100110000 => return 0x35, // '5'
        0b001110000 => return 0x36, // '6'
        0b000100101 => return 0x37, // '7'
        0b100100100 => return 0x38, // '8'
        0b001100100 => return 0x39, // '9'

        0b100001001 => return 0x41, // 'A'
        0b001001001 => return 0x42, // 'B'
        0b101001000 => return 0x43, // 'C'
        0b000011001 => return 0x44, // 'D'
        0b100011000 => return 0x45, // 'E'
        0b001011000 => return 0x46, // 'F'
        0b000001101 => return 0x47, // 'G'
        0b100001100 => return 0x48, // 'H'
        0b001001100 => return 0x49, // 'I'
        0b000011100 => return 0x4A, // 'J'

        0b100000011 => return 0x4B, // 'K'
        0b001000011 => return 0x4C, // 'L'
        0b101000010 => return 0x4D, // 'M'
        0b000010011 => return 0x4E, // 'N'
        0b100010010 => return 0x4F, // 'O'
        0b001010010 => return 0x50, // 'P'
        0b000000111 => return 0x51, // 'Q'
        0b100000110 => return 0x52, // 'R'
        0b001000110 => return 0x53, // 'S'
        0b000010110 => return 0x54, // 'T'

        0b110000001 => return 0x55, // 'U'
        0b011000001 => return 0x56, // 'V'
        0b111000000 => return 0x57, // 'W'
        0b010010001 => return 0x58, // 'X'
        0b110010000 => return 0x59, // 'Y'
        0b011010000 => return 0x5A, // 'Z'
        0b010000101 => return 0x2D, // '-'
        0b110000100 => return 0x2E, // '.'
        0b011000100 => return 0x20, // ' '
        0b010010100 => return 0x2A, // '*'

        0b010101000 => return 0x24, // '$'
        0b010100010 => return 0x2F, // '/'
        0b010001010 => return 0x2B, // '+'
        0b000101010 => return 0x25, // '%'

        else => return null,
    }
}

/// Computes the ISO/IEC 16388 Mod-43 check character for `text`.
/// Returns `null` if `text` is empty or contains any character outside the 43-char alphabet.
pub fn code39CheckDigit(text: []const u8): u8? {
    if (text.len == 0) {
        return null;
    }
    let sum: u32 = 0;
    let i: usize = 0;
    while (i < text.len) {
        const v = code39Value(text[i]) ?? return null;
        sum += v as u32;
        i += 1;
    }
    return code39Char((sum % 43) as u8);
}

/// Verifies that `text_with_check` (at least 2 characters) ends with a valid Mod-43 check character.
pub fn verifyCode39Mod43(text_with_check: []const u8): bool {
    if (text_with_check.len < 2) {
        return false;
    }
    const expected = code39CheckDigit(text_with_check[0..(text_with_check.len - 1)]) ?? return false;
    return expected == text_with_check[text_with_check.len - 1];
}

fn emitPatternModules(pat9: u16, wide_ratio: u8, out: []u8, pos: usize): usize? {
    let p = pos;
    let elem: usize = 0;
    while (elem < CODE39_ELEMENTS_PER_CHAR) {
        const shift: u4 = (8 - elem) as u4;
        const is_wide = ((pat9 >> shift) & 1) != 0;
        const width: usize = if (is_wide) wide_ratio as usize else 1;
        const module_val: u8 = if ((elem & 1) == 0) 1 else 0; // Bar=1, Space=0

        if (out.len - p < width) {
            return null;
        }
        let k: usize = 0;
        while (k < width) {
            out[p] = module_val;
            p += 1;
            k += 1;
        }
        elem += 1;
    }
    return p;
}

/// Encodes `text` into a Code 39 `0`/`1` module stream inside `out_modules`.
/// - Supports `wide_ratio == 2` or `wide_ratio == 3`.
/// - Automatically wraps with leading and trailing `'*'` start/stop sentinels and 1-module narrow white gaps.
/// - Optionally appends the Mod-43 check character when `with_mod43 == true`.
/// - Returns total modules written, or `null` on invalid parameters, unsupported chars, or buffer overflow.
pub fn encodeCode39Modules(text: []const u8, with_mod43: bool, wide_ratio: u8, out_modules: []u8): usize? {
    if (wide_ratio != 2 && wide_ratio != 3) {
        return null;
    }

    let check_ch: u8 = 0;
    if (with_mod43) {
        check_ch = code39CheckDigit(text) ?? return null;
    } else {
        // Pre-validate every character before writing.
        let v_idx: usize = 0;
        while (v_idx < text.len) {
            * = code39Value(text[v_idx]) ?? return null;
            v_idx += 1;
        }
    }

    const star_pat = code39Pattern9(CODE39_START_STOP) ?? return null;
    let pos = emitPatternModules(star_pat, wide_ratio, out_modules, 0) ?? return null;

    let i: usize = 0;
    while (i < text.len) {
        if (out_modules.len - pos < 1) {
            return null;
        }
        out_modules[pos] = 0; // Inter-character narrow white gap
        pos += 1;

        const pat = code39Pattern9(text[i]) ?? return null;
        pos = emitPatternModules(pat, wide_ratio, out_modules, pos) ?? return null;
        i += 1;
    }

    if (with_mod43) {
        if (out_modules.len - pos < 1) {
            return null;
        }
        out_modules[pos] = 0;
        pos += 1;

        const chk_pat = code39Pattern9(check_ch) ?? return null;
        pos = emitPatternModules(chk_pat, wide_ratio, out_modules, pos) ?? return null;
    }

    // Trailing gap + Stop sentinel '*'
    if (out_modules.len - pos < 1) {
        return null;
    }
    out_modules[pos] = 0;
    pos += 1;

    pos = emitPatternModules(star_pat, wide_ratio, out_modules, pos) ?? return null;
    return pos;
}

/// Decodes a Code 39 `0`/`1` module stream into `out_text` with zero heap allocation.
/// - Automatically trims leading/trailing quiet zones (`0`) and adapts to narrow/wide run thresholds.
/// - Strictly verifies start/stop `'*'` sentinels, inter-character gaps, and optional Mod-43 check digit.
/// - Returns decoded text length (excluding sentinels and optional check digit), or `null` on error.
pub fn decodeCode39Modules(modules: []const u8, with_mod43: bool, out_text: []u8): usize? {
    // Validate binary module values and locate active [start..end) span.
    let k: usize = 0;
    while (k < modules.len) {
        if (modules[k] != 0 && modules[k] != 1) {
            return null;
        }
        k += 1;
    }

    let start: usize = 0;
    while (start < modules.len && modules[start] == 0) {
        start += 1;
    }
    let end: usize = modules.len;
    while (end > start && modules[end - 1] == 0) {
        end -= 1;
    }
    if (start >= end) {
        return null;
    }

    let pos: usize = start;
    let char_count: usize = 0;
    let out_len: usize = 0;
    let mod43_sum: u32 = 0;
    let has_pending = false;
    let pending_ch: u8 = 0;
    let saw_stop = false;

    while (pos < end) {
        // Read 9 alternating runs: Bar(1), Space(0), ..., Bar(1).
        let runs: [9]usize = [0, 0, 0, 0, 0, 0, 0, 0, 0];
        let min_run: usize = 0;
        let max_run: usize = 0;

        let elem: usize = 0;
        while (elem < CODE39_ELEMENTS_PER_CHAR) {
            const expected_val: u8 = if ((elem & 1) == 0) 1 else 0;
            if (pos >= end || modules[pos] != expected_val) {
                return null;
            }
            const run_start = pos;
            while (pos < end && modules[pos] == expected_val) {
                pos += 1;
            }
            const run_len = pos - run_start;
            runs[elem] = run_len;
            if (elem == 0 || run_len < min_run) {
                min_run = run_len;
            }
            if (run_len > max_run) {
                max_run = run_len;
            }
            elem += 1;
        }

        if (min_run == 0 || max_run < min_run * 2) {
            return null;
        }

        // Threshold midpoint classifies narrow (<= thresh) vs wide (> thresh).
        const thresh2 = min_run + max_run;
        let pat9: u16 = 0;
        elem = 0;
        while (elem < CODE39_ELEMENTS_PER_CHAR) {
            pat9 <<= 1;
            if (runs[elem] * 2 > thresh2) {
                pat9 |= 1;
            }
            elem += 1;
        }

        const ch = code39CharFromPattern9(pat9) ?? return null;
        const is_last = pos == end;

        if (char_count == 0) {
            if (ch != CODE39_START_STOP || is_last) {
                return null;
            }
        } else if (is_last) {
            if (ch != CODE39_START_STOP) {
                return null;
            }
            saw_stop = true;
        } else {
            if (ch == CODE39_START_STOP) {
                return null;
            }
            if (with_mod43) {
                if (has_pending) {
                    if (out_len >= out_text.len) {
                        return null;
                    }
                    out_text[out_len] = pending_ch;
                    out_len += 1;
                    const pv = code39Value(pending_ch) ?? return null;
                    mod43_sum += pv as u32;
                }
                pending_ch = ch;
                has_pending = true;
            } else {
                if (out_len >= out_text.len) {
                    return null;
                }
                out_text[out_len] = ch;
                out_len += 1;
            }
        }

        char_count += 1;

        if (!is_last) {
            // Consume inter-character white space gap (must be 0s followed by next bar).
            if (modules[pos] != 0) {
                return null;
            }
            while (pos < end && modules[pos] == 0) {
                pos += 1;
            }
            if (pos >= end) {
                return null;
            }
        }
    }

    if (!saw_stop || char_count < 2) {
        return null;
    }

    if (with_mod43) {
        if (!has_pending || out_len == 0) {
            return null;
        }
        const expected_chk = code39Char((mod43_sum % 43) as u8) ?? return null;
        if (pending_ch != expected_chk) {
            return null;
        }
    }

    return out_len;
}
