// @cap/data-encoding — Zero-allocation RFC 4648 Base32/Base32hex/Crockford/Base64url codec
// Clean-room MIT implementation inspired by Rust `data-encoding` and RFC 4648.

const BASE32_ALPHA: string = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
const BASE32HEX_ALPHA: string = "0123456789ABCDEFGHIJKLMNOPQRSTUV";
const CROCKFORD_ALPHA: string = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
const BASE64URL_ALPHA: string = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

// ---------------------------------------------------------------------------
// Internal Base32 Core
// ---------------------------------------------------------------------------

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn encodeBase32Core(input: string, alpha: string, pad: bool, out: []u8) -> string {
    if (input.len == 0) {
        return out[0..0];
    }
    let pos: usize = 0;
    let i: usize = 0;

    // Full 5-byte (40-bit) -> 8-char blocks
    while (i + 5 <= input.len) {
        const b0 = input[i] as u32;
        const b1 = input[i + 1] as u32;
        const b2 = input[i + 2] as u32;
        const b3 = input[i + 3] as u32;
        const b4 = input[i + 4] as u32;

        pos = appendByte(out, pos, alpha[(b0 >> 3) as usize]);
        pos = appendByte(out, pos, alpha[(((b0 & 0x07) << 2) | (b1 >> 6)) as usize]);
        pos = appendByte(out, pos, alpha[((b1 >> 1) & 0x1F) as usize]);
        pos = appendByte(out, pos, alpha[(((b1 & 0x01) << 4) | (b2 >> 4)) as usize]);
        pos = appendByte(out, pos, alpha[(((b2 & 0x0F) << 1) | (b3 >> 7)) as usize]);
        pos = appendByte(out, pos, alpha[((b3 >> 2) & 0x1F) as usize]);
        pos = appendByte(out, pos, alpha[(((b3 & 0x03) << 3) | (b4 >> 5)) as usize]);
        pos = appendByte(out, pos, alpha[(b4 & 0x1F) as usize]);
        i += 5;
    }

    const rem = input.len - i;
    if (rem == 1) {
        const b0 = input[i] as u32;
        pos = appendByte(out, pos, alpha[(b0 >> 3) as usize]);
        pos = appendByte(out, pos, alpha[((b0 & 0x07) << 2) as usize]);
        if (pad) {
            let k: usize = 0;
            while (k < 6) { pos = appendByte(out, pos, '='); k += 1; }
        }
    } else if (rem == 2) {
        const b0 = input[i] as u32;
        const b1 = input[i + 1] as u32;
        pos = appendByte(out, pos, alpha[(b0 >> 3) as usize]);
        pos = appendByte(out, pos, alpha[(((b0 & 0x07) << 2) | (b1 >> 6)) as usize]);
        pos = appendByte(out, pos, alpha[((b1 >> 1) & 0x1F) as usize]);
        pos = appendByte(out, pos, alpha[((b1 & 0x01) << 4) as usize]);
        if (pad) {
            let k: usize = 0;
            while (k < 4) { pos = appendByte(out, pos, '='); k += 1; }
        }
    } else if (rem == 3) {
        const b0 = input[i] as u32;
        const b1 = input[i + 1] as u32;
        const b2 = input[i + 2] as u32;
        pos = appendByte(out, pos, alpha[(b0 >> 3) as usize]);
        pos = appendByte(out, pos, alpha[(((b0 & 0x07) << 2) | (b1 >> 6)) as usize]);
        pos = appendByte(out, pos, alpha[((b1 >> 1) & 0x1F) as usize]);
        pos = appendByte(out, pos, alpha[(((b1 & 0x01) << 4) | (b2 >> 4)) as usize]);
        pos = appendByte(out, pos, alpha[((b2 & 0x0F) << 1) as usize]);
        if (pad) {
            let k: usize = 0;
            while (k < 3) { pos = appendByte(out, pos, '='); k += 1; }
        }
    } else if (rem == 4) {
        const b0 = input[i] as u32;
        const b1 = input[i + 1] as u32;
        const b2 = input[i + 2] as u32;
        const b3 = input[i + 3] as u32;
        pos = appendByte(out, pos, alpha[(b0 >> 3) as usize]);
        pos = appendByte(out, pos, alpha[(((b0 & 0x07) << 2) | (b1 >> 6)) as usize]);
        pos = appendByte(out, pos, alpha[((b1 >> 1) & 0x1F) as usize]);
        pos = appendByte(out, pos, alpha[(((b1 & 0x01) << 4) | (b2 >> 4)) as usize]);
        pos = appendByte(out, pos, alpha[(((b2 & 0x0F) << 1) | (b3 >> 7)) as usize]);
        pos = appendByte(out, pos, alpha[((b3 >> 2) & 0x1F) as usize]);
        pos = appendByte(out, pos, alpha[((b3 & 0x03) << 3) as usize]);
        if (pad) {
            pos = appendByte(out, pos, '=');
        }
    }

    return out[0..pos];
}

