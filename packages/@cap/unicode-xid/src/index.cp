// packages/@cap/unicode-xid/src/index.cp
// Zero-allocation UAX #31 Unicode Identifier (XID_Start / XID_Continue) classifier & scanner.
// Clean-room MIT implementation inspired by Rust `unicode-xid`.

// ---------------------------------------------------------------------------
// Private Strict UTF-8 Decoder & Binary Search Tables (prefixed with uxid)
// ---------------------------------------------------------------------------

struct UxidRune {
    cp: u32,
    len: usize,
    valid: bool,
}

fn uxidDecodeUtf8Strict(s: string, offset: usize): UxidRune {
    if (offset >= s.len) {
        return UxidRune{ cp: 0, len: 0, valid: false };
    }
    const b0 = s[offset];
    if (b0 < (0x80 as u8)) {
        return UxidRune{ cp: b0 as u32, len: 1, valid: true };
    }
    if ((b0 & (0xE0 as u8)) == (0xC0 as u8) && offset + 1 < s.len) {
        const b1 = s[offset + 1];
        if ((b1 & (0xC0 as u8)) == (0x80 as u8)) {
            const cp: u32 = (((b0 & (0x1F as u8)) as u32) << (6 as u5))
                | ((b1 & (0x3F as u8)) as u32);
            if (cp >= (0x80 as u32)) {
                return UxidRune{ cp: cp, len: 2, valid: true };
            }
        }
        return UxidRune{ cp: 0xFFFD, len: 1, valid: false };
    }
    if ((b0 & (0xF0 as u8)) == (0xE0 as u8) && offset + 2 < s.len) {
        const b1 = s[offset + 1];
        const b2 = s[offset + 2];
        if ((b1 & (0xC0 as u8)) == (0x80 as u8) && (b2 & (0xC0 as u8)) == (0x80 as u8)) {
            const cp: u32 = (((b0 & (0x0F as u8)) as u32) << (12 as u5))
                | (((b1 & (0x3F as u8)) as u32) << (6 as u5))
                | ((b2 & (0x3F as u8)) as u32);
            if (cp >= (0x800 as u32) && !(cp >= (0xD800 as u32) && cp <= (0xDFFF as u32))) {
                return UxidRune{ cp: cp, len: 3, valid: true };
            }
        }
        return UxidRune{ cp: 0xFFFD, len: 1, valid: false };
    }
    if ((b0 & (0xF8 as u8)) == (0xF0 as u8) && offset + 3 < s.len) {
        const b1 = s[offset + 1];
        const b2 = s[offset + 2];
        const b3 = s[offset + 3];
        if ((b1 & (0xC0 as u8)) == (0x80 as u8)
            && (b2 & (0xC0 as u8)) == (0x80 as u8)
            && (b3 & (0xC0 as u8)) == (0x80 as u8))
        {
            const cp: u32 = (((b0 & (0x07 as u8)) as u32) << (18 as u5))
                | (((b1 & (0x3F as u8)) as u32) << (12 as u5))
                | (((b2 & (0x3F as u8)) as u32) << (6 as u5))
                | ((b3 & (0x3F as u8)) as u32);
            if (cp >= (0x10000 as u32) && cp <= (0x10FFFF as u32)) {
                return UxidRune{ cp: cp, len: 4, valid: true };
            }
        }
        return UxidRune{ cp: 0xFFFD, len: 1, valid: false };
    }
    return UxidRune{ cp: 0xFFFD, len: 1, valid: false };
}

// Sorted non-overlapping non-ASCII XID_Start ranges (Unicode 15.1 major scripts)
const UXID_START_COUNT: usize = 80;

const UXID_START_LO: [UXID_START_COUNT]u32 = [
    0x00AA, 0x00B5, 0x00BA, 0x00C0, 0x00D8, 0x00F8, 0x02C6, 0x02E0,
    0x02EC, 0x02EE, 0x0370, 0x0376, 0x037B, 0x037F, 0x0386, 0x0388,
    0x038C, 0x038E, 0x03A3, 0x03F7, 0x048A, 0x0531, 0x0559, 0x0560,
    0x05D0, 0x05EF, 0x0620, 0x066E, 0x0671, 0x06D5, 0x06E5, 0x06EE,
    0x06FA, 0x06FF, 0x0904, 0x093D, 0x0950, 0x0958, 0x0971, 0x0E01,
    0x0E32, 0x0E40, 0x1100, 0x1E00, 0x1F00, 0x1F18, 0x1F20, 0x1F48,
    0x1F50, 0x1F59, 0x1F5B, 0x1F5D, 0x1F5F, 0x1F80, 0x1FB6, 0x1FC2,
    0x1FC6, 0x1FD0, 0x1FD6, 0x1FE0, 0x1FF2, 0x1FF6, 0x2102, 0x210A,
    0x2115, 0x2118, 0x2124, 0x2126, 0x212A, 0x212F, 0x3005, 0x3041,
    0x309D, 0x30A1, 0x30FC, 0x3105, 0x3400, 0x4E00, 0xAC00, 0x20000,
];

