// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Canonical Roman Numerals 1..3999 & Bijective Base-26 Spreadsheet Columns)

/// Minimum representable canonical Roman numeral value.
pub const ROMAN_MIN: u16 = 1;

/// Maximum representable canonical Roman numeral value.
pub const ROMAN_MAX: u16 = 3999;

/// Maximum length of a canonical Roman numeral string (`"MMMDCCCLXXXVIII"` = 15 bytes).
pub const ROMAN_MAX_STR_LEN: usize = 15;

/// Maximum length of a 32-bit Bijective Base-26 column label (`"FXSHRXW"` = 7 bytes for `u32::MAX`).
pub const MAX_COL_LETTERS_LEN: usize = 7;

fn romanCharValue(ch: u8): i32? {
    let up = ch;
    if (up >= 0x61 && up <= 0x7A) {
        up -= 32;
    }
    switch (up) {
        0x49 => return 1,    // I
        0x56 => return 5,    // V
        0x58 => return 10,   // X
        0x4C => return 50,   // L
        0x43 => return 100,  // C
        0x44 => return 500,  // D
        0x4D => return 1000, // M
        else => return null,
    }
}

fn appendStr(out: []u8, pos: usize, s: []const u8): usize? {
    if (out.len - pos < s.len) {
        return null;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos + i] = s[i];
        i += 1;
    }
    return pos + s.len;
}

/// Encodes an integer `n` in `1..3999` into its canonical uppercase Roman numeral string in `out`.
/// Returns bytes written, or `null` if `n == 0`, `n > 3999`, or `out` is too small.
pub fn romanEncode(n: u16, out: []u8): usize? {
    if (n < ROMAN_MIN || n > ROMAN_MAX) {
        return null;
    }

    const vals: [13]u16 = [1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1];
    const syms: [13][]const u8 = ["M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I"];

    let rem = n;
    let pos: usize = 0;
    let i: usize = 0;
    while (i < 13 && rem > 0) {
        while (rem >= vals[i]) {
            pos = appendStr(out, pos, syms[i]) ?? return null;
            rem -= vals[i];
        }
        i += 1;
    }
    return pos;
}

/// Encodes an integer `n` in `1..3999` into its canonical lowercase Roman numeral string (e.g. `"xiv"`) in `out`.
/// Returns bytes written, or `null` if `n` is out of range or `out` is too small.
pub fn romanEncodeLower(n: u16, out: []u8): usize? {
    const len = romanEncode(n, out) ?? return null;
    let i: usize = 0;
    while (i < len) {
        out[i] += 32;
        i += 1;
    }
    return len;
}

/// Leniently decodes a Roman numeral string (case-insensitive) into `1..3999`.
/// Accepts traditional/clockface additive forms such as `"IIII"` (4) or `"VIIII"` (9).
/// Rejects empty strings, non-Roman characters, and values outside `1..3999`.
pub fn romanDecodeLenient(s: []const u8): u16? {
    if (s.len == 0) {
        return null;
    }

    let total: i32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const cur = romanCharValue(s[i]) ?? return null;
        if (i + 1 < s.len) {
            const next = romanCharValue(s[i + 1]) ?? return null;
            if (cur < next) {
                total -= cur;
            } else {
                total += cur;
            }
        } else {
            total += cur;
        }
        i += 1;
    }

    if (total < (ROMAN_MIN as i32) || total > (ROMAN_MAX as i32)) {
        return null;
    }
    return total as u16;
}

/// Strictly decodes a canonical Roman numeral string (case-insensitive) into `1..3999`.
/// Guarantees 100% canonical compliance by verifying exact roundtrip against `romanEncode`,
/// rejecting all non-canonical forms such as `"IIII"`, `"VV"`, `"IC"`, `"IL"`, `"IIV"`, `"VX"`, `"XM"`, `"MMMM"`.
pub fn romanDecode(s: []const u8): u16? {
    if (s.len == 0 || s.len > ROMAN_MAX_STR_LEN) {
        return null;
    }

    const val = romanDecodeLenient(s) ?? return null;

    let canon: [ROMAN_MAX_STR_LEN]u8 = [0; ROMAN_MAX_STR_LEN];
    const clen = romanEncode(val, canon[0..]) ?? return null;
    if (clen != s.len) {
        return null;
    }

    let i: usize = 0;
    while (i < clen) {
        let up = s[i];
        if (up >= 0x61 && up <= 0x7A) {
            up -= 32;
        }
        if (up != canon[i]) {
            return null;
        }
        i += 1;
    }

    return val;
}

/// Returns `true` if `s` is a strictly canonical Roman numeral in `1..3999` (case-insensitive).
pub fn isValidRoman(s: []const u8): bool {
    return romanDecode(s) != null;
}

/// Converts a 1-based spreadsheet column index (`1 -> "A"`, `26 -> "Z"`, `27 -> "AA"`,
/// `702 -> "ZZ"`, `703 -> "AAA"`, `16384 -> "XFD"`) into uppercase Bijective Base-26 letters in `out`.
/// Returns bytes written, or `null` if `col_1based == 0` or `out` is too small.
pub fn colIndexToLetters(col_1based: u32, out: []u8): usize? {
    if (col_1based == 0) {
        return null;
    }

    let tmp: [8]u8 = [0; 8];
    let len: usize = 0;
    let n: u32 = col_1based;

    while (n > 0) {
        n -= 1;
        tmp[len] = (0x41 + (n % 26)) as u8;
        n /= 26;
        len += 1;
    }

    if (out.len < len) {
        return null;
    }

    let i: usize = 0;
    while (i < len) {
        out[i] = tmp[len - 1 - i];
        i += 1;
    }
    return len;
}

/// Converts a Bijective Base-26 spreadsheet column label (case-insensitive, e.g. `"A"`, `"XFD"`)
/// into its 1-based `u32` column index.
/// Rejects empty strings, non-ASCII letters, and values overflowing `u32::MAX` (`"FXSHRXW"`).
pub fn lettersToColIndex(letters: []const u8): u32? {
    if (letters.len == 0) {
        return null;
    }

    let acc: u64 = 0;
    let i: usize = 0;
    while (i < letters.len) {
        let up = letters[i];
        if (up >= 0x61 && up <= 0x7A) {
            up -= 32;
        }
        if (up < 0x41 || up > 0x5A) {
            return null;
        }
        const digit: u64 = (up - 0x41 + 1) as u64;
        acc = acc * 26 + digit;
        if (acc > 0xFFFFFFFF) {
            return null;
        }
        i += 1;
    }

    return acc as u32;
}
