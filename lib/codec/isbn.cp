// Origin: Pure-CP Stdlib (Zero-Alloc ISO 2108 ISBN-10/13, ISO 3297 ISSN & GS1 EAN/UPC Codec) | Created: 2026-11-22

/// Standard lengths of canonical unformatted/formatted identifiers.
pub const ISBN10_LEN: usize = 10;
pub const ISBN13_LEN: usize = 13;
pub const ISSN_LEN: usize = 8;
pub const ISSN_FORMATTED_LEN: usize = 9;
pub const EAN13_LEN: usize = 13;
pub const EAN8_LEN: usize = 8;
pub const UPCA_LEN: usize = 12;

fn isSep(ch: u8): bool {
    return ch == 0x2D || ch == 0x20; // '-' or ' '
}

fn isDigit(ch: u8): bool {
    return ch >= 0x30 && ch <= 0x39;
}

fn isIsbn13Prefix(d0: u8, d1: u8, d2: u8): bool {
    return d0 == 0x39 && d1 == 0x37 && (d2 == 0x38 || d2 == 0x39); // "978" or "979"
}

fn gs1Mod10CheckFromClean(digits: []const u8): u8 {
    let sum: u32 = 0;
    let i: usize = 0;
    while (i < digits.len) {
        const d = (digits[i] - 0x30) as u32;
        const from_right = digits.len - 1 - i;
        if ((from_right & 1) == 0) {
            sum += d * 3;
        } else {
            sum += d;
        }
        i += 1;
    }
    const rem = sum % 10;
    const chk = if (rem == 0) 0 else 10 - rem;
    return (0x30 + chk) as u8;
}

/// Computes the generic GS1 Mod-10 check digit for `body_digits` (ignoring `'-'` and `' '`).
/// Weights alternate 3 and 1 starting from the rightmost digit (position 1 from right = x3).
/// Supports EAN-13 (12 body digits), EAN-8 (7), UPC-A (11), GTIN-14 (13), etc.
/// Returns `null` if empty or if any non-digit, non-separator byte is encountered.
pub fn eanCheckDigit(body_digits: []const u8): u8? {
    // First pass: count clean digits and validate every character.
    let count: usize = 0;
    let i: usize = 0;
    while (i < body_digits.len) {
        const ch = body_digits[i];
        i += 1;
        if (isSep(ch)) {
            continue;
        }
        if (!isDigit(ch)) {
            return null;
        }
        count += 1;
    }
    if (count == 0) {
        return null;
    }

    // Second pass: accumulate weighted sum without allocating a buffer.
    let sum: u32 = 0;
    let idx: usize = 0;
    i = 0;
    while (i < body_digits.len) {
        const ch = body_digits[i];
        i += 1;
        if (isSep(ch)) {
            continue;
        }
        const d = (ch - 0x30) as u32;
        const from_right = count - 1 - idx;
        if ((from_right & 1) == 0) {
            sum += d * 3;
        } else {
            sum += d;
        }
        idx += 1;
    }

    const rem = sum % 10;
    const chk = if (rem == 0) 0 else 10 - rem;
    return (0x30 + chk) as u8;
}

/// Computes the ISO 2108 ISBN-10 Mod-11 check digit (`'0'..'9'` or `'X'`) from 9 body digits.
/// Ignores `'-'` and `' '`. Returns `null` if cleaned length != 9 or on invalid characters.
pub fn isbn10CheckDigit(digits9: []const u8): u8? {
    let sum: u32 = 0;
    let count: usize = 0;
    let i: usize = 0;

    while (i < digits9.len) {
        const ch = digits9[i];
        i += 1;
        if (isSep(ch)) {
            continue;
        }
        if (!isDigit(ch) || count >= 9) {
            return null;
        }
        const weight = (10 - count) as u32;
        sum += ((ch - 0x30) as u32) * weight;
        count += 1;
    }

    if (count != 9) {
        return null;
    }

    const chk = (11 - (sum % 11)) % 11;
    if (chk == 10) {
        return 0x58; // 'X'
    }
    return (0x30 + chk) as u8;
}