const UXID_START_HI: [UXID_START_COUNT]u32 = [
    0x00AA, 0x00B5, 0x00BA, 0x00D6, 0x00F6, 0x02C1, 0x02D1, 0x02E4,
    0x02EC, 0x02EE, 0x0374, 0x0377, 0x037D, 0x037F, 0x0386, 0x038A,
    0x038C, 0x03A1, 0x03F5, 0x0481, 0x052F, 0x0556, 0x0559, 0x0588,
    0x05EA, 0x05F2, 0x064A, 0x066F, 0x06D3, 0x06D5, 0x06E6, 0x06EF,
    0x06FC, 0x06FF, 0x0939, 0x093D, 0x0950, 0x0961, 0x0980, 0x0E30,
    0x0E32, 0x0E46, 0x1248, 0x1EFF, 0x1F15, 0x1F1D, 0x1F45, 0x1F4D,
    0x1F57, 0x1F59, 0x1F5B, 0x1F5D, 0x1F7D, 0x1FB4, 0x1FBC, 0x1FC4,
    0x1FCC, 0x1FD3, 0x1FDB, 0x1FEC, 0x1FF4, 0x1FFC, 0x2102, 0x2113,
    0x2115, 0x211D, 0x2124, 0x2126, 0x212D, 0x2139, 0x3007, 0x3096,
    0x309F, 0x30FA, 0x30FF, 0x318E, 0x4DBF, 0xA48C, 0xD7A3, 0x2A6DF,
];

// Additional non-ASCII XID_Start ranges above U+A48C not covered by the main table
fn uxidIsExtraStart(cp: u32): bool {
    if (cp >= 0xA960 && cp <= 0xA97C) { return true; }
    if ((cp >= 0xD7B0 && cp <= 0xD7C6) || (cp >= 0xD7CB && cp <= 0xD7FB)) { return true; }
    if ((cp >= 0xF900 && cp <= 0xFA6D) || (cp >= 0xFA70 && cp <= 0xFAD9)) { return true; }
    if ((cp >= 0xFF21 && cp <= 0xFF3A) || (cp >= 0xFF41 && cp <= 0xFF5A) || (cp >= 0xFF66 && cp <= 0xFFBE)) { return true; }
    if (cp >= 0x2A700 && cp <= 0x2EBE0) { return true; }
    if (cp >= 0x2F800 && cp <= 0x2FA1D) { return true; }
    if (cp >= 0x30000 && cp <= 0x3134A) { return true; }
    return false;
}

// Sorted non-overlapping non-ASCII XID_Continue delta ranges (combining marks, digits, connectors)
const UXID_CONT_COUNT: usize = 38;

const UXID_CONT_LO: [UXID_CONT_COUNT]u32 = [
    0x00B7, 0x0300, 0x0387, 0x0483, 0x0591, 0x05BF, 0x05C1, 0x05C4,
    0x05C7, 0x0610, 0x064B, 0x0670, 0x06D6, 0x06DF, 0x06E7, 0x06EA,
    0x06F0, 0x0900, 0x093A, 0x093E, 0x0951, 0x0962, 0x0966, 0x0E31,
    0x0E34, 0x0E47, 0x0E50, 0x1AB0, 0x1DC0, 0x203F, 0x2054, 0x20D0,
    0x302A, 0x3099, 0xFE00, 0xFE20, 0xFF10, 0xE0100,
];

const UXID_CONT_HI: [UXID_CONT_COUNT]u32 = [
    0x00B7, 0x036F, 0x0387, 0x0487, 0x05BD, 0x05BF, 0x05C2, 0x05C5,
    0x05C7, 0x061A, 0x0669, 0x0670, 0x06DC, 0x06E4, 0x06E8, 0x06ED,
    0x06F9, 0x0903, 0x093C, 0x094F, 0x0957, 0x0963, 0x096F, 0x0E31,
    0x0E3A, 0x0E4E, 0x0E59, 0x1ABD, 0x1DFF, 0x2040, 0x2054, 0x20DC,
    0x302F, 0x309A, 0xFE0F, 0xFE4F, 0xFF19, 0xE01EF,
];

