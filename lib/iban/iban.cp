// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc ISO 13616 IBAN & ISO 7064 Mod-97-10 Financial Validator/Formatter)

import std from "std";

/// Minimum and maximum electronic IBAN lengths defined by ISO 13616.
pub const IBAN_MIN_LEN: usize = 15;
pub const IBAN_MAX_LEN: usize = 34;

/// Parsed and validated IBAN components (zero-allocation fixed storage).
pub struct IbanInfo {
    country: [2]u8;
    check_digits: [2]u8;
    bban: [30]u8;
    bban_len: usize;

    /// Returns the active BBAN byte slice (`bban[0..bban_len]`).
    fn bbanSlice(): []const u8 {
        return _.bban[0.._.bban_len];
    }

    /// Returns total electronic IBAN length (`4 + bban_len`).
    fn totalLen(): usize {
        return 4 + _.bban_len;
    }
}

fn toUpperAscii(ch: u8): u8 {
    if (ch >= 0x61 && ch <= 0x7A) {
        return ch - 0x20;
    }
    return ch;
}

fn isUpperAlpha(ch: u8): bool {
    return ch >= 0x41 && ch <= 0x5A;
}

fn isDigit(ch: u8): bool {
    return ch >= 0x30 && ch <= 0x39;
}

fn isAlnumUpper(ch: u8): bool {
    return isDigit(ch) || isUpperAlpha(ch);
}

fn ccKey(c0: u8, c1: u8): u16 {
    return ((c0 as u16) << 8) | (c1 as u16);
}

/// Looks up the ISO 13616 IBAN length for a 2-letter ISO 3166-1 alpha-2 country code.
/// Case-insensitive; returns `null` if unknown or non-alphabetic.
pub fn ibanCountryLength(cc0: u8, cc1: u8): u8? {
    const a = toUpperAscii(cc0);
    const b = toUpperAscii(cc1);
    if (!isUpperAlpha(a) || !isUpperAlpha(b)) {
        return null;
    }
    const k = ccKey(a, b);

    // Core European, Middle Eastern & International IBAN Registry (52 countries)
    if (k == ccKey(0x41, 0x44)) { return 24; } // AD - Andorra
    if (k == ccKey(0x41, 0x45)) { return 23; } // AE - United Arab Emirates
    if (k == ccKey(0x41, 0x4C)) { return 28; } // AL - Albania
    if (k == ccKey(0x41, 0x54)) { return 20; } // AT - Austria
    if (k == ccKey(0x42, 0x41)) { return 20; } // BA - Bosnia & Herzegovina
    if (k == ccKey(0x42, 0x45)) { return 16; } // BE - Belgium
    if (k == ccKey(0x42, 0x47)) { return 22; } // BG - Bulgaria
    if (k == ccKey(0x42, 0x48)) { return 22; } // BH - Bahrain
    if (k == ccKey(0x42, 0x52)) { return 29; } // BR - Brazil
    if (k == ccKey(0x43, 0x48)) { return 21; } // CH - Switzerland
    if (k == ccKey(0x43, 0x59)) { return 28; } // CY - Cyprus
    if (k == ccKey(0x43, 0x5A)) { return 24; } // CZ - Czech Republic
    if (k == ccKey(0x44, 0x45)) { return 22; } // DE - Germany
    if (k == ccKey(0x44, 0x4B)) { return 18; } // DK - Denmark
    if (k == ccKey(0x45, 0x45)) { return 20; } // EE - Estonia
    if (k == ccKey(0x45, 0x53)) { return 24; } // ES - Spain
    if (k == ccKey(0x46, 0x49)) { return 18; } // FI - Finland
    if (k == ccKey(0x46, 0x52)) { return 27; } // FR - France
    if (k == ccKey(0x47, 0x42)) { return 22; } // GB - United Kingdom
    if (k == ccKey(0x47, 0x52)) { return 27; } // GR - Greece
    if (k == ccKey(0x48, 0x52)) { return 21; } // HR - Croatia
    if (k == ccKey(0x48, 0x55)) { return 28; } // HU - Hungary
    if (k == ccKey(0x49, 0x45)) { return 22; } // IE - Ireland
    if (k == ccKey(0x49, 0x4C)) { return 23; } // IL - Israel
    if (k == ccKey(0x49, 0x53)) { return 26; } // IS - Iceland
    if (k == ccKey(0x49, 0x54)) { return 27; } // IT - Italy
    if (k == ccKey(0x4A, 0x4F)) { return 30; } // JO - Jordan
    if (k == ccKey(0x4B, 0x57)) { return 30; } // KW - Kuwait
    if (k == ccKey(0x4C, 0x49)) { return 21; } // LI - Liechtenstein
    if (k == ccKey(0x4C, 0x54)) { return 20; } // LT - Lithuania
    if (k == ccKey(0x4C, 0x55)) { return 20; } // LU - Luxembourg
    if (k == ccKey(0x4C, 0x56)) { return 21; } // LV - Latvia
    if (k == ccKey(0x4D, 0x43)) { return 27; } // MC - Monaco
    if (k == ccKey(0x4D, 0x45)) { return 22; } // ME - Montenegro
    if (k == ccKey(0x4D, 0x4B)) { return 19; } // MK - North Macedonia
    if (k == ccKey(0x4D, 0x54)) { return 31; } // MT - Malta
    if (k == ccKey(0x4E, 0x4C)) { return 18; } // NL - Netherlands
    if (k == ccKey(0x4E, 0x4F)) { return 15; } // NO - Norway
    if (k == ccKey(0x50, 0x4B)) { return 24; } // PK - Pakistan
    if (k == ccKey(0x50, 0x4C)) { return 28; } // PL - Poland
    if (k == ccKey(0x50, 0x54)) { return 25; } // PT - Portugal
    if (k == ccKey(0x51, 0x41)) { return 29; } // QA - Qatar
    if (k == ccKey(0x52, 0x4F)) { return 24; } // RO - Romania
    if (k == ccKey(0x52, 0x53)) { return 22; } // RS - Serbia
    if (k == ccKey(0x53, 0x41)) { return 24; } // SA - Saudi Arabia
    if (k == ccKey(0x53, 0x45)) { return 24; } // SE - Sweden
    if (k == ccKey(0x53, 0x49)) { return 19; } // SI - Slovenia
    if (k == ccKey(0x53, 0x4B)) { return 24; } // SK - Slovakia
    if (k == ccKey(0x53, 0x4D)) { return 27; } // SM - San Marino
    if (k == ccKey(0x54, 0x4E)) { return 24; } // TN - Tunisia
    if (k == ccKey(0x54, 0x52)) { return 26; } // TR - Turkey
    if (k == ccKey(0x55, 0x41)) { return 29; } // UA - Ukraine

    return null;
}