/// Validates an ISO 2108 ISBN-10 string (ignoring `'-'` and `' '`, accepting `'X'` or `'x'` as check digit).
pub fn isValidIsbn10(code: []const u8): bool {
    let sum: u32 = 0;
    let count: usize = 0;
    let i: usize = 0;

    while (i < code.len) {
        const ch = code[i];
        i += 1;
        if (isSep(ch)) {
            continue;
        }
        if (count >= 10) {
            return false;
        }
        let val: u32 = 0;
        if (isDigit(ch)) {
            val = (ch - 0x30) as u32;
        } else if (count == 9 && (ch == 0x58 || ch == 0x78)) {
            val = 10;
        } else {
            return false;
        }
        sum += val * ((10 - count) as u32);
        count += 1;
    }

    return count == 10 && (sum % 11) == 0;
}

/// Computes the ISO 2108 ISBN-13 check digit (`'0'..'9'`) from 12 body digits (must start with `"978"` or `"979"`).
/// Ignores `'-'` and `' '`. Returns `null` if cleaned length != 12, invalid prefix, or invalid characters.
pub fn isbn13CheckDigit(digits12: []const u8): u8? {
    let clean: [12]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let count: usize = 0;
    let i: usize = 0;

    while (i < digits12.len) {
        const ch = digits12[i];
        i += 1;
        if (isSep(ch)) {
            continue;
        }
        if (!isDigit(ch) || count >= 12) {
            return null;
        }
        clean[count] = ch;
        count += 1;
    }

    if (count != 12 || !isIsbn13Prefix(clean[0], clean[1], clean[2])) {
        return null;
    }

    return gs1Mod10CheckFromClean(clean[0..12]);
}

/// Validates an ISO 2108 ISBN-13 string (`"978"` or `"979"` prefix + valid GS1 Mod-10 check digit).
pub fn isValidIsbn13(code: []const u8): bool {
    let clean: [13]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let count: usize = 0;
    let i: usize = 0;

    while (i < code.len) {
        const ch = code[i];
        i += 1;
        if (isSep(ch)) {
            continue;
        }
        if (!isDigit(ch) || count >= 13) {
            return false;
        }
        clean[count] = ch;
        count += 1;
    }

    if (count != 13 || !isIsbn13Prefix(clean[0], clean[1], clean[2])) {
        return false;
    }

    return gs1Mod10CheckFromClean(clean[0..12]) == clean[12];
}

/// Returns `true` if `code` is a valid ISBN-10 or ISBN-13.
pub fn isValidIsbn(code: []const u8): bool {
    return isValidIsbn10(code) || isValidIsbn13(code);
}

/// Losslessly converts a valid ISBN-10 into its 13-byte unformatted `"978"` ISBN-13 representation in `out`.
/// Returns `13` on success, or `null` if `isbn10` is invalid or `out.len < 13`.
pub fn isbn10To13(isbn10: []const u8, out: []u8): usize? {
    if (out.len < ISBN13_LEN || !isValidIsbn10(isbn10)) {
        return null;
    }

    out[0] = 0x39; // '9'
    out[1] = 0x37; // '7'
    out[2] = 0x38; // '8'

    let written: usize = 3;
    let i: usize = 0;
    while (i < isbn10.len && written < 12) {
        const ch = isbn10[i];
        i += 1;
        if (isSep(ch)) {
            continue;
        }
        out[written] = ch;
        written += 1;
    }

    out[12] = gs1Mod10CheckFromClean(out[0..12]);
    return ISBN13_LEN;
}

/// Converts a valid `"978"`-prefixed ISBN-13 into its 10-byte unformatted ISBN-10 representation in `out`.
/// Strictly rejects `"979"`-prefixed ISBN-13 (which have no ISBN-10 equivalent), invalid inputs, or `out.len < 10`.
pub fn isbn13To10(isbn13: []const u8, out: []u8): usize? {
    if (out.len < ISBN10_LEN || !isValidIsbn13(isbn13)) {
        return null;
    }

    let clean: [13]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let count: usize = 0;
    let i: usize = 0;
    while (i < isbn13.len) {
        const ch = isbn13[i];
        i += 1;
        if (isSep(ch)) {
            continue;
        }
        clean[count] = ch;
        count += 1;
    }

    // Only "978" Bookland prefix maps back to ISBN-10 ("979" does not).
    if (clean[0] != 0x39 || clean[1] != 0x37 || clean[2] != 0x38) {
        return null;
    }

    let k: usize = 0;
    while (k < 9) {
        out[k] = clean[3 + k];
        k += 1;
    }

    const chk = isbn10CheckDigit(out[0..9]) ?? return null;
    out[9] = chk;
    return ISBN10_LEN;
}