fn base32Val(c: u8) -> u8? {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= '2' && c <= '7') return (c - '2') + 26;
    return null;
}

fn base32HexVal(c: u8) -> u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'V') return (c - 'A') + 10;
    return null;
}

fn crockfordVal(c: u8) -> u8? {
    if (c >= '0' && c <= '9') return c - '0';
    // Tolerate visually ambiguous characters per Crockford spec
    if (c == 'o' || c == 'O') return 0;
    if (c == 'i' || c == 'I' || c == 'l' || c == 'L') return 1;
    let u = c;
    if (u >= 'a' && u <= 'z') {
        u -= 32;
    }
    if (u >= 'A' && u <= 'H') return (u - 'A') + 10;
    if (u >= 'J' && u <= 'K') return (u - 'J') + 18;
    if (u >= 'M' && u <= 'N') return (u - 'M') + 20;
    if (u >= 'P' && u <= 'T') return (u - 'P') + 22;
    if (u >= 'V' && u <= 'Z') return (u - 'V') + 27;
    return null;
}

fn decodeBase32Standard(encoded: string, is_hex: bool, out: []u8) -> string? {
    if (encoded.len == 0) {
        return out[0..0];
    }

    // Count trailing '=' padding
    let data_len = encoded.len;
    let pad_count: usize = 0;
    while (data_len > 0 && encoded[data_len - 1] == '=') {
        data_len -= 1;
        pad_count += 1;
    }

    if (pad_count > 0) {
        if ((encoded.len % 8) != 0) {
            return null;
        }
        if (pad_count != 1 && pad_count != 3 && pad_count != 4 && pad_count != 6) {
            return null;
        }
    }

    const rem_chars = data_len % 8;
    if (rem_chars == 1 || rem_chars == 3 || rem_chars == 6) {
        return null;
    }

    let pos: usize = 0;
    let bits: u32 = 0;
    let bit_count: u32 = 0;
    let i: usize = 0;
    while (i < data_len) {
        const v_opt = if (is_hex) base32HexVal(encoded[i]) else base32Val(encoded[i]);
        if (v_opt == null) {
            return null;
        }
        bits = (bits << 5) | (v_opt.? as u32);
        bit_count += 5;
        if (bit_count >= 8) {
            bit_count -= 8;
            if (pos >= out.len) {
                return null;
            }
            out[pos] = ((bits >> bit_count) & 0xFF) as u8;
            pos += 1;
        }
        i += 1;
    }

    // Strict canonical check: unused trailing bits must be zero
    if (bit_count > 0) {
        const mask: u32 = ((1 as u32) << bit_count) - 1;
        if ((bits & mask) != 0) {
            return null;
        }
    }

    return out[0..pos];
}

// ---------------------------------------------------------------------------
// 1. RFC 4648 Base32
// ---------------------------------------------------------------------------

/// Encodes `input` using RFC 4648 Base32 (`A-Z2-7`), with optional `'='` padding.
pub fn encodeBase32(input: string, pad: bool, out: []u8) -> string {
    return encodeBase32Core(input, BASE32_ALPHA, pad, out);
}

/// Strictly decodes RFC 4648 Base32 (`A-Z2-7`, padded or unpadded) into `out`.
/// Rejects invalid characters, bad padding, and non-zero trailing bits.
pub fn decodeBase32(encoded: string, out: []u8) -> string? {
    return decodeBase32Standard(encoded, false, out);
}

// ---------------------------------------------------------------------------
// 2. RFC 4648 Base32hex
// ---------------------------------------------------------------------------

/// Encodes `input` using RFC 4648 Base32hex (`0-9A-V`, preserves binary lexicographical order).
pub fn encodeBase32Hex(input: string, pad: bool, out: []u8) -> string {
    return encodeBase32Core(input, BASE32HEX_ALPHA, pad, out);
}

/// Strictly decodes RFC 4648 Base32hex (`0-9A-V`, padded or unpadded) into `out`.
pub fn decodeBase32Hex(encoded: string, out: []u8) -> string? {
    return decodeBase32Standard(encoded, true, out);
}

// ---------------------------------------------------------------------------
// 3. Crockford Base32
// ---------------------------------------------------------------------------

/// Encodes `input` using Crockford Base32 (`0-9A-HJKMNP-TV-Z`, unpadded).
pub fn encodeCrockford(input: string, out: []u8) -> string {
    return encodeBase32Core(input, CROCKFORD_ALPHA, false, out);
}

