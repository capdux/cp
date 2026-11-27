// @cap/percent-encoding — Pure CP zero-allocation WHATWG URL Standard percent-encoding
// Clean-room MIT implementation with full parity to Rust `percent-encoding` & WHATWG URL Standard.

import { findByteInBytes } from "cap:bin";

const PE_HEX_UPPER: string = "0123456789ABCDEF";

fn peBuildHexLut(): [256]u8 {
    let lut: [256]u8 = [255; 256];
    let c: usize = 48;
    while (c <= 57) {
        lut[c] = (c - 48) as u8;
        c += 1;
    }
    c = 65;
    while (c <= 70) {
        lut[c] = (c - 55) as u8;
        c += 1;
    }
    c = 97;
    while (c <= 102) {
        lut[c] = (c - 87) as u8;
        c += 1;
    }
    return lut;
}

const PE_HEX_LUT: [256]u8 = peBuildHexLut();

fn peBitMask(byte: u8): u32 {
    return (1 as u32) << ((byte & 31) as u5);
}

fn peHexUpper(nib: u8): u8 {
    return PE_HEX_UPPER[(nib & 0x0F) as usize];
}

fn peHexVal(ch: u8): u8 {
    return PE_HEX_LUT[ch as usize];
}

/// 128-bit ASCII bitmap for O(1) percent-encode membership lookup.
/// Non-ASCII bytes (>= 0x80) are always percent-encoded per WHATWG URL Standard.
pub struct AsciiSet {
    pub bits: [4]u32,

    pub static fn empty(): AsciiSet {
        let b: [4]u32 = [0, 0, 0, 0];
        return AsciiSet { bits: b };
    }

    pub fn add(*_, byte: u8) {
        if (byte >= 0x80) {
            return;
        }
        let wi: usize = (byte >> 5) as usize;
        _.bits[wi] = _.bits[wi] | peBitMask(byte);
    }

    pub fn remove(*_, byte: u8) {
        if (byte >= 0x80) {
            return;
        }
        let wi: usize = (byte >> 5) as usize;
        _.bits[wi] = _.bits[wi] & (~peBitMask(byte));
    }

    pub fn unionWith(other: AsciiSet): AsciiSet {
        let b: [4]u32 = [
            _.bits[0] | other.bits[0],
            _.bits[1] | other.bits[1],
            _.bits[2] | other.bits[2],
            _.bits[3] | other.bits[3]
        ];
        return AsciiSet { bits: b };
    }

    pub fn shouldPercentEncode(byte: u8): bool {
        if (byte >= 0x80) {
            return true;
        }
        let wi: usize = (byte >> 5) as usize;
        return (_.bits[wi] & peBitMask(byte)) != 0;
    }
}

fn peBuildControlsSet(): AsciiSet {
    let b: [4]u32 = [
        0xFFFFFFFF as u32,
        0 as u32,
        0 as u32,
        0x80000000 as u32
    ];
    return AsciiSet { bits: b };
}

fn peBuildFragmentSet(): AsciiSet {
    let s = peBuildControlsSet();
    s.add(0x20); // ' '
    s.add(0x22); // '"'
    s.add(0x3C); // '<'
    s.add(0x3E); // '>'
    s.add(0x60); // '`'
    return s;
}

fn peBuildQuerySet(): AsciiSet {
    let s = peBuildControlsSet();
    s.add(0x20); // ' '
    s.add(0x22); // '"'
    s.add(0x23); // '#'
    s.add(0x3C); // '<'
    s.add(0x3E); // '>'
    return s;
}

fn peBuildPathSet(): AsciiSet {
    let s = peBuildFragmentSet();
    s.add(0x23); // '#'
    s.add(0x3F); // '?'
    s.add(0x7B); // '{'
    s.add(0x7D); // '}'
    return s;
}

fn peBuildUserinfoSet(): AsciiSet {
    let s = peBuildPathSet();
    s.add(0x2F); // '/'
    s.add(0x3A); // ':'
    s.add(0x3B); // ';'
    s.add(0x3D); // '='
    s.add(0x40); // '@'
    s.add(0x5B); // '['
    s.add(0x5C); // '\'
    s.add(0x5D); // ']'
    s.add(0x5E); // '^'
    s.add(0x7C); // '|'
    return s;
}

