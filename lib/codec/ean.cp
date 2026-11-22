// Origin: Pure-CP Stdlib (Zero-Alloc ISO/IEC 15420 EAN-13 / EAN-8 / UPC-A 1D Barcode L/G/R Codec) | Created: 2026-11-22

import std from "std";

/// ISO/IEC 15420 barcode module lengths & guard bit patterns.
pub const EAN13_MODULES: usize = 95;
pub const EAN8_MODULES: usize = 67;
pub const UPCA_MODULES: usize = 95;
pub const EAN_DIGIT_MODULES: usize = 7;

pub const GUARD_OUTER: u8 = 0b101;     // 3 modules
pub const GUARD_CENTER: u8 = 0b01010;  // 5 modules

/// ISO/IEC 15420 L-code (left odd-parity) 7-bit patterns for digits 0..9.
const L_PATTERNS: [10]u8 = [
    0b0001101, // 0
    0b0011001, // 1
    0b0010011, // 2
    0b0111101, // 3
    0b0100011, // 4
    0b0110001, // 5
    0b0101111, // 6
    0b0111011, // 7
    0b0110111, // 8
    0b0001011, // 9
];

/// ISO/IEC 15420 EAN-13 first-digit parity masks for left 6 digits (0 = L, 1 = G; bit 5 = first left digit).
const EAN13_PARITY_MASKS: [10]u8 = [
    0b000000, // 0: LLLLLL
    0b001011, // 1: LLGLGG
    0b001101, // 2: LLGGLG
    0b001110, // 3: LLGGGL
    0b010011, // 4: LGLLGG
    0b011001, // 5: LGGLLG
    0b011100, // 6: LGGGLL
    0b010101, // 7: LGLGLG
    0b010110, // 8: LGLGGL
    0b011010, // 9: LGGLGL
];

fn reverse7(v: u8): u8 {
    let out: u8 = 0;
    let i: usize = 0;
    while (i < 7) {
        if (((v >> (i as u3)) & 1) != 0) {
            out = out | ((1 as u8) << ((6 - i) as u3));
        }
        i += 1;
    }
    return out;
}

/// Returns the 7-bit L-code (odd parity) pattern for `digit` (`0..9`), or `null` if `digit > 9`.
pub fn eanPatternL(digit: u8): u8? {
    if (digit > 9) {
        return null;
    }
    return L_PATTERNS[digit as usize];
}

/// Returns the 7-bit R-code (even parity, bitwise complement of L-code) pattern for `digit` (`0..9`).
pub fn eanPatternR(digit: u8): u8? {
    if (digit > 9) {
        return null;
    }
    return (~L_PATTERNS[digit as usize]) & 0x7F;
}

/// Returns the 7-bit G-code (even parity, 7-bit reversal of R-code) pattern for `digit` (`0..9`).
pub fn eanPatternG(digit: u8): u8? {
    const r = eanPatternR(digit) ?? return null;
    return reverse7(r);
}

/// Returns the 6-bit L/G parity mask for EAN-13 `first_digit` (`0..9`), where bit 5..0 maps left digits 1..6 (`0=L, 1=G`).
pub fn ean13ParityMask(first_digit: u8): u8? {
    if (first_digit > 9) {
        return null;
    }
    return EAN13_PARITY_MASKS[first_digit as usize];
}

/// Computes the GS1 Mod-10 check digit (`0..9`) for an ASCII digit slice (`'0'..'9'`).
/// Weights alternate `3, 1, 3, 1, ...` from right to left (valid for EAN-13, EAN-8, UPC-A).
/// Returns `null` if `digits` is empty or contains non-digit bytes.
pub fn eanCheckDigit(digits: []const u8): u8? {
    if (digits.len == 0) {
        return null;
    }
    let sum: u32 = 0;
    let i: usize = 0;
    while (i < digits.len) {
        const ch = digits[digits.len - 1 - i];
        if (ch < 0x30 || ch > 0x39) {
            return null;
        }
        const d = (ch - 0x30) as u32;
        if ((i % 2) == 0) {
            sum += d * 3;
        } else {
            sum += d;
        }
        i += 1;
    }
    return ((10 - (sum % 10)) % 10) as u8;
}

fn writeBits(out: []u8, pos: usize, bits: u8, width: usize): usize {
    let i: usize = 0;
    while (i < width) {
        const shift = (width - 1 - i) as u3;
        out[pos + i] = (bits >> shift) & 1;
        i += 1;
    }
    return pos + width;
}

