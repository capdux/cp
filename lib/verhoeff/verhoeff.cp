// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Verhoeff D5, Damm Quasigroup & ISO/IEC 7064 Mod-11-2 / Mod-37-2 Check Digits)

// Verhoeff Dihedral group D5 multiplication table (10x10 flattened).
const VERHOEFF_D: [100]u8 = [
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    1, 2, 3, 4, 0, 6, 7, 8, 9, 5,
    2, 3, 4, 0, 1, 7, 8, 9, 5, 6,
    3, 4, 0, 1, 2, 8, 9, 5, 6, 7,
    4, 0, 1, 2, 3, 9, 5, 6, 7, 8,
    5, 9, 8, 7, 6, 0, 4, 3, 2, 1,
    6, 5, 9, 8, 7, 1, 0, 4, 3, 2,
    7, 6, 5, 9, 8, 2, 1, 0, 4, 3,
    8, 7, 6, 5, 9, 3, 2, 1, 0, 4,
    9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
];

// Verhoeff permutation table P[pos % 8][digit] (8x10 flattened).
const VERHOEFF_P: [80]u8 = [
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    1, 5, 7, 6, 2, 8, 3, 0, 9, 4,
    5, 8, 0, 3, 7, 9, 6, 1, 4, 2,
    8, 9, 1, 6, 0, 4, 3, 5, 2, 7,
    9, 4, 5, 3, 1, 2, 6, 8, 7, 0,
    4, 2, 8, 6, 5, 7, 3, 9, 0, 1,
    2, 7, 9, 3, 8, 0, 6, 4, 1, 5,
    7, 0, 4, 6, 9, 1, 3, 2, 5, 8,
];

// Verhoeff D5 inverse table.
const VERHOEFF_INV: [10]u8 = [0, 4, 3, 2, 1, 5, 6, 7, 8, 9];

// Damm order-10 totally anti-symmetric quasigroup table (H. Michael Damm, 2004, 10x10 flattened).
const DAMM_TABLE: [100]u8 = [
    0, 3, 1, 7, 5, 9, 8, 6, 4, 2,
    7, 0, 9, 2, 1, 5, 4, 8, 6, 3,
    4, 2, 0, 6, 8, 7, 1, 3, 5, 9,
    1, 7, 5, 0, 9, 8, 3, 4, 2, 6,
    6, 1, 2, 3, 0, 4, 5, 9, 7, 8,
    3, 6, 7, 4, 2, 0, 9, 5, 8, 1,
    5, 8, 6, 9, 7, 2, 0, 1, 3, 4,
    8, 9, 4, 5, 3, 6, 2, 0, 1, 7,
    9, 4, 3, 8, 6, 1, 7, 2, 0, 5,
    2, 5, 8, 1, 4, 3, 6, 7, 9, 0,
];

fn isDigit(ch: u8): bool {
    return ch >= '0' && ch <= '9';
}

fn isSeparator(ch: u8): bool {
    return ch == '-' || ch == ' ';
}

fn alnumVal(ch: u8): u8? {
    if (ch >= '0' && ch <= '9') {
        return ch - ('0' as u8);
    }
    if (ch >= 'A' && ch <= 'Z') {
        return ch - ('A' as u8) + 10;
    }
    if (ch >= 'a' && ch <= 'z') {
        return ch - ('a' as u8) + 10;
    }
    return null;
}

/// Computes the Verhoeff (D5) check digit (`'0'..'9'`) for an ASCII digit sequence.
/// Returns `null` if `digits` is empty or contains any non-digit byte.
pub fn verhoeffCheckDigit(digits: []const u8): u8? {
    if (digits.len == 0) {
        return null;
    }

    let c: usize = 0;
    let i: usize = 0;
    while (i < digits.len) {
        const ch: u8 = digits[digits.len - 1 - i];
        if (!isDigit(ch)) {
            return null;
        }
        const d: usize = (ch - ('0' as u8)) as usize;
        const p_row: usize = (i + 1) & 7;
        const p_val: usize = VERHOEFF_P[p_row * 10 + d] as usize;
        c = VERHOEFF_D[c * 10 + p_val] as usize;
        i += 1;
    }

    return ('0' as u8) + VERHOEFF_INV[c];
}