/// Computes the ISO 3297 ISSN Mod-11 check digit (`'0'..'9'` or `'X'`) from 7 body digits.
/// Ignores `'-'` and `' '`. Returns `null` if cleaned length != 7 or on invalid characters.
pub fn issnCheckDigit(digits7: []const u8): u8? {
    let sum: u32 = 0;
    let count: usize = 0;
    let i: usize = 0;

    while (i < digits7.len) {
        const ch = digits7[i];
        i += 1;
        if (isSep(ch)) {
            continue;
        }
        if (!isDigit(ch) || count >= 7) {
            return null;
        }
        const weight = (8 - count) as u32;
        sum += ((ch - 0x30) as u32) * weight;
        count += 1;
    }

    if (count != 7) {
        return null;
    }

    const chk = (11 - (sum % 11)) % 11;
    if (chk == 10) {
        return 0x58; // 'X'
    }
    return (0x30 + chk) as u8;
}

/// Validates an ISO 3297 8-character ISSN (ignoring `'-'` and `' '`, accepting `'X'` or `'x'` as check digit).
pub fn isValidIssn(code: []const u8): bool {
    let sum: u32 = 0;
    let count: usize = 0;
    let i: usize = 0;

    while (i < code.len) {
        const ch = code[i];
        i += 1;
        if (isSep(ch)) {
            continue;
        }
        if (count >= 8) {
            return false;
        }
        let val: u32 = 0;
        if (isDigit(ch)) {
            val = (ch - 0x30) as u32;
        } else if (count == 7 && (ch == 0x58 || ch == 0x78)) {
            val = 10;
        } else {
            return false;
        }
        sum += val * ((8 - count) as u32);
        count += 1;
    }

    return count == 8 && (sum % 11) == 0;
}

/// Validates and formats an ISSN into canonical 9-byte `XXXX-XXXX` (uppercasing `'x'` -> `'X'`) inside `out`.
/// Returns `9` on success, or `null` if `code` is invalid or `out.len < 9`.
pub fn formatIssn(code: []const u8, out: []u8): usize? {
    if (out.len < ISSN_FORMATTED_LEN || !isValidIssn(code)) {
        return null;
    }

    let clean: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 0];
    let count: usize = 0;
    let i: usize = 0;
    while (i < code.len) {
        const ch = code[i];
        i += 1;
        if (isSep(ch)) {
            continue;
        }
        clean[count] = if (ch == 0x78) 0x58 else ch;
        count += 1;
    }

    out[0] = clean[0];
    out[1] = clean[1];
    out[2] = clean[2];
    out[3] = clean[3];
    out[4] = 0x2D; // '-'
    out[5] = clean[4];
    out[6] = clean[5];
    out[7] = clean[6];
    out[8] = clean[7];
    return ISSN_FORMATTED_LEN;
}

fn isValidGs1Fixed(code: []const u8, expected_len: usize): bool {
    let clean: [14]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let count: usize = 0;
    let i: usize = 0;

    while (i < code.len) {
        const ch = code[i];
        i += 1;
        if (isSep(ch)) {
            continue;
        }
        if (!isDigit(ch) || count >= expected_len) {
            return false;
        }
        clean[count] = ch;
        count += 1;
    }

    if (count != expected_len) {
        return false;
    }

    return gs1Mod10CheckFromClean(clean[0..(expected_len - 1)]) == clean[expected_len - 1];
}

/// Validates a GS1 EAN-13 (13-digit) retail barcode (ignoring `'-'` and `' '`).
pub fn isValidEan13(code: []const u8): bool {
    return isValidGs1Fixed(code, EAN13_LEN);
}

/// Validates a GS1 EAN-8 (8-digit) retail barcode (ignoring `'-'` and `' '`).
pub fn isValidEan8(code: []const u8): bool {
    return isValidGs1Fixed(code, EAN8_LEN);
}

/// Validates a GS1 UPC-A (12-digit) retail barcode (ignoring `'-'` and `' '`).
pub fn isValidUpcA(code: []const u8): bool {
    return isValidGs1Fixed(code, UPCA_LEN);
}
