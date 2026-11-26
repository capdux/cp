// @cap/percent-encoding — Pure CP zero-allocation WHATWG URL Standard percent-encoding
// Clean-room MIT implementation with full parity to Rust `percent-encoding` & WHATWG URL Standard.

fn peBitMask(byte: u8): u32 {
    return (1 as u32) << ((byte & 31) as u5);
}

fn peHexUpper(nib: u8): u8 {
    let n = nib & 0x0F;
    if (n < 10) {
        return 48 + n;
    }
    return 55 + n;
}

fn peHexVal(ch: u8): u8 {
    if (ch >= 48 && ch <= 57) {
        return ch - 48;
    }
    if (ch >= 65 && ch <= 70) {
        return ch - 55;
    }
    if (ch >= 97 && ch <= 102) {
        return ch - 87;
    }
    return 255;
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

/// WHATWG C0 control percent-encode set: 0x00..=0x1F, 0x7F, and all non-ASCII (>= 0x80).
pub fn controlsSet(): AsciiSet {
    let b: [4]u32 = [
        0xFFFFFFFF as u32,
        0 as u32,
        0 as u32,
        0x80000000 as u32
    ];
    return AsciiSet { bits: b };
}

/// WHATWG fragment percent-encode set: controls + ' ', '"', '<', '>', '`'.
pub fn fragmentSet(): AsciiSet {
    let s = controlsSet();
    s.add(0x20); // ' '
    s.add(0x22); // '"'
    s.add(0x3C); // '<'
    s.add(0x3E); // '>'
    s.add(0x60); // '`'
    return s;
}

/// WHATWG query percent-encode set: controls + ' ', '"', '#', '<', '>'.
pub fn querySet(): AsciiSet {
    let s = controlsSet();
    s.add(0x20); // ' '
    s.add(0x22); // '"'
    s.add(0x23); // '#'
    s.add(0x3C); // '<'
    s.add(0x3E); // '>'
    return s;
}

/// WHATWG path percent-encode set: fragment + '#', '?', '{', '}'.
pub fn pathSet(): AsciiSet {
    let s = fragmentSet();
    s.add(0x23); // '#'
    s.add(0x3F); // '?'
    s.add(0x7B); // '{'
    s.add(0x7D); // '}'
    return s;
}

/// WHATWG userinfo percent-encode set: path + '/', ':', ';', '=', '@', '[', '\', ']', '^', '|'.
pub fn userinfoSet(): AsciiSet {
    let s = pathSet();
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

/// WHATWG component percent-encode set: userinfo + '$', '%', '&', '+', ','.
pub fn componentSet(): AsciiSet {
    let s = userinfoSet();
    s.add(0x24); // '$'
    s.add(0x25); // '%'
    s.add(0x26); // '&'
    s.add(0x2B); // '+'
    s.add(0x2C); // ','
    return s;
}

/// Non-alphanumeric percent-encode set: encodes all bytes except ASCII A-Z, a-z, 0-9.
pub fn nonAlphanumericSet(): AsciiSet {
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
    while (i < input.len) {
        let b = input[i];
        if (set.shouldPercentEncode(b)) {
            if (pos + 3 > out.len) {
                break;
            }
            out[pos] = 0x25; // '%'
            out[pos + 1] = peHexUpper(b >> 4);
            out[pos + 2] = peHexUpper(b & 0x0F);
            pos = pos + 3;
        } else {
            if (pos >= out.len) {
                break;
            }
            out[pos] = b;
            pos = pos + 1;
        }
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
    while (i < input.len) {
        if (pos >= out.len) {
            break;
        }
        let b = input[i];
        if (b == 0x25 && i + 2 < input.len) {
            let hi = peHexVal(input[i + 1]);
            let lo = peHexVal(input[i + 2]);
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

/// Strict percent-decoding.
/// Returns the decoded byte length (>= 0), or -1 if any `%` is incomplete,
/// has non-hex digits, or `out` is too small.
pub fn percentDecodeStrict(input: string, out: []u8): i64 {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        let b = input[i];
        if (b == 0x25) {
            if (i + 2 >= input.len) {
                return -1;
            }
            let hi = peHexVal(input[i + 1]);
            let lo = peHexVal(input[i + 2]);
            if (hi == 255 || lo == 255) {
                return -1;
            }
            if (pos >= out.len) {
                return -1;
            }
            out[pos] = ((hi << 4) | lo) as u8;
            pos = pos + 1;
            i = i + 3;
        } else {
            if (pos >= out.len) {
                return -1;
            }
            out[pos] = b;
            pos = pos + 1;
            i = i + 1;
        }
    }
    return pos as i64;
}

/// Form `application/x-www-form-urlencoded` compatible decoding.
/// Decodes `+` to space (`0x20`) and valid `%XX` sequences to bytes (lenient on malformed `%`).
pub fn percentDecodePlus(input: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (pos >= out.len) {
            break;
        }
        let b = input[i];
        if (b == 0x2B) { // '+'
            out[pos] = 0x20; // ' '
            pos = pos + 1;
            i = i + 1;
            continue;
        }
        if (b == 0x25 && i + 2 < input.len) {
            let hi = peHexVal(input[i + 1]);
            let lo = peHexVal(input[i + 2]);
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
