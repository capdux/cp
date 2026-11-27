// @cap/bech32 — Zero-allocation BIP-173 Bech32 & BIP-350 Bech32m codec for CP
// Clean-room MIT implementation inspired by Rust `bech32` and npm `bech32`.

import { toLowerByte, toLower } from "cap:ascii";
import { findByteInBytes } from "cap:bin";

pub const BECH32_CHARSET: string = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
const BECH32_CONST: u32 = 1;
const BECH32M_CONST: u32 = 0x2bc830a3;

pub enum Bech32Variant: u8 {
    Bech32 = 0,
    Bech32m = 1,
}

pub struct Bech32Decoded {
    pub valid: bool,
    pub variant: Bech32Variant,
    pub hrp: string,
    pub data: string,

    pub static fn invalid() -> Bech32Decoded {
        return Bech32Decoded{
            valid: false,
            variant: Bech32Variant.Bech32,
            hrp: "",
            data: "",
        };
    }
}

// ---------------------------------------------------------------------------
// Internal BCH Polymod & Character Helpers
// ---------------------------------------------------------------------------

fn charsetIndex(c: u8) -> u8? {
    const idx = findByteInBytes(BECH32_CHARSET, toLowerByte(c), 0);
    if (idx == null) {
        return null;
    }
    return idx.? as u8;
}

fn polymodStep(chk: u32, v: u8) -> u32 {
    const top = chk >> 25;
    let c = ((chk & 0x1ffffff) << 5) ^ (v as u32);
    if ((top & 1) != 0) c ^= 0x3b6a57b2;
    if ((top & 2) != 0) c ^= 0x26508e6d;
    if ((top & 4) != 0) c ^= 0x1ea119fa;
    if ((top & 8) != 0) c ^= 0x3d4233dd;
    if ((top & 16) != 0) c ^= 0x2a1462b3;
    return c;
}

fn polymodHrp(hrp: string) -> u32 {
    let chk: u32 = 1;
    let i: usize = 0;
    while (i < hrp.len) {
        const c = toLowerByte(hrp[i]);
        chk = polymodStep(chk, c >> 5);
        i += 1;
    }
    chk = polymodStep(chk, 0);
    i = 0;
    while (i < hrp.len) {
        const c = toLowerByte(hrp[i]);
        chk = polymodStep(chk, c & 31);
        i += 1;
    }
    return chk;
}

fn variantConst(v: Bech32Variant) -> u32 {
    switch (v) {
        .Bech32 => return BECH32_CONST,
        .Bech32m => return BECH32M_CONST,
    }
}

fn isValidHrp(hrp: string) -> bool {
    if (hrp.len == 0 || hrp.len > 83) {
        return false;
    }
    let i: usize = 0;
    while (i < hrp.len) {
        const c = hrp[i];
        if (c < 33 || c > 126) {
            return false;
        }
        i += 1;
    }
    return true;
}

// ---------------------------------------------------------------------------
// 1. 8-bit <-> 5-bit Word Conversion (`convertBits`)
// ---------------------------------------------------------------------------

/// Packs 8-bit bytes into 5-bit words (`0..31`) with trailing zero padding, writing into `out`.
/// Returns number of 5-bit words written, or `null` if `out` is too small.
pub fn to5BitWords(bytes: string, out: []u8) -> usize? {
    let acc: u32 = 0;
    let bits: u32 = 0;
    let pos: usize = 0;
    let i: usize = 0;
    while (i < bytes.len) {
        acc = (acc << 8) | (bytes[i] as u32);
        bits += 8;
        while (bits >= 5) {
            bits -= 5;
            if (pos >= out.len) {
                return null;
            }
            out[pos] = ((acc >> bits) & 0x1F) as u8;
            pos += 1;
        }
        i += 1;
    }
    if (bits > 0) {
        if (pos >= out.len) {
            return null;
        }
        out[pos] = ((acc << (5 - bits)) & 0x1F) as u8;
        pos += 1;
    }
    return pos;
}

/// Unpacks 5-bit words (`0..31`) back into 8-bit bytes in `out`.
/// Strictly rejects words `>= 32`, excessive padding (`>= 5` bits), or non-zero trailing bits.
pub fn from5BitWords(words: []const u8, out: []u8) -> usize? {
    let acc: u32 = 0;
    let bits: u32 = 0;
    let pos: usize = 0;
    let i: usize = 0;
    while (i < words.len) {
        const w = words[i];
        if (w >= 32) {
            return null;
        }
        acc = (acc << 5) | (w as u32);
        bits += 5;
        while (bits >= 8) {
            bits -= 8;
            if (pos >= out.len) {
                return null;
            }
            out[pos] = ((acc >> bits) & 0xFF) as u8;
            pos += 1;
        }
        i += 1;
    }
    if (bits >= 5) {
        return null;
    }
    if (bits > 0) {
        const mask: u32 = ((1 as u32) << bits) - 1;
        if ((acc & mask) != 0) {
            return null;
        }
    }
    return pos;
}

// ---------------------------------------------------------------------------
// 2. Core Bech32 / Bech32m Encoding, Decoding & Verification
// ---------------------------------------------------------------------------