/// Validates an ASCII digit string ending with its Verhoeff check digit.
/// Returns `false` if length < 2, any non-digit byte is present, or checksum != 0.
pub fn isValidVerhoeff(digits_with_check: []const u8): bool {
    if (digits_with_check.len < 2) {
        return false;
    }

    let c: usize = 0;
    let i: usize = 0;
    while (i < digits_with_check.len) {
        const ch: u8 = digits_with_check[digits_with_check.len - 1 - i];
        if (!isDigit(ch)) {
            return false;
        }
        const d: usize = (ch - ('0' as u8)) as usize;
        const p_row: usize = i & 7;
        const p_val: usize = VERHOEFF_P[p_row * 10 + d] as usize;
        c = VERHOEFF_D[c * 10 + p_val] as usize;
        i += 1;
    }

    return c == 0;
}

/// Copies `digits` into `out` and appends its Verhoeff check digit.
/// Returns written length (`digits.len + 1`), or `null` on invalid input or short buffer.
pub fn verhoeffAppend(digits: []const u8, out: *[]u8): usize? {
    if (out.*.len < digits.len + 1) {
        return null;
    }
    if (cd := verhoeffCheckDigit(digits)) {
        let i: usize = 0;
        while (i < digits.len) {
            out.*[i] = digits[i];
            i += 1;
        }
        out.*[digits.len] = cd;
        return digits.len + 1;
    }
    return null;
}

/// Computes the Damm quasigroup check digit (`'0'..'9'`) for an ASCII digit sequence.
/// Returns `null` if `digits` is empty or contains any non-digit byte.
pub fn dammCheckDigit(digits: []const u8): u8? {
    if (digits.len == 0) {
        return null;
    }

    let interim: usize = 0;
    let i: usize = 0;
    while (i < digits.len) {
        const ch: u8 = digits[i];
        if (!isDigit(ch)) {
            return null;
        }
        const d: usize = (ch - ('0' as u8)) as usize;
        interim = DAMM_TABLE[interim * 10 + d] as usize;
        i += 1;
    }

    return ('0' as u8) + (interim as u8);
}

/// Validates an ASCII digit string ending with its Damm check digit.
/// Returns `false` if length < 2, any non-digit byte is present, or final interim != 0.
pub fn isValidDamm(digits_with_check: []const u8): bool {
    if (digits_with_check.len < 2) {
        return false;
    }

    let interim: usize = 0;
    let i: usize = 0;
    while (i < digits_with_check.len) {
        const ch: u8 = digits_with_check[i];
        if (!isDigit(ch)) {
            return false;
        }
        const d: usize = (ch - ('0' as u8)) as usize;
        interim = DAMM_TABLE[interim * 10 + d] as usize;
        i += 1;
    }

    return interim == 0;
}

/// Copies `digits` into `out` and appends its Damm check digit.
/// Returns written length (`digits.len + 1`), or `null` on invalid input or short buffer.
pub fn dammAppend(digits: []const u8, out: *[]u8): usize? {
    if (out.*.len < digits.len + 1) {
        return null;
    }
    if (cd := dammCheckDigit(digits)) {
        let i: usize = 0;
        while (i < digits.len) {
            out.*[i] = digits[i];
            i += 1;
        }
        out.*[digits.len] = cd;
        return digits.len + 1;
    }
    return null;
}

/// Computes the ISO/IEC 7064 Mod-11-2 check character (`'0'..'9'` or `'X'`), used by ORCID / ISNI.
/// Ignores `'-'` and `' '` separators. Returns `null` if no digits are present or an invalid byte occurs.
pub fn iso7064Mod11_2CheckDigit(digits: []const u8): u8? {
    let total: u32 = 0;
    let count: usize = 0;
    let i: usize = 0;
    while (i < digits.len) {
        const ch: u8 = digits[i];
        if (isSeparator(ch)) {
            i += 1;
            continue;
        }
        if (!isDigit(ch)) {
            return null;
        }
        total = ((total + ((ch - ('0' as u8)) as u32)) * 2) % 11;
        count += 1;
        i += 1;
    }

    if (count == 0) {
        return null;
    }
    const rem: u32 = ((12 as u32) - total) % 11;
    if (rem == 10) {
        return 'X' as u8;
    }
    return ('0' as u8) + (rem as u8);
}

