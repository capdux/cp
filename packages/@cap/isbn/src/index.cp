// packages/@cap/isbn/src/index.cp
// Zero-allocation ISO 2108 ISBN-10 / ISBN-13 / ISSN / EAN-13 validator & converter.
// MIT clean-room pure CP implementation.

fn isbnToUpper(c: u8): u8 {
    if (c >= ('a' as u8) && c <= ('z' as u8)) {
        return c - (32 as u8);
    }
    return c;
}

fn isbnIsDigit(c: u8): bool {
    return c >= ('0' as u8) && c <= ('9' as u8);
}

/// Strip '-' and ' ' characters and uppercase ASCII letters into `out`.
/// Returns number of bytes written (capped at `out.len`).
pub fn stripHyphens(input: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const c = input[i];
        if (c != ('-' as u8) && c != (' ' as u8)) {
            if (pos >= out.len) {
                break;
            }
            out[pos] = isbnToUpper(c);
            pos += 1;
        }
        i += 1;
    }
    return pos;
}

/// Internal helper that normalizes `input` into `out` and rejects inputs whose
/// normalized length exceeds `expected_len` (returns -1 on length mismatch).
fn isbnNormalizeExact(input: string, expected_len: usize, out: []u8): bool {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const c = input[i];
        if (c != ('-' as u8) && c != (' ' as u8)) {
            if (pos >= expected_len || pos >= out.len) {
                return false;
            }
            out[pos] = isbnToUpper(c);
            pos += 1;
        }
        i += 1;
    }
    return pos == expected_len;
}

/// Compute the ISBN-13 / EAN-13 check digit ('0'..'9') for a 12-digit prefix.
/// Returns `0` (NUL byte) if `prefix12` does not normalize to 12 ASCII digits.
pub fn computeIsbn13CheckDigit(prefix12: string): u8 {
    let clean: [12]u8 = undefined;
    if (!isbnNormalizeExact(prefix12, 12, clean[0..12])) {
        return 0 as u8;
    }
    let sum: u32 = 0 as u32;
    let i: usize = 0;
    while (i < 12) {
        if (!isbnIsDigit(clean[i])) {
            return 0 as u8;
        }
        const d = (clean[i] - ('0' as u8)) as u32;
        if ((i % 2) == 0) {
            sum += d;
        } else {
            sum += d * (3 as u32);
        }
        i += 1;
    }
    const rem = sum % (10 as u32);
    const chk = ((10 as u32) - rem) % (10 as u32);
    return ('0' as u8) + (chk as u8);
}

/// Validate an ISBN-10 string (weighted modulo 11; trailing 'X'/'x' allowed).
pub fn isValidIsbn10(input: string): bool {
    let clean: [10]u8 = undefined;
    if (!isbnNormalizeExact(input, 10, clean[0..10])) {
        return false;
    }
    let sum: u32 = 0 as u32;
    let i: usize = 0;
    while (i < 10) {
        const c = clean[i];
        let val: u32 = 0 as u32;
        if (isbnIsDigit(c)) {
            val = (c - ('0' as u8)) as u32;
        } else if (i == 9 && c == ('X' as u8)) {
            val = 10 as u32;
        } else {
            return false;
        }
        const weight = (10 - i) as u32;
        sum += val * weight;
        i += 1;
    }
    return (sum % (11 as u32)) == (0 as u32);
}

/// Validate a general 13-digit EAN-13 / GTIN-13 barcode (modulo 10 check).
pub fn isValidEan13(input: string): bool {
    let clean: [13]u8 = undefined;
    if (!isbnNormalizeExact(input, 13, clean[0..13])) {
        return false;
    }
    let sum: u32 = 0 as u32;
    let i: usize = 0;
    while (i < 13) {
        if (!isbnIsDigit(clean[i])) {
            return false;
        }
        const d = (clean[i] - ('0' as u8)) as u32;
        if ((i % 2) == 0) {
            sum += d;
        } else {
            sum += d * (3 as u32);
        }
        i += 1;
    }
    return (sum % (10 as u32)) == (0 as u32);
}