/// Encodes `hrp` and raw `payload_bytes` (auto-converted to 5-bit words) into a lowercase
/// `<hrp>1<data><6-char-checksum>` string in `out` using BIP-173 (`Bech32`) or BIP-350 (`Bech32m`).
pub fn encodeBech32(hrp: string, payload_bytes: string, variant: Bech32Variant, out: []u8) -> string? {
    if (!isValidHrp(hrp)) {
        return null;
    }
    // Convert payload bytes into stack 5-bit words (up to 160 words = 100 bytes)
    let words: [256]u8 = undefined;
    const w_opt = to5BitWords(payload_bytes, words[0..256]);
    if (w_opt == null) {
        return null;
    }
    const wlen = w_opt.?;
    const total_len = hrp.len + 1 + wlen + 6;
    if (out.len < total_len) {
        return null;
    }

    let pos: usize = toLower(hrp, out);
    out[pos] = '1';
    pos += 1;

    let chk = polymodHrp(hrp);
    let i: usize = 0;
    while (i < wlen) {
        const w = words[i];
        chk = polymodStep(chk, w);
        out[pos] = BECH32_CHARSET[w as usize];
        pos += 1;
        i += 1;
    }

    // Feed 6 zero words to compute the 30-bit BCH checksum
    let z: usize = 0;
    while (z < 6) {
        chk = polymodStep(chk, 0);
        z += 1;
    }
    chk ^= variantConst(variant);

    let c_i: usize = 0;
    while (c_i < 6) {
        const shift: u32 = ((5 - c_i) * 5) as u32;
        const sym = ((chk >> shift) & 31) as usize;
        out[pos] = BECH32_CHARSET[sym];
        pos += 1;
        c_i += 1;
    }

    return out[0..pos];
}

struct ParsedBech32Meta {
    valid: bool,
    variant: Bech32Variant,
    sep: usize,
}

fn validateStructureAndChecksum(encoded: string) -> ParsedBech32Meta {
    const bad = ParsedBech32Meta{ valid: false, variant: Bech32Variant.Bech32, sep: 0 };
    if (encoded.len < 8) {
        return bad;
    }

    // Reject mixed case & non-printable ASCII; locate last '1' separator
    let has_lower = false;
    let has_upper = false;
    let sep: usize = encoded.len;
    let i: usize = 0;
    while (i < encoded.len) {
        const c = encoded[i];
        if (c < 33 || c > 126) {
            return bad;
        }
        if (c >= 'a' && c <= 'z') has_lower = true;
        if (c >= 'A' && c <= 'Z') has_upper = true;
        if (c == '1') {
            sep = i;
        }
        i += 1;
    }
    if (has_lower && has_upper) {
        return bad;
    }
    if (sep < 1 || sep > 83 || encoded.len - sep < 7) {
        return bad;
    }

    const hrp = encoded[0..sep];
    let chk = polymodHrp(hrp);
    i = sep + 1;
    while (i < encoded.len) {
        const idx_opt = charsetIndex(encoded[i]);
        if (idx_opt == null) {
            return bad;
        }
        chk = polymodStep(chk, idx_opt.?);
        i += 1;
    }

    if (chk == BECH32_CONST) {
        return ParsedBech32Meta{ valid: true, variant: Bech32Variant.Bech32, sep: sep };
    }
    if (chk == BECH32M_CONST) {
        return ParsedBech32Meta{ valid: true, variant: Bech32Variant.Bech32m, sep: sep };
    }
    return bad;
}

/// Verifies whether `encoded` is a valid BIP-173 (Bech32) or BIP-350 (Bech32m) string.
pub fn verifyBech32(encoded: string) -> bool {
    return validateStructureAndChecksum(encoded).valid;
}

/// Decodes a Bech32 or Bech32m string, writing lowercase HRP into `hrp_out` and unpacked
/// 8-bit payload bytes into `data_out`. Rejects mixed case, bad checksums, and bad padding.
pub fn decodeBech32(encoded: string, hrp_out: []u8, data_out: []u8) -> Bech32Decoded {
    const meta = validateStructureAndChecksum(encoded);
    if (!meta.valid) {
        return Bech32Decoded.invalid();
    }
    const sep = meta.sep;
    if (hrp_out.len < sep) {
        return Bech32Decoded.invalid();
    }

    * = toLower(encoded[0..sep], hrp_out);

    const data_chars = (encoded.len - sep - 1) - 6;
    if (data_chars > 256) {
        return Bech32Decoded.invalid();
    }
    let words: [256]u8 = undefined;
    let i: usize = 0;
    while (i < data_chars) {
        words[i] = charsetIndex(encoded[sep + 1 + i]).?;
        i += 1;
    }

    const byte_len_opt = from5BitWords(words[0..data_chars], data_out);
    if (byte_len_opt == null) {
        return Bech32Decoded.invalid();
    }

    return Bech32Decoded{
        valid: true,
        variant: meta.variant,
        hrp: hrp_out[0..sep],
        data: data_out[0..byte_len_opt.?],
    };
}
