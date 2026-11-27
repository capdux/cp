// @cap/bs58 — Zero-allocation Base58 multi-alphabet codec for CP
// Clean-room MIT implementation inspired by Rust `bs58` and npm `bs58`.
//
// Features:
// - 3 standard alphabets: `Bitcoin`, `Ripple`, and `Flickr`.
// - Exact leading-zero byte preservation (`\x00` <-> `alphabet[0]`).
// - Strict character validation rejecting ambiguous chars (`0`, `O`, `I`, `l` in Bitcoin).
// - Zero heap allocation with fixed stack buffers (supports raw inputs up to 256 bytes).

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

fn alphabetChars(alpha: Base58Alphabet) -> string {
    if (alpha == Base58Alphabet.Ripple) {
        return ALPHABET_RIPPLE;
    }
    if (alpha == Base58Alphabet.Flickr) {
        return ALPHABET_FLICKR;
    }
    return ALPHABET_BITCOIN;
}

fn charIndex(alphabet: string, c: u8) -> i8 {
    let i: usize = 0;
    while (i < alphabet.len) {
        if (alphabet[i] == c) {
            return i as i8;
        }
        i += 1;
    }
    return -1;
}

/// Checks whether every character in `encoded` belongs to the specified Base58 alphabet.
pub fn isValidBase58(encoded: string, alpha: Base58Alphabet) -> bool {
    const alphabet = alphabetChars(alpha);
    let i: usize = 0;
    while (i < encoded.len) {
        if (charIndex(alphabet, encoded[i]) < 0) {
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

    // Count leading 0x00 bytes
    let zeros: usize = 0;
    while (zeros < input.len && input[zeros] == 0) {
        zeros += 1;
    }

    let b58: [MAX_ENC_LEN]u8 = undefined;
    let init_i: usize = 0;
    while (init_i < MAX_ENC_LEN) {
        b58[init_i] = 0;
        init_i += 1;
    }

    let length: usize = 0;
    let i = zeros;
    while (i < input.len) {
        let carry: u32 = input[i] as u32;
        let k: usize = 0;
        while (k < length || carry > 0) {
            if (k >= MAX_ENC_LEN) {
                return null;
            }
            carry += (b58[k] as u32) * 256;
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
    let z: usize = 0;
    while (z < zeros) {
        out[pos] = alphabet[0];
        pos += 1;
        z += 1;
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

    let zeros: usize = 0;
    while (zeros < encoded.len && encoded[zeros] == zero_char) {
        zeros += 1;
    }

    let b256: [MAX_RAW_LEN]u8 = undefined;
    let init_i: usize = 0;
    while (init_i < MAX_RAW_LEN) {
        b256[init_i] = 0;
        init_i += 1;
    }

    let length: usize = 0;
    let i = zeros;
    while (i < encoded.len) {
        const digit = charIndex(alphabet, encoded[i]);
        if (digit < 0) {
            return null;
        }
        let carry: u32 = digit as u32;
        let k: usize = 0;
        while (k < length || carry > 0) {
            if (k >= MAX_RAW_LEN) {
                return null;
            }
            carry += (b256[k] as u32) * 58;
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
    let z: usize = 0;
    while (z < zeros) {
        out[pos] = 0;
        pos += 1;
        z += 1;
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