fn normalizeAlnum(src: []const u8, dst: []u8): usize? {
    let out: usize = 0;
    let i: usize = 0;
    while (i < src.len) {
        const raw = src[i];
        if (raw == 0x20 || raw == 0x2D) { // ignore ' ' and '-'
            i += 1;
            continue;
        }
        const ch = toUpperAscii(raw);
        if (!isAlnumUpper(ch)) {
            return null;
        }
        if (out >= dst.len) {
            return null;
        }
        dst[out] = ch;
        out += 1;
        i += 1;
    }
    return out;
}

fn feedMod97(rem_in: u32, ch: u8): u32? {
    if (isDigit(ch)) {
        return (rem_in * 10 + ((ch - 0x30) as u32)) % 97;
    }
    if (isUpperAlpha(ch)) {
        const val = (ch - 0x41 + 10) as u32; // A=10 .. Z=35
        return (rem_in * 100 + val) % 97;
    }
    return null;
}

fn computeMod97Normalized(norm: []const u8): u8? {
    if (norm.len < 4) {
        return null;
    }
    let rem: u32 = 0;
    let i: usize = 4;
    while (i < norm.len) {
        rem = feedMod97(rem, norm[i]) ?? return null;
        i += 1;
    }
    let j: usize = 0;
    while (j < 4) {
        rem = feedMod97(rem, norm[j]) ?? return null;
        j += 1;
    }
    return rem as u8;
}

/// Computes the ISO 7064 Mod-97-10 remainder of an IBAN string (ignoring spaces and hyphens).
/// Moves the first 4 characters to the end, expands `A..Z` to `10..35`, and reduces mod 97.
/// A valid IBAN always yields remainder `1`. Returns `null` on invalid characters or length `< 4` / `> 34`.
pub fn ibanMod97(iban: []const u8): u8? {
    let norm: [34]u8 = std.mem.zeroes([34]u8);
    const nlen = normalizeAlnum(iban, norm[0..]) ?? return null;
    if (nlen < 4) {
        return null;
    }
    return computeMod97Normalized(norm[0..nlen]);
}

