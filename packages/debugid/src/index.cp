// @cap/debugid — Zero-heap Sentry/Breakpad/PDB DebugId parser & formatter in pure CP
// Clean-room Apache-2.0 implementation of crates.io `debugid` (#723, 171.3M downloads).

pub struct DebugId {
    pub bytes: [16]u8,
    pub appendix: u32,
    pub valid: bool,

    pub fn isNil(): bool {
        if (!_.valid) { return true; }
        let i: usize = 0;
        while (i < 16) {
            if (_.bytes[i] != 0) { return false; }
            i += 1;
        }
        return _.appendix == 0;
    }
}

fn dbgHexVal(b: u8): u8? {
    if (b >= 48 && b <= 57) { return b - 48; }
    if (b >= 65 && b <= 70) { return b - 55; }
    if (b >= 97 && b <= 102) { return b - 87; }
    return null;
}

fn dbgHexChar(n: u8, upper: bool): u8 {
    if (n < 10) { return 48 + n; }
    if (upper) { return 55 + n; }
    return 87 + n;
}

/// Parses a Breakpad debug ID (33..40 hex chars: 32-hex UUID + 1..8 hex appendix) or
/// canonical hyphenated UUID with optional `-<hex-appendix>` (e.g. `dfb8e43a-f242-3d73-a453-aeb6a777ef75-1`).
pub fn parseDebugId(s: string): DebugId {
    const inv = DebugId { bytes: [0; 16], appendix: 0, valid: false };
    if (s.len < 32) {
        return inv;
    }

    let raw: [16]u8 = [0; 16];
    let byte_idx: usize = 0;
    let i: usize = 0;
    let has_hi = false;
    let hi_nib: u8 = 0;
    let is_hyphenated = false;

    while (i < s.len && byte_idx < 16) {
        const c = s[i];
        if (c == 45) { // '-'
            is_hyphenated = true;
            i += 1;
            continue;
        }
        const hv = dbgHexVal(c);
        if (hv == null) {
            return inv;
        }
        const nib = hv ?? 0;
        if (!has_hi) {
            hi_nib = nib;
            has_hi = true;
        } else {
            raw[byte_idx] = (hi_nib << 4) | nib;
            byte_idx += 1;
            has_hi = false;
        }
        i += 1;
    }

    if (byte_idx != 16 || has_hi) {
        return inv;
    }

    let appendix: u32 = 0;
    if (i < s.len) {
        if (is_hyphenated) {
            if (s[i] != 45) { return inv; }
            i += 1;
        }
        if (i >= s.len || (s.len - i) > 8) {
            return inv;
        }
        while (i < s.len) {
            const hv = dbgHexVal(s[i]);
            if (hv == null) { return inv; }
            appendix = (appendix << 4) | ((hv ?? 0) as u32);
            i += 1;
        }
    }

    return DebugId { bytes: raw, appendix: appendix, valid: true };
}

fn dbgWriteHexByte(b: u8, upper: bool, out: []u8, pos: usize): usize {
    if (pos + 1 < out.len) {
        out[pos] = dbgHexChar((b >> 4) & 0x0F, upper);
        out[pos + 1] = dbgHexChar(b & 0x0F, upper);
        return pos + 2;
    }
    return pos;
}

fn dbgWriteHexU32(val: u32, upper: bool, out: []u8, pos: usize): usize {
    let digits: [8]u8 = [0; 8];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        digits[0] = 48;
        n = 1;
    } else {
        while (v > 0 && n < 8) {
            digits[n] = dbgHexChar((v & 0x0F) as u8, upper);
            v = v >> 4;
            n += 1;
        }
    }
    let p = pos;
    let k = n;
    while (k > 0 && p < out.len) {
        k -= 1;
        out[p] = digits[k];
        p += 1;
    }
    return p;
}

/// Formats a `DebugId` in Breakpad uppercase format (`32-HEX-UUID` + `APPENDIX-HEX`).
pub fn formatBreakpad(id: DebugId, out: []u8): usize {
    if (!id.valid) { return 0; }
    let p: usize = 0;
    let i: usize = 0;
    while (i < 16) {
        p = dbgWriteHexByte(id.bytes[i], true, out, p);
        i += 1;
    }
    p = dbgWriteHexU32(id.appendix, true, out, p);
    return p;
}

/// Formats a `DebugId` in canonical Sentry hyphenated lowercase format (`uuid[-appendix]`).
pub fn formatCanonical(id: DebugId, out: []u8): usize {
    if (!id.valid) { return 0; }
    let p: usize = 0;
    let i: usize = 0;
    while (i < 16) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            if (p < out.len) { out[p] = 45; p += 1; }
        }
        p = dbgWriteHexByte(id.bytes[i], false, out, p);
        i += 1;
    }
    if (id.appendix != 0) {
        if (p < out.len) { out[p] = 45; p += 1; }
        p = dbgWriteHexU32(id.appendix, false, out, p);
    }
    return p;
}

export {
    DebugId,
    parseDebugId,
    formatBreakpad,
    formatCanonical,
};