fn peBuildComponentSet(): AsciiSet {
    let s = peBuildUserinfoSet();
    s.add(0x24); // '$'
    s.add(0x25); // '%'
    s.add(0x26); // '&'
    s.add(0x2B); // '+'
    s.add(0x2C); // ','
    return s;
}

fn peBuildNonAlphanumericSet(): AsciiSet {
    let b: [4]u32 = [
        0xFFFFFFFF as u32,
        0xFFFFFFFF as u32,
        0xFFFFFFFF as u32,
        0xFFFFFFFF as u32
    ];
    let s = AsciiSet { bits: b };
    let c: u8 = 48; // '0'..'9'
    while (c <= 57) {
        s.remove(c);
        c = c + 1;
    }
    c = 65; // 'A'..'Z'
    while (c <= 90) {
        s.remove(c);
        c = c + 1;
    }
    c = 97; // 'a'..'z'
    while (c <= 122) {
        s.remove(c);
        c = c + 1;
    }
    return s;
}

const PE_CONTROLS_SET: AsciiSet = peBuildControlsSet();
const PE_FRAGMENT_SET: AsciiSet = peBuildFragmentSet();
const PE_QUERY_SET: AsciiSet = peBuildQuerySet();
const PE_PATH_SET: AsciiSet = peBuildPathSet();
const PE_USERINFO_SET: AsciiSet = peBuildUserinfoSet();
const PE_COMPONENT_SET: AsciiSet = peBuildComponentSet();
const PE_NON_ALNUM_SET: AsciiSet = peBuildNonAlphanumericSet();

/// WHATWG C0 control percent-encode set: 0x00..=0x1F, 0x7F, and all non-ASCII (>= 0x80).
pub fn controlsSet(): AsciiSet {
    return PE_CONTROLS_SET;
}

/// WHATWG fragment percent-encode set: controls + ' ', '"', '<', '>', '`'.
pub fn fragmentSet(): AsciiSet {
    return PE_FRAGMENT_SET;
}

/// WHATWG query percent-encode set: controls + ' ', '"', '#', '<', '>'.
pub fn querySet(): AsciiSet {
    return PE_QUERY_SET;
}

/// WHATWG path percent-encode set: fragment + '#', '?', '{', '}'.
pub fn pathSet(): AsciiSet {
    return PE_PATH_SET;
}

/// WHATWG userinfo percent-encode set: path + '/', ':', ';', '=', '@', '[', '\', ']', '^', '|'.
pub fn userinfoSet(): AsciiSet {
    return PE_USERINFO_SET;
}

/// WHATWG component percent-encode set: userinfo + '$', '%', '&', '+', ','.
pub fn componentSet(): AsciiSet {
    return PE_COMPONENT_SET;
}

/// Non-alphanumeric percent-encode set: encodes all bytes except ASCII A-Z, a-z, 0-9.
pub fn nonAlphanumericSet(): AsciiSet {
    return PE_NON_ALNUM_SET;
}

/// Computes the exact encoded byte length for `input` under `set`.
pub fn percentEncodeLen(input: []const u8, set: AsciiSet): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (set.shouldPercentEncode(input[i])) {
            total = total + 3;
        } else {
            total = total + 1;
        }
        i = i + 1;
    }
    return total;
}

/// Percent-encodes `input` into `out` using uppercase `%XX` notation.
/// Automatically encodes all non-ASCII bytes (>= 0x80).
/// Stops safely without overflowing `out` or emitting truncated `%` sequences.
pub fn percentEncode(input: []const u8, set: AsciiSet, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len && pos < out.len) {
        const seg_start = i;
        while (i < input.len && !set.shouldPercentEncode(input[i])) {
            i = i + 1;
        }
        if (i > seg_start) {
            const seg_len = i - seg_start;
            const avail = out.len - pos;
            const n = if (seg_len < avail) seg_len else avail;
            @memcpy(out[pos .. pos + n], input[seg_start .. seg_start + n]);
            pos = pos + n;
            if (n < seg_len || i >= input.len) {
                break;
            }
        }
        if (pos + 3 > out.len) {
            break;
        }
        const b = input[i];
        out[pos] = 0x25; // '%'
        out[pos + 1] = peHexUpper(b >> 4);
        out[pos + 2] = peHexUpper(b & 0x0F);
        pos = pos + 3;
        i = i + 1;
    }
    return pos;
}

