// @cap/bs58 — Zero-allocation Base58 multi-alphabet codec for CP
// Clean-room MIT implementation inspired by Rust `bs58` and npm `bs58`.
// Accelerated with comptime 256-entry decode LUTs, 16-lane @Vector(16, u8) SIMD
// leading-zero scanning, and zero-clear uninitialized stack workspaces.
//
// Features:
// - 3 standard alphabets: `Bitcoin`, `Ripple`, and `Flickr`.
// - Exact leading-zero byte preservation (`\x00` <-> `alphabet[0]`).
// - Strict character validation rejecting ambiguous chars (`0`, `O`, `I`, `l` in Bitcoin).
// - Zero heap allocation with fixed stack buffers (supports raw inputs up to 256 bytes).

type B58Vec16 = @Vector(16, u8);

const MAX_RAW_LEN: usize = 256;
const MAX_ENC_LEN: usize = 384;

const ALPHABET_BITCOIN: string = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
const ALPHABET_RIPPLE: string = "rpshnaf39wBUDNEGHJKLM4PQRST7VWXYZ2bcdeCg65jkm8oFqi1tuvAxyz";
const ALPHABET_FLICKR: string = "123456789abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ";

pub enum Base58Alphabet: u8 {
    Bitcoin,
    Ripple,
    Flickr,
}

fn b58BuildDecLut(alphabet: string) -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [255; 256];
    let i: usize = 0;
    while (i < alphabet.len) {
        lut[alphabet[i] as usize] = i as u8;
        i += 1;
    }
    return lut;
}

const B58_LUT_BITCOIN: [256]u8 = b58BuildDecLut(ALPHABET_BITCOIN);
const B58_LUT_RIPPLE: [256]u8 = b58BuildDecLut(ALPHABET_RIPPLE);
const B58_LUT_FLICKR: [256]u8 = b58BuildDecLut(ALPHABET_FLICKR);

fn alphabetChars(alpha: Base58Alphabet) -> string {
    if (alpha == Base58Alphabet.Ripple) {
        return ALPHABET_RIPPLE;
    }
    if (alpha == Base58Alphabet.Flickr) {
        return ALPHABET_FLICKR;
    }
    return ALPHABET_BITCOIN;
}

fn decodeLutVal(alpha: Base58Alphabet, c: u8) -> u8 {
    if (alpha == Base58Alphabet.Ripple) {
        return B58_LUT_RIPPLE[c as usize];
    }
    if (alpha == Base58Alphabet.Flickr) {
        return B58_LUT_FLICKR[c as usize];
    }
    return B58_LUT_BITCOIN[c as usize];
}

fn b58CountLeading(s: string, target: u8) -> usize {
    let i: usize = 0;
    const len = s.len;
    const vt: B58Vec16 = @splat(target);
    while (i + 16 <= len) {
        const v: B58Vec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v != vt)) {
            let k: usize = 0;
            while (k < 16 && s[i + k] == target) {
                k += 1;
            }
            return i + k;
        }
        i += 16;
    }
    while (i < len && s[i] == target) {
        i += 1;
    }
    return i;
}