/// Validate an ISBN-13 string (requires 978 or 979 Bookland prefix + EAN-13 check).
pub fn isValidIsbn13(input: string): bool {
    let clean: [13]u8 = undefined;
    if (!isbnNormalizeExact(input, 13, clean[0..13])) {
        return false;
    }
    if (clean[0] != ('9' as u8) || clean[1] != ('7' as u8)) {
        return false;
    }
    if (clean[2] != ('8' as u8) && clean[2] != ('9' as u8)) {
        return false;
    }
    let sum: u32 = 0 as u32;
    let i: usize = 0;
    while (i < 13) {
        if (!isbnIsDigit(clean[i])) {
            return false;
        }
        const d = (clean[i] - ('0' as u8)) as u32;
        if ((i % 2) == 0) {
            sum += d;
        } else {
            sum += d * (3 as u32);
        }
        i += 1;
    }
    return (sum % (10 as u32)) == (0 as u32);
}

/// Validate an 8-character ISSN (weighted modulo 11; trailing 'X'/'x' allowed).
pub fn isValidIssn(input: string): bool {
    let clean: [8]u8 = undefined;
    if (!isbnNormalizeExact(input, 8, clean[0..8])) {
        return false;
    }
    let sum: u32 = 0 as u32;
    let i: usize = 0;
    while (i < 8) {
        const c = clean[i];
        let val: u32 = 0 as u32;
        if (isbnIsDigit(c)) {
            val = (c - ('0' as u8)) as u32;
        } else if (i == 7 && c == ('X' as u8)) {
            val = 10 as u32;
        } else {
            return false;
        }
        const weight = (8 - i) as u32;
        sum += val * weight;
        i += 1;
    }
    return (sum % (11 as u32)) == (0 as u32);
}

/// Validate whether `input` is a valid ISBN-10 or ISBN-13.
pub fn isValidIsbn(input: string): bool {
    return isValidIsbn10(input) || isValidIsbn13(input);
}

/// Convert a valid ISBN-10 into an unhyphenated 13-digit ISBN-13 (with `978` prefix).
/// Returns `13` on success, or `0` on invalid input or insufficient `out` buffer.
pub fn isbn10To13(isbn10: string, out: []u8): usize {
    if (out.len < 13 || !isValidIsbn10(isbn10)) {
        return 0;
    }
    let clean: [10]u8 = undefined;
    if (!isbnNormalizeExact(isbn10, 10, clean[0..10])) {
        return 0;
    }
    out[0] = '9' as u8;
    out[1] = '7' as u8;
    out[2] = '8' as u8;
    let i: usize = 0;
    while (i < 9) {
        out[3 + i] = clean[i];
        i += 1;
    }
    const chk = computeIsbn13CheckDigit(out[0..12]);
    if (chk == (0 as u8)) {
        return 0;
    }
    out[12] = chk;
    return 13;
}

/// Convert a valid `978`-prefixed ISBN-13 into an unhyphenated 10-character ISBN-10.
/// `979`-prefixed ISBN-13 codes have no ISBN-10 equivalent and return `0`.
/// Returns `10` on success, or `0` on invalid input or insufficient `out` buffer.
pub fn isbn13To10(isbn13: string, out: []u8): usize {
    if (out.len < 10 || !isValidIsbn13(isbn13)) {
        return 0;
    }
    let clean: [13]u8 = undefined;
    if (!isbnNormalizeExact(isbn13, 13, clean[0..13])) {
        return 0;
    }
    // Only 978-prefixed ISBN-13 can be converted to ISBN-10.
    if (clean[0] != ('9' as u8) || clean[1] != ('7' as u8) || clean[2] != ('8' as u8)) {
        return 0;
    }
    let sum: u32 = 0 as u32;
    let i: usize = 0;
    while (i < 9) {
        const c = clean[3 + i];
        out[i] = c;
        const d = (c - ('0' as u8)) as u32;
        sum += d * ((10 - i) as u32);
        i += 1;
    }
    const rem = sum % (11 as u32);
    const chk = ((11 as u32) - rem) % (11 as u32);
    if (chk == (10 as u32)) {
        out[9] = 'X' as u8;
    } else {
        out[9] = ('0' as u8) + (chk as u8);
    }
    return 10;
}

export {
    stripHyphens,
    isValidIsbn10,
    isValidIsbn13,
    isValidIssn,
    isValidEan13,
    isValidIsbn,
    isbn10To13,
    isbn13To10,
    computeIsbn13CheckDigit,
};
