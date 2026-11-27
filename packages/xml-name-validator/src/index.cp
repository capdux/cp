// @cap/xml-name-validator — W3C XML 1.0 (5th Ed.) & Namespaces in XML 1.0 validator
// Inspired by npm:xml-name-validator (https://github.com/jsdom/xml-name-validator)
// Accelerated with comptime 256-entry ASCII LUT and 16-lane @Vector(16, u8) SIMD fast paths.

type XnvVec16 = @Vector(16, u8);

const XNV_FLAG_START: u8 = 1;
const XNV_FLAG_CHAR: u8 = 2;
const XNV_FLAG_COLON: u8 = 4;

fn xnvBuildAsciiLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 128) {
        if ((c >= 0x41 && c <= 0x5A) || (c >= 0x61 && c <= 0x7A) || c == 0x5F) {
            lut[c] = XNV_FLAG_START | XNV_FLAG_CHAR;
        } else if ((c >= 0x30 && c <= 0x39) || c == 0x2D || c == 0x2E) {
            lut[c] = XNV_FLAG_CHAR;
        } else if (c == 0x3A) {
            lut[c] = XNV_FLAG_START | XNV_FLAG_CHAR | XNV_FLAG_COLON;
        }
        c += 1;
    }
    return lut;
}

const XNV_ASCII_LUT: [256]u8 = xnvBuildAsciiLut();

struct XnvDecoded {
    cp: u32,
    len: usize,
    ok: bool,
}

fn xnvDecodeUtf8(s: string, pos: usize): XnvDecoded {
    if (pos >= s.len) {
        return XnvDecoded{ cp: 0, len: 0, ok: false };
    }
    const b0: u8 = s[pos];
    if (b0 <= 0x7F) {
        return XnvDecoded{ cp: u32(b0), len: 1, ok: true };
    }
    if (b0 >= 0xC2 && b0 <= 0xDF) {
        if (pos + 1 >= s.len) {
            return XnvDecoded{ cp: 0, len: 0, ok: false };
        }
        const b1: u8 = s[pos + 1];
        if ((b1 & 0xC0) != 0x80) {
            return XnvDecoded{ cp: 0, len: 0, ok: false };
        }
        const cp: u32 = (u32(b0 & 0x1F) << 6) | u32(b1 & 0x3F);
        return XnvDecoded{ cp: cp, len: 2, ok: true };
    }
    if (b0 >= 0xE0 && b0 <= 0xEF) {
        if (pos + 2 >= s.len) {
            return XnvDecoded{ cp: 0, len: 0, ok: false };
        }
        const b1: u8 = s[pos + 1];
        const b2: u8 = s[pos + 2];
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) {
            return XnvDecoded{ cp: 0, len: 0, ok: false };
        }
        if (b0 == 0xE0 && b1 < 0xA0) {
            return XnvDecoded{ cp: 0, len: 0, ok: false };
        }
        if (b0 == 0xED && b1 >= 0xA0) {
            return XnvDecoded{ cp: 0, len: 0, ok: false };
        }
        const cp: u32 = (u32(b0 & 0x0F) << 12) | (u32(b1 & 0x3F) << 6) | u32(b2 & 0x3F);
        return XnvDecoded{ cp: cp, len: 3, ok: true };
    }
    if (b0 >= 0xF0 && b0 <= 0xF4) {
        if (pos + 3 >= s.len) {
            return XnvDecoded{ cp: 0, len: 0, ok: false };
        }
        const b1: u8 = s[pos + 1];
        const b2: u8 = s[pos + 2];
        const b3: u8 = s[pos + 3];
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) {
            return XnvDecoded{ cp: 0, len: 0, ok: false };
        }
        if (b0 == 0xF0 && b1 < 0x90) {
            return XnvDecoded{ cp: 0, len: 0, ok: false };
        }
        if (b0 == 0xF4 && b1 > 0x8F) {
            return XnvDecoded{ cp: 0, len: 0, ok: false };
        }
        const cp: u32 =
            (u32(b0 & 0x07) << 18) |
            (u32(b1 & 0x3F) << 12) |
            (u32(b2 & 0x3F) << 6) |
            u32(b3 & 0x3F);
        return XnvDecoded{ cp: cp, len: 4, ok: true };
    }
    return XnvDecoded{ cp: 0, len: 0, ok: false };
}