/// Checks whether every character in `encoded` belongs to the specified Base58 alphabet.
pub fn isValidBase58(encoded: string, alpha: Base58Alphabet) -> bool {
    let i: usize = 0;
    const len = encoded.len;
    if (alpha == Base58Alphabet.Ripple) {
        while (i + 4 <= len) {
            if (((B58_LUT_RIPPLE[encoded[i] as usize] |
                B58_LUT_RIPPLE[encoded[i + 1] as usize] |
                B58_LUT_RIPPLE[encoded[i + 2] as usize] |
                B58_LUT_RIPPLE[encoded[i + 3] as usize]) & 0x80) != 0)
            {
                return false;
            }
            i += 4;
        }
        while (i < len) {
            if ((B58_LUT_RIPPLE[encoded[i] as usize] & 0x80) != 0) {
                return false;
            }
            i += 1;
        }
        return true;
    }
    if (alpha == Base58Alphabet.Flickr) {
        while (i + 4 <= len) {
            if (((B58_LUT_FLICKR[encoded[i] as usize] |
                B58_LUT_FLICKR[encoded[i + 1] as usize] |
                B58_LUT_FLICKR[encoded[i + 2] as usize] |
                B58_LUT_FLICKR[encoded[i + 3] as usize]) & 0x80) != 0)
            {
                return false;
            }
            i += 4;
        }
        while (i < len) {
            if ((B58_LUT_FLICKR[encoded[i] as usize] & 0x80) != 0) {
                return false;
            }
            i += 1;
        }
        return true;
    }
    while (i + 4 <= len) {
        if (((B58_LUT_BITCOIN[encoded[i] as usize] |
            B58_LUT_BITCOIN[encoded[i + 1] as usize] |
            B58_LUT_BITCOIN[encoded[i + 2] as usize] |
            B58_LUT_BITCOIN[encoded[i + 3] as usize]) & 0x80) != 0)
        {
            return false;
        }
        i += 4;
    }
    while (i < len) {
        if ((B58_LUT_BITCOIN[encoded[i] as usize] & 0x80) != 0) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Encodes `input` bytes using the chosen `Base58Alphabet` into caller-provided `out`.
/// Returns `null` if `input.len > 256` or `out` is too small.
pub fn encodeBase58With(input: string, alpha: Base58Alphabet, out: []u8) -> string? {
    if (input.len == 0) {
        return out[0..0];
    }
    if (input.len > MAX_RAW_LEN) {
        return null;
    }

    const alphabet = alphabetChars(alpha);
    const zeros = b58CountLeading(input, 0);

    let b58: [MAX_ENC_LEN]u8 = undefined;
    let length: usize = 0;
    let i = zeros;
    while (i < input.len) {
        let carry: u32 = input[i] as u32;
        let k: usize = 0;
        while (k < length) {
            carry += (b58[k] as u32) << 8;
            b58[k] = (carry % 58) as u8;
            carry = carry / 58;
            k += 1;
        }
        while (carry > 0) {
            if (k >= MAX_ENC_LEN) {
                return null;
            }
            b58[k] = (carry % 58) as u8;
            carry = carry / 58;
            k += 1;
        }
        length = k;
        i += 1;
    }

    const total = zeros + length;
    if (total > out.len) {
        return null;
    }

    let pos: usize = 0;
    const zchar = alphabet[0];
    while (pos < zeros) {
        out[pos] = zchar;
        pos += 1;
    }

    let r = length;
    while (r > 0) {
        r -= 1;
        out[pos] = alphabet[b58[r] as usize];
        pos += 1;
    }

    return out[0..total];
}

/// Decodes a Base58 string using the chosen `Base58Alphabet` into caller-provided `out`.
/// Returns `null` on invalid characters, input > 384 bytes, or insufficient `out` capacity.
pub fn decodeBase58With(encoded: string, alpha: Base58Alphabet, out: []u8) -> string? {
    if (encoded.len == 0) {
        return out[0..0];
    }
    if (encoded.len > MAX_ENC_LEN) {
        return null;
    }

    const alphabet = alphabetChars(alpha);
    const zero_char = alphabet[0];
    const zeros = b58CountLeading(encoded, zero_char);

    let b256: [MAX_RAW_LEN]u8 = undefined;
    let length: usize = 0;
    let i = zeros;
    while (i < encoded.len) {
        const digit = decodeLutVal(alpha, encoded[i]);
        if (digit >= 58) {
            return null;
        }
        let carry: u32 = digit as u32;
        let k: usize = 0;
        while (k < length) {
            carry += (b256[k] as u32) * 58;
            b256[k] = (carry & 0xFF) as u8;
            carry = carry >> 8;
            k += 1;
        }
        while (carry > 0) {
            if (k >= MAX_RAW_LEN) {
                return null;
            }
            b256[k] = (carry & 0xFF) as u8;
            carry = carry >> 8;
            k += 1;
        }
        length = k;
        i += 1;
    }

    const total = zeros + length;
    if (total > out.len) {
        return null;
    }

    let pos: usize = 0;
    while (pos < zeros) {
        out[pos] = 0;
        pos += 1;
    }

    let r = length;
    while (r > 0) {
        r -= 1;
        out[pos] = b256[r];
        pos += 1;
    }

    return out[0..total];
}

/// Encodes `input` using the default Bitcoin Base58 alphabet into `out`.
pub fn encodeBase58(input: string, out: []u8) -> string? {
    return encodeBase58With(input, Base58Alphabet.Bitcoin, out);
}

/// Decodes a Bitcoin Base58 string into `out`.
pub fn decodeBase58(encoded: string, out: []u8) -> string? {
    return decodeBase58With(encoded, Base58Alphabet.Bitcoin, out);
}