fn uxidBsearchStart(cp: u32): bool {
    let lo: usize = 0;
    let hi: usize = UXID_START_COUNT;
    while (lo < hi) {
        const mid: usize = lo + ((hi - lo) >> (1 as u6));
        const r_lo: u32 = UXID_START_LO[mid];
        const r_hi: u32 = UXID_START_HI[mid];
        if (cp < r_lo) {
            hi = mid;
        } else if (cp > r_hi) {
            lo = mid + 1;
        } else {
            return true;
        }
    }
    return false;
}

fn uxidBsearchCont(cp: u32): bool {
    let lo: usize = 0;
    let hi: usize = UXID_CONT_COUNT;
    while (lo < hi) {
        const mid: usize = lo + ((hi - lo) >> (1 as u6));
        const r_lo: u32 = UXID_CONT_LO[mid];
        const r_hi: u32 = UXID_CONT_HI[mid];
        if (cp < r_lo) {
            hi = mid;
        } else if (cp > r_hi) {
            lo = mid + 1;
        } else {
            return true;
        }
    }
    return false;
}

fn uxidIsAsciiAlpha(b: u8): bool {
    return (b >= (0x41 as u8) && b <= (0x5A as u8))
        || (b >= (0x61 as u8) && b <= (0x7A as u8));
}

fn uxidIsAsciiDigit(b: u8): bool {
    return b >= (0x30 as u8) && b <= (0x39 as u8);
}

// ---------------------------------------------------------------------------
// Public UAX #31 API
// ---------------------------------------------------------------------------

/// Returns `true` if `cp` has the UAX #31 `XID_Start` property.
pub fn isXidStart(cp: u32): bool {
    // Fast ASCII path
    if (cp < (0x80 as u32)) {
        return (cp >= (0x41 as u32) && cp <= (0x5A as u32))
            || (cp >= (0x61 as u32) && cp <= (0x7A as u32));
    }
    if (uxidBsearchStart(cp)) {
        return true;
    }
    return uxidIsExtraStart(cp);
}

/// Returns `true` if `cp` has the UAX #31 `XID_Continue` property.
pub fn isXidContinue(cp: u32): bool {
    // Fast ASCII path: A-Z, a-z, 0-9, '_'
    if (cp < (0x80 as u32)) {
        return (cp >= (0x41 as u32) && cp <= (0x5A as u32))
            || (cp >= (0x61 as u32) && cp <= (0x7A as u32))
            || (cp >= (0x30 as u32) && cp <= (0x39 as u32))
            || cp == (0x5F as u32);
    }
    if (isXidStart(cp)) {
        return true;
    }
    if (cp == (0xFF3F as u32)) {
        return true;
    }
    return uxidBsearchCont(cp);
}

/// Returns `true` if `cp` is a valid Rust/CP identifier start character (`'_'` or `XID_Start`).
pub fn isIdentStart(cp: u32): bool {
    return cp == (0x5F as u32) || isXidStart(cp);
}

/// Fast pure-ASCII identifier check (`[A-Za-z_][A-Za-z0-9_]*`).
pub fn isAsciiIdent(s: string): bool {
    if (s.len == 0) {
        return false;
    }
    const b0 = s[0];
    if (!uxidIsAsciiAlpha(b0) && b0 != (0x5F as u8)) {
        return false;
    }
    let i: usize = 1;
    while (i < s.len) {
        const b = s[i];
        if (!uxidIsAsciiAlpha(b) && !uxidIsAsciiDigit(b) && b != (0x5F as u8)) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Validates whether the entire UTF-8 string `s` is a legal UAX #31 identifier
/// (allows leading `'_'`, rejects empty strings, leading digits, symbols, and invalid UTF-8).
pub fn isValidIdent(s: string): bool {
    if (s.len == 0) {
        return false;
    }
    const r0 = uxidDecodeUtf8Strict(s, 0);
    if (!r0.valid || !isIdentStart(r0.cp)) {
        return false;
    }
    let pos: usize = r0.len;
    while (pos < s.len) {
        const rn = uxidDecodeUtf8Strict(s, pos);
        if (!rn.valid || !isXidContinue(rn.cp)) {
            return false;
        }
        pos += rn.len;
    }
    return true;
}

/// Scans the longest valid UAX #31 identifier starting at byte offset `start` in `s`,
/// returning its byte length (or `0` if no valid identifier begins at `start`).
pub fn scanIdentLen(s: string, start: usize): usize {
    if (start >= s.len) {
        return 0;
    }
    const r0 = uxidDecodeUtf8Strict(s, start);
    if (!r0.valid || !isIdentStart(r0.cp)) {
        return 0;
    }
    let pos: usize = start + r0.len;
    while (pos < s.len) {
        const rn = uxidDecodeUtf8Strict(s, pos);
        if (!rn.valid || !isXidContinue(rn.cp)) {
            break;
        }
        pos += rn.len;
    }
    return pos - start;
}