/// Validates an ISO/IEC 7064 Mod-11-2 code (such as an ORCID or ISNI), ignoring `'-'` and `' '`.
/// Requires at least 1 data digit plus 1 trailing check character (`'0'..'9'`, `'X'`, or `'x'`).
pub fn isValidIso7064Mod11_2(code: []const u8): bool {
    // Locate the last non-separator character (the check character).
    let last_idx: usize = code.len;
    let scan: usize = code.len;
    while (scan > 0) {
        scan -= 1;
        if (!isSeparator(code[scan])) {
            last_idx = scan;
            break;
        }
    }
    if (last_idx == code.len) {
        return false;
    }

    const chk_ch: u8 = code[last_idx];
    let chk_val: u32 = 0;
    if (isDigit(chk_ch)) {
        chk_val = (chk_ch - ('0' as u8)) as u32;
    } else if (chk_ch == 'X' || chk_ch == 'x') {
        chk_val = 10;
    } else {
        return false;
    }

    let total: u32 = 0;
    let data_count: usize = 0;
    let i: usize = 0;
    while (i < last_idx) {
        const ch: u8 = code[i];
        if (isSeparator(ch)) {
            i += 1;
            continue;
        }
        if (!isDigit(ch)) {
            return false;
        }
        total = ((total + ((ch - ('0' as u8)) as u32)) * 2) % 11;
        data_count += 1;
        i += 1;
    }

    if (data_count == 0) {
        return false;
    }
    return ((total + chk_val) % 11) == 1;
}

/// Computes the ISO/IEC 7064 Mod-37-2 check character (`'0'..'9'`, `'A'..'Z'`, or `'*'`).
/// Accepts case-insensitive alphanumeric input (`0-9A-Za-z`) and ignores `'-'` and `' '`.
pub fn iso7064Mod37_2CheckDigit(alnum: []const u8): u8? {
    let total: u32 = 0;
    let count: usize = 0;
    let i: usize = 0;
    while (i < alnum.len) {
        const ch: u8 = alnum[i];
        if (isSeparator(ch)) {
            i += 1;
            continue;
        }
        if (v := alnumVal(ch)) {
            total = ((total + (v as u32)) * 2) % 37;
            count += 1;
        } else {
            return null;
        }
        i += 1;
    }

    if (count == 0) {
        return null;
    }
    const rem: u32 = ((38 as u32) - total) % 37;
    if (rem < 10) {
        return ('0' as u8) + (rem as u8);
    }
    if (rem < 36) {
        return ('A' as u8) + ((rem - 10) as u8);
    }
    return '*' as u8;
}

/// Validates an ISO/IEC 7064 Mod-37-2 alphanumeric code, ignoring `'-'` and `' '`.
/// Requires at least 1 data character (`0-9A-Za-z`) plus 1 trailing check character (`0-9A-Za-z` or `'*'`).
pub fn isValidIso7064Mod37_2(code: []const u8): bool {
    let last_idx: usize = code.len;
    let scan: usize = code.len;
    while (scan > 0) {
        scan -= 1;
        if (!isSeparator(code[scan])) {
            last_idx = scan;
            break;
        }
    }
    if (last_idx == code.len) {
        return false;
    }

    const chk_ch: u8 = code[last_idx];
    let chk_val: u32 = 0;
    if (chk_ch == '*') {
        chk_val = 36;
    } else if (v := alnumVal(chk_ch)) {
        chk_val = v as u32;
    } else {
        return false;
    }

    let total: u32 = 0;
    let data_count: usize = 0;
    let i: usize = 0;
    while (i < last_idx) {
        const ch: u8 = code[i];
        if (isSeparator(ch)) {
            i += 1;
            continue;
        }
        if (v := alnumVal(ch)) {
            total = ((total + (v as u32)) * 2) % 37;
            data_count += 1;
        } else {
            return false;
        }
        i += 1;
    }

    if (data_count == 0) {
        return false;
    }
    return ((total + chk_val) % 37) == 1;
}