/// Decodes Crockford Base32 into `out`.
/// Case-insensitive, maps `i/I/l/L -> 1`, `o/O -> 0`, and ignores `'-'` hyphens.
pub fn decodeCrockford(encoded: string, out: []u8) -> string? {
    if (encoded.len == 0) {
        return out[0..0];
    }
    let pos: usize = 0;
    let bits: u32 = 0;
    let bit_count: u32 = 0;
    let sym_count: usize = 0;
    let i: usize = 0;
    while (i < encoded.len) {
        const c = encoded[i];
        if (c == '-') {
            i += 1;
            continue;
        }
        const v_opt = crockfordVal(c);
        if (v_opt == null) {
            return null;
        }
        sym_count += 1;
        bits = (bits << 5) | (v_opt.? as u32);
        bit_count += 5;
        if (bit_count >= 8) {
            bit_count -= 8;
            if (pos >= out.len) {
                return null;
            }
            out[pos] = ((bits >> bit_count) & 0xFF) as u8;
            pos += 1;
        }
        i += 1;
    }

    const rem = sym_count % 8;
    if (rem == 1 || rem == 3 || rem == 6) {
        return null;
    }
    if (bit_count > 0) {
        const mask: u32 = ((1 as u32) << bit_count) - 1;
        if ((bits & mask) != 0) {
            return null;
        }
    }
    return out[0..pos];
}

// ---------------------------------------------------------------------------
// 4. RFC 4648 Base64url (No Padding)
// ---------------------------------------------------------------------------

fn b64UrlVal(c: u8) -> u8? {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return (c - 'a') + 26;
    if (c >= '0' && c <= '9') return (c - '0') + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return null;
}

/// Encodes `input` using RFC 4648 Base64url (`A-Za-z0-9-_`) without `'='` padding.
pub fn encodeBase64UrlNoPad(input: string, out: []u8) -> string {
    if (input.len == 0) {
        return out[0..0];
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i + 3 <= input.len) {
        const b0 = input[i] as u32;
        const b1 = input[i + 1] as u32;
        const b2 = input[i + 2] as u32;
        pos = appendByte(out, pos, BASE64URL_ALPHA[(b0 >> 2) as usize]);
        pos = appendByte(out, pos, BASE64URL_ALPHA[(((b0 & 0x03) << 4) | (b1 >> 4)) as usize]);
        pos = appendByte(out, pos, BASE64URL_ALPHA[(((b1 & 0x0F) << 2) | (b2 >> 6)) as usize]);
        pos = appendByte(out, pos, BASE64URL_ALPHA[(b2 & 0x3F) as usize]);
        i += 3;
    }

    const rem = input.len - i;
    if (rem == 1) {
        const b0 = input[i] as u32;
        pos = appendByte(out, pos, BASE64URL_ALPHA[(b0 >> 2) as usize]);
        pos = appendByte(out, pos, BASE64URL_ALPHA[((b0 & 0x03) << 4) as usize]);
    } else if (rem == 2) {
        const b0 = input[i] as u32;
        const b1 = input[i + 1] as u32;
        pos = appendByte(out, pos, BASE64URL_ALPHA[(b0 >> 2) as usize]);
        pos = appendByte(out, pos, BASE64URL_ALPHA[(((b0 & 0x03) << 4) | (b1 >> 4)) as usize]);
        pos = appendByte(out, pos, BASE64URL_ALPHA[((b1 & 0x0F) << 2) as usize]);
    }

    return out[0..pos];
}

/// Strictly decodes unpadded RFC 4648 Base64url (`A-Za-z0-9-_`) into `out`.
/// Rejects `'='`, `'+'`, `'/'`, invalid lengths (`len % 4 == 1`), and non-zero trailing bits.
pub fn decodeBase64UrlNoPad(encoded: string, out: []u8) -> string? {
    if (encoded.len == 0) {
        return out[0..0];
    }
    const rem = encoded.len % 4;
    if (rem == 1) {
        return null;
    }

    let pos: usize = 0;
    let bits: u32 = 0;
    let bit_count: u32 = 0;
    let i: usize = 0;
    while (i < encoded.len) {
        const v_opt = b64UrlVal(encoded[i]);
        if (v_opt == null) {
            return null;
        }
        bits = (bits << 6) | (v_opt.? as u32);
        bit_count += 6;
        if (bit_count >= 8) {
            bit_count -= 8;
            if (pos >= out.len) {
                return null;
            }
            out[pos] = ((bits >> bit_count) & 0xFF) as u8;
            pos += 1;
        }
        i += 1;
    }

    if (bit_count > 0) {
        const mask: u32 = ((1 as u32) << bit_count) - 1;
        if ((bits & mask) != 0) {
            return null;
        }
    }

    return out[0..pos];
}