fn validateAndNormalize(iban: []const u8, norm: []u8): usize? {
    const nlen = normalizeAlnum(iban, norm) ?? return null;
    if (nlen < IBAN_MIN_LEN || nlen > IBAN_MAX_LEN) {
        return null;
    }
    if (!isUpperAlpha(norm[0]) || !isUpperAlpha(norm[1])) {
        return null;
    }
    if (!isDigit(norm[2]) || !isDigit(norm[3])) {
        return null;
    }
    const expected = ibanCountryLength(norm[0], norm[1]) ?? return null;
    if (nlen != (expected as usize)) {
        return null;
    }
    const rem = computeMod97Normalized(norm[0..nlen]) ?? return null;
    if (rem != 1) {
        return null;
    }
    return nlen;
}

/// Validates an IBAN against ISO 13616 structure, country length registry, and ISO 7064 Mod-97-10 (`rem == 1`).
pub fn isValidIban(iban: []const u8): bool {
    let norm: [34]u8 = std.mem.zeroes([34]u8);
    const nlen = validateAndNormalize(iban, norm[0..]);
    return nlen != null;
}

/// Parses and validates an IBAN into an `IbanInfo` struct (ignoring spaces/hyphens, uppercasing).
/// Returns `null` if invalid.
pub fn parseIban(iban: []const u8): IbanInfo? {
    let norm: [34]u8 = std.mem.zeroes([34]u8);
    const nlen = validateAndNormalize(iban, norm[0..]) ?? return null;
    let bban: [30]u8 = std.mem.zeroes([30]u8);
    const blen = nlen - 4;
    let i: usize = 0;
    while (i < blen) {
        bban[i] = norm[4 + i];
        i += 1;
    }
    return IbanInfo {
        country: [norm[0], norm[1]],
        check_digits: [norm[2], norm[3]],
        bban: bban,
        bban_len: blen,
    };
}

/// Two-digit zero-padded ISO 7064 check digits (`[2]u8`).
pub type IbanCheckDigits = [2]u8;

/// Computes the 2-digit zero-padded ISO 7064 check digits (`98 - mod97(bban + country + "00")`)
/// for a given 2-letter country code and BBAN. Returns `null` on unknown country, invalid characters, or length mismatch.
pub fn computeIbanCheckDigits(country: [2]u8, bban: []const u8): IbanCheckDigits? {
    const c0 = toUpperAscii(country[0]);
    const c1 = toUpperAscii(country[1]);
    const expected = ibanCountryLength(c0, c1) ?? return null;

    let norm_bban: [30]u8 = std.mem.zeroes([30]u8);
    const blen = normalizeAlnum(bban, norm_bban[0..]) ?? return null;
    if (blen + 4 != (expected as usize)) {
        return null;
    }

    let rem: u32 = 0;
    let i: usize = 0;
    while (i < blen) {
        rem = feedMod97(rem, norm_bban[i]) ?? return null;
        i += 1;
    }
    rem = feedMod97(rem, c0) ?? return null;
    rem = feedMod97(rem, c1) ?? return null;
    rem = feedMod97(rem, 0x30) ?? return null; // '0'
    rem = feedMod97(rem, 0x30) ?? return null; // '0'

    const check: u32 = (98 as u32) - rem; // range 2..98
    return [
        (0x30 + (check / 10)) as u8,
        (0x30 + (check % 10)) as u8,
    ];
}

/// Formats a valid IBAN into uppercase electronic format (no spaces) into `out`.
/// Returns bytes written, or `null` if `iban` is invalid or `out` is too small.
pub fn formatIbanElectronic(iban: []const u8, out: []u8): usize? {
    let norm: [34]u8 = std.mem.zeroes([34]u8);
    const nlen = validateAndNormalize(iban, norm[0..]) ?? return null;
    if (out.len < nlen) {
        return null;
    }
    let i: usize = 0;
    while (i < nlen) {
        out[i] = norm[i];
        i += 1;
    }
    return nlen;
}

/// Formats a valid IBAN into uppercase print format grouped by 4 characters separated by spaces
/// (e.g. `"DE89 3704 0044 0532 0130 00"`). Returns bytes written, or `null` if invalid or `out` is too small.
pub fn formatIbanPrint(iban: []const u8, out: []u8): usize? {
    let norm: [34]u8 = std.mem.zeroes([34]u8);
    const nlen = validateAndNormalize(iban, norm[0..]) ?? return null;
    const spaces = (nlen - 1) / 4;
    const needed = nlen + spaces;
    if (out.len < needed) {
        return null;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < nlen) {
        if (i > 0 && (i % 4) == 0) {
            out[pos] = 0x20; // ' '
            pos += 1;
        }
        out[pos] = norm[i];
        pos += 1;
        i += 1;
    }
    return pos;
}