fn xnvValidateName(s: string, allow_colon: bool): bool {
    if (s.len == 0) {
        return false;
    }

    let pos: usize = 0;
    const b0 = s[0];
    if (b0 < 0x80) {
        const flags = XNV_ASCII_LUT[b0 as usize];
        if ((flags & XNV_FLAG_START) == 0) {
            return false;
        }
        if (!allow_colon && (flags & XNV_FLAG_COLON) != 0) {
            return false;
        }
        pos = 1;
    } else {
        const first: XnvDecoded = xnvDecodeUtf8(s, 0);
        if (!first.ok) {
            return false;
        }
        if (!isNameStartChar(first.cp)) {
            return false;
        }
        pos = first.len;
    }

    const v_a: XnvVec16 = @splat(0x61 as u8);
    const v_z: XnvVec16 = @splat(0x7A as u8);
    const v_0: XnvVec16 = @splat(0x30 as u8);
    const v_9: XnvVec16 = @splat(0x39 as u8);
    const v_case: XnvVec16 = @splat(0x20 as u8);
    const v_under: XnvVec16 = @splat(0x5F as u8);
    const v_dash: XnvVec16 = @splat(0x2D as u8);
    const v_dot: XnvVec16 = @splat(0x2E as u8);
    const v_colon: XnvVec16 = @splat(0x3A as u8);

    while (pos < s.len) {
        // 16-lane SIMD ASCII NameChar fast path
        while (pos + 16 <= s.len) {
            const v: XnvVec16 = s[pos .. pos + 16][0..16].*;
            const vl = v | v_case;
            let ok = ((vl >= v_a) & (vl <= v_z)) |
                ((v >= v_0) & (v <= v_9)) |
                (v == v_under) | (v == v_dash) | (v == v_dot);
            if (allow_colon) {
                ok = ok | (v == v_colon);
            }
            if (!@reduce(.And, ok)) {
                break;
            }
            pos += 16;
        }
        if (pos >= s.len) {
            break;
        }
        const b = s[pos];
        if (b < 0x80) {
            const flags = XNV_ASCII_LUT[b as usize];
            if ((flags & XNV_FLAG_CHAR) == 0) {
                return false;
            }
            if (!allow_colon && (flags & XNV_FLAG_COLON) != 0) {
                return false;
            }
            pos += 1;
        } else {
            const d: XnvDecoded = xnvDecodeUtf8(s, pos);
            if (!d.ok) {
                return false;
            }
            if (!isNameChar(d.cp)) {
                return false;
            }
            pos += d.len;
        }
    }
    return true;
}

fn xnvFindColon(s: string): usize {
    let i: usize = 0;
    const len = s.len;
    const v_colon: XnvVec16 = @splat(0x3A as u8);
    while (i + 16 <= len) {
        const v: XnvVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_colon)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == 0x3A) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (s[i] == 0x3A) {
            return i;
        }
        i += 1;
    }
    return len;
}

/// Returns true if `cp` matches W3C XML 1.0 (5th Ed.) §2.2 `Char`:
/// `#x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]`
pub fn isXmlChar(cp: u32): bool {
    if (cp == 0x09 || cp == 0x0A || cp == 0x0D) {
        return true;
    }
    if (cp >= 0x20 && cp <= 0xD7FF) {
        return true;
    }
    if (cp >= 0xE000 && cp <= 0xFFFD) {
        return true;
    }
    if (cp >= 0x10000 && cp <= 0x10FFFF) {
        return true;
    }
    return false;
}

/// Returns true if `cp` matches W3C XML 1.0 (5th Ed.) §2.3 `NameStartChar`.
pub fn isNameStartChar(cp: u32): bool {
    if (cp < 0x80) {
        return (XNV_ASCII_LUT[cp as usize] & XNV_FLAG_START) != 0;
    }
    if (cp >= 0xC0 && cp <= 0xD6) {
        return true;
    }
    if (cp >= 0xD8 && cp <= 0xF6) {
        return true;
    }
    if (cp >= 0xF8 && cp <= 0x2FF) {
        return true;
    }
    if (cp >= 0x370 && cp <= 0x37D) {
        return true;
    }
    if (cp >= 0x37F && cp <= 0x1FFF) {
        return true;
    }
    if (cp == 0x200C || cp == 0x200D) {
        return true;
    }
    if (cp >= 0x2070 && cp <= 0x218F) {
        return true;
    }
    if (cp >= 0x2C00 && cp <= 0x2FEF) {
        return true;
    }
    if (cp >= 0x3001 && cp <= 0xD7FF) {
        return true;
    }
    if (cp >= 0xF900 && cp <= 0xFDCF) {
        return true;
    }
    if (cp >= 0xFDF0 && cp <= 0xFFFD) {
        return true;
    }
    if (cp >= 0x10000 && cp <= 0xEFFFF) {
        return true;
    }
    return false;
}

/// Returns true if `cp` matches W3C XML 1.0 (5th Ed.) §2.3 `NameChar`.
pub fn isNameChar(cp: u32): bool {
    if (cp < 0x80) {
        return (XNV_ASCII_LUT[cp as usize] & XNV_FLAG_CHAR) != 0;
    }
    if (isNameStartChar(cp)) {
        return true;
    }
    if (cp == 0xB7) {
        return true;
    }
    if (cp >= 0x0300 && cp <= 0x036F) {
        return true;
    }
    if (cp >= 0x203F && cp <= 0x2040) {
        return true;
    }
    return false;
}

/// Validates a UTF-8 string against the W3C XML 1.0 `Name` production.
pub fn isValidXmlName(s: string): bool {
    return xnvValidateName(s, true);
}

/// Validates a UTF-8 string against Namespaces in XML 1.0 `NCName` (colon-free `Name`).
pub fn isValidNCName(s: string): bool {
    return xnvValidateName(s, false);
}

/// Validates a UTF-8 string against Namespaces in XML 1.0 `QName` (`NCName` or `NCName:NCName`).
pub fn isValidQName(s: string): bool {
    if (s.len == 0) {
        return false;
    }
    const colon = xnvFindColon(s);
    if (colon == s.len) {
        return isValidNCName(s);
    }
    if (colon == 0 || colon + 1 >= s.len) {
        return false;
    }
    return isValidNCName(s[0..colon]) && isValidNCName(s[colon + 1..s.len]);
}