/// Lenient percent-decoding (Rust `percent_decode` parity).
/// Valid `%XX` sequences (case-insensitive) are decoded to bytes;
/// malformed or incomplete `%` sequences are preserved verbatim.
pub fn percentDecode(input: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len && pos < out.len) {
        const next_pct = findByteInBytes(input, 0x25, i) ?? input.len;
        if (next_pct > i) {
            const seg_len = next_pct - i;
            const avail = out.len - pos;
            const n = if (seg_len < avail) seg_len else avail;
            @memcpy(out[pos .. pos + n], input[i .. i + n]);
            pos = pos + n;
            i = next_pct;
            if (n < seg_len || i >= input.len || pos >= out.len) {
                break;
            }
        }
        if (i + 2 < input.len) {
            const hi = PE_HEX_LUT[input[i + 1] as usize];
            const lo = PE_HEX_LUT[input[i + 2] as usize];
            if (hi != 255 && lo != 255) {
                out[pos] = ((hi << 4) | lo) as u8;
                pos = pos + 1;
                i = i + 3;
                continue;
            }
        }
        out[pos] = 0x25;
        pos = pos + 1;
        i = i + 1;
    }
    return pos;
}

/// Strict percent-decoding.
/// Returns the decoded byte length (>= 0), or -1 if any `%` is incomplete,
/// has non-hex digits, or `out` is too small.
pub fn percentDecodeStrict(input: string, out: []u8): i64 {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const next_pct = findByteInBytes(input, 0x25, i) ?? input.len;
        if (next_pct > i) {
            const seg_len = next_pct - i;
            if (pos + seg_len > out.len) {
                return -1;
            }
            @memcpy(out[pos .. pos + seg_len], input[i .. next_pct]);
            pos = pos + seg_len;
            i = next_pct;
            if (i >= input.len) {
                break;
            }
        }
        if (i + 2 >= input.len) {
            return -1;
        }
        const hi = PE_HEX_LUT[input[i + 1] as usize];
        const lo = PE_HEX_LUT[input[i + 2] as usize];
        if (hi == 255 || lo == 255) {
            return -1;
        }
        if (pos >= out.len) {
            return -1;
        }
        out[pos] = ((hi << 4) | lo) as u8;
        pos = pos + 1;
        i = i + 3;
    }
    return pos as i64;
}

/// Form `application/x-www-form-urlencoded` compatible decoding.
/// Decodes `+` to space (`0x20`) and valid `%XX` sequences to bytes (lenient on malformed `%`).
pub fn percentDecodePlus(input: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    let next_pct = findByteInBytes(input, 0x25, 0) ?? input.len;
    let next_plus = findByteInBytes(input, 0x2B, 0) ?? input.len;
    while (i < input.len && pos < out.len) {
        if (next_pct < i) {
            next_pct = findByteInBytes(input, 0x25, i) ?? input.len;
        }
        if (next_plus < i) {
            next_plus = findByteInBytes(input, 0x2B, i) ?? input.len;
        }
        const next_sp = if (next_pct < next_plus) next_pct else next_plus;
        if (next_sp > i) {
            const seg_len = next_sp - i;
            const avail = out.len - pos;
            const n = if (seg_len < avail) seg_len else avail;
            @memcpy(out[pos .. pos + n], input[i .. i + n]);
            pos = pos + n;
            i = next_sp;
            if (n < seg_len || i >= input.len || pos >= out.len) {
                break;
            }
        }
        const b = input[i];
        if (b == 0x2B) { // '+'
            out[pos] = 0x20; // ' '
            pos = pos + 1;
            i = i + 1;
            continue;
        }
        if (i + 2 < input.len) {
            const hi = PE_HEX_LUT[input[i + 1] as usize];
            const lo = PE_HEX_LUT[input[i + 2] as usize];
            if (hi != 255 && lo != 255) {
                out[pos] = ((hi << 4) | lo) as u8;
                pos = pos + 1;
                i = i + 3;
                continue;
            }
        }
        out[pos] = b;
        pos = pos + 1;
        i = i + 1;
    }
    return pos;
}

export {
    AsciiSet,
    controlsSet,
    fragmentSet,
    querySet,
    pathSet,
    userinfoSet,
    componentSet,
    nonAlphanumericSet,
    percentEncodeLen,
    percentEncode,
    percentDecode,
    percentDecodeStrict,
    percentDecodePlus
};
