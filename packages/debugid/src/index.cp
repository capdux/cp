// @cap/debugid — Zero-heap Sentry/Breakpad/PDB DebugId parser & formatter in pure CP
// Clean-room Apache-2.0 implementation of crates.io `debugid` (#723, 171.3M downloads).

import { hexVal } from "cap:ascii";
import { BufWriter } from "cap:fmt";

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

/// Parses a Breakpad debug ID (33..40 hex chars: 32-hex UUID + 1..8 hex appendix) or
/// canonical hyphenated UUID with optional `-<hex-appendix>` (e.g. `dfb8e43a-f242-3d73-a453-aeb6a777ef75-1`).
pub fn parseDebugId(s: string): DebugId {
    const inv = DebugId { bytes: [0; 16], appendix: 0, valid: false };
    if (s.len < 32) {
        return inv;
    }

    let raw: [16]u8 = undefined;
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
        const hv = hexVal(c);
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
            const hv = hexVal(s[i]);
            if (hv == null) { return inv; }
            appendix = (appendix << 4) | ((hv ?? 0) as u32);
            i += 1;
        }
    }

    return DebugId { bytes: raw, appendix: appendix, valid: true };
}

/// Formats a `DebugId` in Breakpad uppercase format (`32-HEX-UUID` + `APPENDIX-HEX`).
pub fn formatBreakpad(id: DebugId, out: []u8): usize {
    if (!id.valid) { return 0; }
    let w = BufWriter.new(out);
    let i: usize = 0;
    while (i < 16) {
        w.writeHexByte(id.bytes[i], true);
        i += 1;
    }
    w.writeHexU64(id.appendix as u64, true);
    return w.written();
}

/// Formats a `DebugId` in canonical Sentry hyphenated lowercase format (`uuid[-appendix]`).
pub fn formatCanonical(id: DebugId, out: []u8): usize {
    if (!id.valid) { return 0; }
    let w = BufWriter.new(out);
    let i: usize = 0;
    while (i < 16) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            w.writeByte(45);
        }
        w.writeHexByte(id.bytes[i], false);
        i += 1;
    }
    if (id.appendix != 0) {
        w.writeByte(45);
        w.writeHexU64(id.appendix as u64, false);
    }
    return w.written();
}

export {
    DebugId,
    parseDebugId,
    formatBreakpad,
    formatCanonical,
};