/// Encodes a 13-digit ASCII EAN-13 string into 95 module bytes (`0`/`1`) in `out_modules`.
/// Validates all 13 digits and the GS1 Mod-10 check digit. Returns `95` or `null` on error.
pub fn encodeEan13Modules(digits13: []const u8, out_modules: []u8): usize? {
    if (digits13.len != 13 || out_modules.len < EAN13_MODULES) {
        return null;
    }
    const expected_check = eanCheckDigit(digits13[0..12]) ?? return null;
    if (digits13[12] < 0x30 || digits13[12] > 0x39) {
        return null;
    }
    if ((digits13[12] - 0x30) != expected_check) {
        return null;
    }

    const first = digits13[0] - 0x30;
    const parity = ean13ParityMask(first) ?? return null;

    let pos: usize = 0;
    pos = writeBits(out_modules, pos, GUARD_OUTER, 3);

    // Left 6 digits (indices 1..6) using L/G parity
    let i: usize = 0;
    while (i < 6) {
        const d = digits13[1 + i] - 0x30;
        const use_g = ((parity >> ((5 - i) as u3)) & 1) != 0;
        const pat = if (use_g) (eanPatternG(d) ?? return null) else (eanPatternL(d) ?? return null);
        pos = writeBits(out_modules, pos, pat, 7);
        i += 1;
    }

    pos = writeBits(out_modules, pos, GUARD_CENTER, 5);

    // Right 6 digits (indices 7..12) using R-code
    let j: usize = 0;
    while (j < 6) {
        const d = digits13[7 + j] - 0x30;
        const pat = eanPatternR(d) ?? return null;
        pos = writeBits(out_modules, pos, pat, 7);
        j += 1;
    }

    pos = writeBits(out_modules, pos, GUARD_OUTER, 3);
    return pos;
}

fn readModule(modules: []const u8, idx: usize, reverse: bool): u8? {
    const raw = if (reverse) modules[modules.len - 1 - idx] else modules[idx];
    if (raw != 0 && raw != 1) {
        return null;
    }
    return raw;
}

fn readBits(modules: []const u8, pos: usize, width: usize, reverse: bool): u8? {
    let val: u8 = 0;
    let i: usize = 0;
    while (i < width) {
        const b = readModule(modules, pos + i, reverse) ?? return null;
        val = (val << 1) | b;
        i += 1;
    }
    return val;
}

fn matchL(pat: u8): u8? {
    let d: usize = 0;
    while (d < 10) {
        if (L_PATTERNS[d] == pat) {
            return d as u8;
        }
        d += 1;
    }
    return null;
}

fn matchR(pat: u8): u8? {
    let d: usize = 0;
    while (d < 10) {
        if (((~L_PATTERNS[d]) & 0x7F) == pat) {
            return d as u8;
        }
        d += 1;
    }
    return null;
}

fn matchG(pat: u8): u8? {
    let d: usize = 0;
    while (d < 10) {
        const r = (~L_PATTERNS[d]) & 0x7F;
        if (reverse7(r) == pat) {
            return d as u8;
        }
        d += 1;
    }
    return null;
}

fn matchParityFirstDigit(mask: u8): u8? {
    let d: usize = 0;
    while (d < 10) {
        if (EAN13_PARITY_MASKS[d] == mask) {
            return d as u8;
        }
        d += 1;
    }
    return null;
}

fn decodeEan13Dir(modules95: []const u8, reverse: bool, out_digits13: []u8): bool {
    const g_left = readBits(modules95, 0, 3, reverse) ?? return false;
    const g_mid = readBits(modules95, 45, 5, reverse) ?? return false;
    const g_right = readBits(modules95, 92, 3, reverse) ?? return false;
    if (g_left != GUARD_OUTER || g_mid != GUARD_CENTER || g_right != GUARD_OUTER) {
        return false;
    }

    let tmp: [13]u8 = std.mem.zeroes([13]u8);
    let parity: u8 = 0;

    // Decode left 6 digits (L or G)
    let i: usize = 0;
    while (i < 6) {
        const pat = readBits(modules95, 3 + i * 7, 7, reverse) ?? return false;
        if (dl := matchL(pat)) {
            tmp[1 + i] = dl + 0x30;
            parity = parity << 1;
        } else if (dg := matchG(pat)) {
            tmp[1 + i] = dg + 0x30;
            parity = (parity << 1) | 1;
        } else {
            return false;
        }
        i += 1;
    }

    const first = matchParityFirstDigit(parity) ?? return false;
    tmp[0] = first + 0x30;

    // Decode right 6 digits (R)
    let j: usize = 0;
    while (j < 6) {
        const pat = readBits(modules95, 50 + j * 7, 7, reverse) ?? return false;
        const dr = matchR(pat) ?? return false;
        tmp[7 + j] = dr + 0x30;
        j += 1;
    }

    const expected = eanCheckDigit(tmp[0..12]) ?? return false;
    if ((tmp[12] - 0x30) != expected) {
        return false;
    }

    let k: usize = 0;
    while (k < 13) {
        out_digits13[k] = tmp[k];
        k += 1;
    }
    return true;
}

