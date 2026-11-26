// @cap/stringprep — Zero-heap RFC 3454 / RFC 4013 SASLprep & Nameprep string preparation
// Matching crates.io Top 510 stringprep (286M+ downloads). Clean-room MIT in pure CP.

pub struct PrepResult {
    pub len: usize,
    pub ok: bool,
}

struct DecCp {
    cp: u32,
    width: usize,
    valid: bool,
}

fn decodeUtf8(s: string, i: usize): DecCp {
    let b0 = s[i];
    if (b0 < 0x80) {
        return DecCp { cp: b0 as u32, width: 1, valid: true };
    }
    if ((b0 & 0xE0) == 0xC0 && i + 1 < s.len) {
        let b1 = s[i + 1];
        if ((b1 & 0xC0) == 0x80) {
            let cp = (((b0 & 0x1F) as u32) << 6) | ((b1 & 0x3F) as u32);
            if (cp >= 0x80) { return DecCp { cp: cp, width: 2, valid: true }; }
        }
    } else if ((b0 & 0xF0) == 0xE0 && i + 2 < s.len) {
        let b1 = s[i + 1];
        let b2 = s[i + 2];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
            let cp = (((b0 & 0x0F) as u32) << 12) | (((b1 & 0x3F) as u32) << 6) | ((b2 & 0x3F) as u32);
            if (cp >= 0x800) { return DecCp { cp: cp, width: 3, valid: true }; }
        }
    } else if ((b0 & 0xF8) == 0xF0 && i + 3 < s.len) {
        let b1 = s[i + 1];
        let b2 = s[i + 2];
        let b3 = s[i + 3];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
            let cp = (((b0 & 0x07) as u32) << 18) | (((b1 & 0x3F) as u32) << 12) | (((b2 & 0x3F) as u32) << 6) | ((b3 & 0x3F) as u32);
            if (cp >= 0x10000 && cp <= 0x10FFFF) { return DecCp { cp: cp, width: 4, valid: true }; }
        }
    }
    return DecCp { cp: 0, width: 1, valid: false };
}

fn encodeUtf8(cp: u32, out: []u8, pos: usize): i64 {
    if (cp < 0x80) {
        if (pos + 1 > out.len) { return -1; }
        out[pos] = cp as u8;
        return (pos + 1) as i64;
    }
    if (cp < 0x800) {
        if (pos + 2 > out.len) { return -1; }
        out[pos] = (0xC0 | ((cp >> 6) & 0x1F)) as u8;
        out[pos + 1] = (0x80 | (cp & 0x3F)) as u8;
        return (pos + 2) as i64;
    }
    if (cp < 0x10000) {
        if (pos + 3 > out.len) { return -1; }
        out[pos] = (0xE0 | ((cp >> 12) & 0x0F)) as u8;
        out[pos + 1] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
        out[pos + 2] = (0x80 | (cp & 0x3F)) as u8;
        return (pos + 3) as i64;
    }
    if (pos + 4 > out.len) { return -1; }
    out[pos] = (0xF0 | ((cp >> 18) & 0x07)) as u8;
    out[pos + 1] = (0x80 | ((cp >> 12) & 0x3F)) as u8;
    out[pos + 2] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
    out[pos + 3] = (0x80 | (cp & 0x3F)) as u8;
    return (pos + 4) as i64;
}

// RFC 3454 Table B.1 — Commonly mapped to nothing
pub fn isMappedToNothing(cp: u32): bool {
    return cp == 0x00AD || cp == 0x034F || cp == 0x1806
        || (cp >= 0x180B && cp <= 0x180D)
        || (cp >= 0x200B && cp <= 0x200D)
        || cp == 0x2060 || cp == 0xFEFF
        || (cp >= 0xFE00 && cp <= 0xFE0F);
}

// RFC 3454 Table C.1.2 — Non-ASCII spaces mapped to ASCII 0x20 in SASLprep
pub fn isNonAsciiSpace(cp: u32): bool {
    return cp == 0x00A0 || cp == 0x1680
        || (cp >= 0x2000 && cp <= 0x200B)
        || cp == 0x202F || cp == 0x205F || cp == 0x3000;
}

// RFC 3454 Tables C.2.1, C.2.2, C.3, C.4, C.5, C.6, C.7, C.8, C.9 — Prohibited output
pub fn isSaslProhibited(cp: u32): bool {
    // C.2.1 ASCII control characters
    if (cp <= 0x001F || cp == 0x007F) { return true; }
    // C.2.2 Non-ASCII control characters
    if (cp >= 0x0080 && cp <= 0x009F) { return true; }
    if (cp == 0x06DD || cp == 0x070F || cp == 0x180E || cp == 0x2028 || cp == 0x2029) { return true; }
    // C.3 Private use
    if (cp >= 0xE000 && cp <= 0xF8FF) { return true; }
    if (cp >= 0xF0000 && cp <= 0xFFFFD) { return true; }
    // C.4 Non-character code points
    if ((cp >= 0xFDD0 && cp <= 0xFDEF) || ((cp & 0xFFFF) == 0xFFFE) || ((cp & 0xFFFF) == 0xFFFF)) { return true; }
    // C.5 Surrogate codes
    if (cp >= 0xD800 && cp <= 0xDFFF) { return true; }
    // C.6 Inappropriate for plain text
    if (cp >= 0xFFF9 && cp <= 0xFFFD) { return true; }
    // C.8 Change display properties
    if (cp == 0x200E || cp == 0x200F || (cp >= 0x202A && cp <= 0x202E)) { return true; }
    return false;
}

// RFC 4013 SASLprep (used by PostgreSQL SCRAM-SHA-256, LDAP, IMAP/SMTP SASL)
pub fn saslprep(input: string, out: []u8): PrepResult {
    let i: usize = 0;
    let pos: usize = 0;
    while (i < input.len) {
        let dec_cp = decodeUtf8(input, i);
        if (!dec_cp.valid) { return PrepResult { len: 0, ok: false }; }
        i = i + dec_cp.width;
        let cp = dec_cp.cp;

        if (isMappedToNothing(cp)) {
            continue;
        }
        if (isNonAsciiSpace(cp)) {
            cp = 0x20;
        }
        if (isSaslProhibited(cp)) {
            return PrepResult { len: 0, ok: false };
        }
        let next_pos = encodeUtf8(cp, out, pos);
        if (next_pos < 0) { return PrepResult { len: 0, ok: false }; }
        pos = next_pos as usize;
    }
    return PrepResult { len: pos, ok: true };
}

// RFC 3491 Nameprep-style ASCII case-folding + Table B.1 stripping + prohibited check
pub fn nameprepAscii(input: string, out: []u8): PrepResult {
    let i: usize = 0;
    let pos: usize = 0;
    while (i < input.len) {
        let dec_cp = decodeUtf8(input, i);
        if (!dec_cp.valid) { return PrepResult { len: 0, ok: false }; }
        i = i + dec_cp.width;
        let cp = dec_cp.cp;

        if (isMappedToNothing(cp)) {
            continue;
        }
        if (cp >= 65 && cp <= 90) {
            cp = cp + 32;
        }
        if (isSaslProhibited(cp) || isNonAsciiSpace(cp)) {
            return PrepResult { len: 0, ok: false };
        }
        let next_pos = encodeUtf8(cp, out, pos);
        if (next_pos < 0) { return PrepResult { len: 0, ok: false }; }
        pos = next_pos as usize;
    }
    return PrepResult { len: pos, ok: true };
}