/// Decodes 95 module bytes (`0`/`1`) into a 13-digit ASCII EAN-13 string in `out_digits13`.
/// Automatically supports both forward and upside-down (reversed) scans and verifies guards + Mod-10.
/// Returns `13`, or `null` on invalid modules, broken guards, or bad check digit.
pub fn decodeEan13Modules(modules95: []const u8, out_digits13: []u8): usize? {
    if (modules95.len != EAN13_MODULES || out_digits13.len < 13) {
        return null;
    }
    if (decodeEan13Dir(modules95, false, out_digits13)) {
        return 13;
    }
    if (decodeEan13Dir(modules95, true, out_digits13)) {
        return 13;
    }
    return null;
}

/// Encodes an 8-digit ASCII EAN-8 string into 67 module bytes (`0`/`1`) in `out_modules`.
/// Validates all 8 digits and GS1 Mod-10 check digit. Returns `67` or `null` on error.
pub fn encodeEan8Modules(digits8: []const u8, out_modules: []u8): usize? {
    if (digits8.len != 8 || out_modules.len < EAN8_MODULES) {
        return null;
    }
    const expected_check = eanCheckDigit(digits8[0..7]) ?? return null;
    if (digits8[7] < 0x30 || digits8[7] > 0x39) {
        return null;
    }
    if ((digits8[7] - 0x30) != expected_check) {
        return null;
    }

    let pos: usize = 0;
    pos = writeBits(out_modules, pos, GUARD_OUTER, 3);

    // Left 4 digits using L-code
    let i: usize = 0;
    while (i < 4) {
        const pat = eanPatternL(digits8[i] - 0x30) ?? return null;
        pos = writeBits(out_modules, pos, pat, 7);
        i += 1;
    }

    pos = writeBits(out_modules, pos, GUARD_CENTER, 5);

    // Right 4 digits using R-code
    let j: usize = 0;
    while (j < 4) {
        const pat = eanPatternR(digits8[4 + j] - 0x30) ?? return null;
        pos = writeBits(out_modules, pos, pat, 7);
        j += 1;
    }

    pos = writeBits(out_modules, pos, GUARD_OUTER, 3);
    return pos;
}

fn decodeEan8Dir(modules67: []const u8, reverse: bool, out_digits8: []u8): bool {
    const g_left = readBits(modules67, 0, 3, reverse) ?? return false;
    const g_mid = readBits(modules67, 31, 5, reverse) ?? return false;
    const g_right = readBits(modules67, 64, 3, reverse) ?? return false;
    if (g_left != GUARD_OUTER || g_mid != GUARD_CENTER || g_right != GUARD_OUTER) {
        return false;
    }

    let tmp: [8]u8 = std.mem.zeroes([8]u8);

    // Left 4 digits (L-code)
    let i: usize = 0;
    while (i < 4) {
        const pat = readBits(modules67, 3 + i * 7, 7, reverse) ?? return false;
        const dl = matchL(pat) ?? return false;
        tmp[i] = dl + 0x30;
        i += 1;
    }

    // Right 4 digits (R-code)
    let j: usize = 0;
    while (j < 4) {
        const pat = readBits(modules67, 36 + j * 7, 7, reverse) ?? return false;
        const dr = matchR(pat) ?? return false;
        tmp[4 + j] = dr + 0x30;
        j += 1;
    }

    const expected = eanCheckDigit(tmp[0..7]) ?? return false;
    if ((tmp[7] - 0x30) != expected) {
        return false;
    }

    let k: usize = 0;
    while (k < 8) {
        out_digits8[k] = tmp[k];
        k += 1;
    }
    return true;
}

/// Decodes 67 module bytes (`0`/`1`) into an 8-digit ASCII EAN-8 string in `out_digits8`.
/// Automatically supports forward and reversed scans and verifies guards + Mod-10.
/// Returns `8`, or `null` on error.
pub fn decodeEan8Modules(modules67: []const u8, out_digits8: []u8): usize? {
    if (modules67.len != EAN8_MODULES || out_digits8.len < 8) {
        return null;
    }
    if (decodeEan8Dir(modules67, false, out_digits8)) {
        return 8;
    }
    if (decodeEan8Dir(modules67, true, out_digits8)) {
        return 8;
    }
    return null;
}

/// Encodes a 12-digit ASCII UPC-A string into 95 module bytes (`0`/`1`) in `out_modules`
/// (equivalent to EAN-13 with leading `'0'`). Returns `95` or `null` on error.
pub fn encodeUpcAModules(digits12: []const u8, out_modules: []u8): usize? {
    if (digits12.len != 12 || out_modules.len < UPCA_MODULES) {
        return null;
    }
    let ean13: [13]u8 = std.mem.zeroes([13]u8);
    ean13[0] = 0x30; // '0'
    let i: usize = 0;
    while (i < 12) {
        ean13[1 + i] = digits12[i];
        i += 1;
    }
    return encodeEan13Modules(ean13[0..], out_modules);
}
