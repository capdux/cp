// @cap/base32 — Zero-allocation RFC 4648 / Extended Hex / Crockford / z-base-32 Encoder & Decoder
// Clean-room pure-CP implementation strictly adhering to RFC 4648 §6/§7, Crockford's Base32, and z-base-32 (MIT License).

/// RFC 4648 §6 Standard Base32 alphabet (`A-Z`, `2-7`).
pub const ALPHABET_RFC4648: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

/// RFC 4648 §7 Extended Hex Base32 alphabet (`0-9`, `A-V`).
pub const ALPHABET_RFC4648_HEX: []const u8 = "0123456789ABCDEFGHIJKLMNOPQRSTUV";

/// Douglas Crockford's Base32 alphabet (`0-9`, `A-Z` excluding `I`, `L`, `O`, `U`).
pub const ALPHABET_CROCKFORD: []const u8 = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

/// Zooko's human-oriented z-base-32 alphabet.
pub const ALPHABET_ZBASE32: []const u8 = "ybndrfg8ejkmcpqxot1uwisza345h769";

pub err Base32Error {
    InvalidCharacter,
    InvalidLength,
    InvalidPadding,
    BufferTooSmall,
}

pub enum Base32Alphabet {
    Rfc4648,
    Rfc4648Hex,
    Crockford,
    ZBase32,
}

fn alphabetTable(alphabet: Base32Alphabet): []const u8 {
    match (alphabet) {
        Base32Alphabet.Rfc4648 => return ALPHABET_RFC4648,
        Base32Alphabet.Rfc4648Hex => return ALPHABET_RFC4648_HEX,
        Base32Alphabet.Crockford => return ALPHABET_CROCKFORD,
        Base32Alphabet.ZBase32 => return ALPHABET_ZBASE32,
    }
}

fn decodeCharRfc4648(ch: u8): u8? {
    if (ch >= 'A' && ch <= 'Z') return ch - 'A';
    if (ch >= 'a' && ch <= 'z') return ch - 'a';
    if (ch >= '2' && ch <= '7') return (ch - '2') + 26;
    return null;
}

fn decodeCharRfc4648Hex(ch: u8): u8? {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'V') return (ch - 'A') + 10;
    if (ch >= 'a' && ch <= 'v') return (ch - 'a') + 10;
    return null;
}

fn decodeCharCrockford(ch: u8): u8? {
    if (ch >= '0' && ch <= '9') return ch - '0';
    let upper = ch;
    if (upper >= 'a' && upper <= 'z') {
        upper = upper - ('a' - 'A');
    }
    if (upper == 'O') return 0;
    if (upper == 'I' || upper == 'L') return 1;
    if (upper >= 'A' && upper <= 'H') return (upper - 'A') + 10;
    if (upper >= 'J' && upper <= 'K') return (upper - 'J') + 18;
    if (upper >= 'M' && upper <= 'N') return (upper - 'M') + 20;
    if (upper >= 'P' && upper <= 'T') return (upper - 'P') + 22;
    if (upper >= 'V' && upper <= 'Z') return (upper - 'V') + 27;
    return null;
}

fn decodeCharZBase32(ch: u8): u8? {
    let lower = ch;
    if (lower >= 'A' && lower <= 'Z') {
        lower = lower + ('a' - 'A');
    }
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        if (ALPHABET_ZBASE32[i] == lower) return i as u8;
    }
    return null;
}

fn decodeChar(alphabet: Base32Alphabet, ch: u8): u8? {
    match (alphabet) {
        Base32Alphabet.Rfc4648 => return decodeCharRfc4648(ch),
        Base32Alphabet.Rfc4648Hex => return decodeCharRfc4648Hex(ch),
        Base32Alphabet.Crockford => return decodeCharCrockford(ch),
        Base32Alphabet.ZBase32 => return decodeCharZBase32(ch),
    }
}

/// Returns the exact Base32 encoded length in bytes for an input of `input_len` bytes.
pub fn encodedLen(input_len: usize, padded: bool): usize {
    if (input_len == 0) return 0;
    const full_blocks = input_len / 5;
    const rem = input_len % 5;
    if (padded) {
        if (rem == 0) return full_blocks * 8;
        return (full_blocks + 1) * 8;
    }
    let extra: usize = 0;
    if (rem > 0) {
        extra = (rem * 8 + 4) / 5;
    }
    return full_blocks * 8 + extra;
}

/// Returns the maximum possible decoded byte length for a Base32 string of `input_len` bytes.
pub fn decodedMaxLen(input_len: usize): usize {
    const full_blocks = input_len / 8;
    const rem = input_len % 8;
    return full_blocks * 5 + (rem * 5) / 8;
}

/// Encodes `input` bytes into Base32 using the chosen `alphabet` and `padded` mode into `out`.
pub fn encode(input: []const u8, alphabet: Base32Alphabet, padded: bool, out: []u8): ([]const u8) ! Base32Error {
    const needed = encodedLen(input.len, padded);
    if (out.len < needed) return Base32Error.BufferTooSmall;
    if (input.len == 0) return out[0..0];

    const table = alphabetTable(alphabet);
    let r: usize = 0;
    let w: usize = 0;

    while (r + 5 <= input.len) {
        const b0 = input[r];
        const b1 = input[r + 1];
        const b2 = input[r + 2];
        const b3 = input[r + 3];
        const b4 = input[r + 4];

        out[w] = table[(b0 >> 3) as usize];
        out[w + 1] = table[(((b0 & 0x07) << 2) | (b1 >> 6)) as usize];
        out[w + 2] = table[((b1 >> 1) & 0x1F) as usize];
        out[w + 3] = table[(((b1 & 0x01) << 4) | (b2 >> 4)) as usize];
        out[w + 4] = table[(((b2 & 0x0F) << 1) | (b3 >> 7)) as usize];
        out[w + 5] = table[((b3 >> 2) & 0x1F) as usize];
        out[w + 6] = table[(((b3 & 0x03) << 3) | (b4 >> 5)) as usize];
        out[w + 7] = table[(b4 & 0x1F) as usize];

        r += 5;
        w += 8;
    }

    const rem = input.len - r;
    if (rem == 1) {
        const b0 = input[r];
        out[w] = table[(b0 >> 3) as usize];
        out[w + 1] = table[((b0 & 0x07) << 2) as usize];
        w += 2;
        if (padded) {
            out[w] = '=';
            out[w + 1] = '=';
            out[w + 2] = '=';
            out[w + 3] = '=';
            out[w + 4] = '=';
            out[w + 5] = '=';
            w += 6;
        }
    } else if (rem == 2) {
        const b0 = input[r];
        const b1 = input[r + 1];
        out[w] = table[(b0 >> 3) as usize];
        out[w + 1] = table[(((b0 & 0x07) << 2) | (b1 >> 6)) as usize];
        out[w + 2] = table[((b1 >> 1) & 0x1F) as usize];
        out[w + 3] = table[((b1 & 0x01) << 4) as usize];
        w += 4;
        if (padded) {
            out[w] = '=';
            out[w + 1] = '=';
            out[w + 2] = '=';
            out[w + 3] = '=';
            w += 4;
        }
    } else if (rem == 3) {
        const b0 = input[r];
        const b1 = input[r + 1];
        const b2 = input[r + 2];
        out[w] = table[(b0 >> 3) as usize];
        out[w + 1] = table[(((b0 & 0x07) << 2) | (b1 >> 6)) as usize];
        out[w + 2] = table[((b1 >> 1) & 0x1F) as usize];
        out[w + 3] = table[(((b1 & 0x01) << 4) | (b2 >> 4)) as usize];
        out[w + 4] = table[((b2 & 0x0F) << 1) as usize];
        w += 5;
        if (padded) {
            out[w] = '=';
            out[w + 1] = '=';
            out[w + 2] = '=';
            w += 3;
        }
    } else if (rem == 4) {
        const b0 = input[r];
        const b1 = input[r + 1];
        const b2 = input[r + 2];
        const b3 = input[r + 3];
        out[w] = table[(b0 >> 3) as usize];
        out[w + 1] = table[(((b0 & 0x07) << 2) | (b1 >> 6)) as usize];
        out[w + 2] = table[((b1 >> 1) & 0x1F) as usize];
        out[w + 3] = table[(((b1 & 0x01) << 4) | (b2 >> 4)) as usize];
        out[w + 4] = table[(((b2 & 0x0F) << 1) | (b3 >> 7)) as usize];
        out[w + 5] = table[((b3 >> 2) & 0x1F) as usize];
        out[w + 6] = table[((b3 & 0x03) << 3) as usize];
        w += 7;
        if (padded) {
            out[w] = '=';
            w += 1;
        }
    }

    return out[0..w];
}

/// Decodes a Base32 string `input` into `out` using the chosen `alphabet`.
/// Case-insensitive for all alphabets; Crockford tolerates `I/L->1`, `O->0`, and `-` hyphens.
/// Accepts both padded and unpadded inputs while strictly rejecting invalid characters,
/// malformed padding, invalid lengths, and non-zero canonical trailing bits.
pub fn decode(input: []const u8, alphabet: Base32Alphabet, out: []u8): ([]const u8) ! Base32Error {
    if (input.len == 0) return out[0..0];

    const is_crockford = alphabet == Base32Alphabet.Crockford;
    let block: [8]u8 = [0; 8];
    let block_len: usize = 0;
    let pad_count: usize = 0;
    let w: usize = 0;

    let i: usize = 0;
    while (i < input.len) : (i += 1) {
        const ch = input[i];
        if (ch == '=') {
            pad_count += 1;
            continue;
        }
        if (pad_count > 0) {
            return Base32Error.InvalidPadding;
        }
        if (is_crockford && ch == '-') {
            continue;
        }

        const val = decodeChar(alphabet, ch) ?? return Base32Error.InvalidCharacter;
        block[block_len] = val;
        block_len += 1;

        if (block_len == 8) {
            if (w + 5 > out.len) return Base32Error.BufferTooSmall;
            out[w] = (block[0] << 3) | (block[1] >> 2);
            out[w + 1] = ((block[1] & 0x03) << 6) | (block[2] << 1) | (block[3] >> 4);
            out[w + 2] = ((block[3] & 0x0F) << 4) | (block[4] >> 1);
            out[w + 3] = ((block[4] & 0x01) << 7) | (block[5] << 2) | (block[6] >> 3);
            out[w + 4] = ((block[6] & 0x07) << 5) | block[7];
            w += 5;
            block_len = 0;
        }
    }

    if (block_len == 0) {
        if (pad_count > 0) return Base32Error.InvalidPadding;
        return out[0..w];
    }

    if (block_len == 1 || block_len == 3 || block_len == 6) {
        return Base32Error.InvalidLength;
    }

    if (pad_count > 0) {
        const expected_pad = 8 - block_len;
        if (pad_count != expected_pad) return Base32Error.InvalidPadding;
    }

    if (block_len == 2) {
        if ((block[1] & 0x03) != 0) return Base32Error.InvalidPadding;
        if (w + 1 > out.len) return Base32Error.BufferTooSmall;
        out[w] = (block[0] << 3) | (block[1] >> 2);
        w += 1;
    } else if (block_len == 4) {
        if ((block[3] & 0x0F) != 0) return Base32Error.InvalidPadding;
        if (w + 2 > out.len) return Base32Error.BufferTooSmall;
        out[w] = (block[0] << 3) | (block[1] >> 2);
        out[w + 1] = ((block[1] & 0x03) << 6) | (block[2] << 1) | (block[3] >> 4);
        w += 2;
    } else if (block_len == 5) {
        if ((block[4] & 0x01) != 0) return Base32Error.InvalidPadding;
        if (w + 3 > out.len) return Base32Error.BufferTooSmall;
        out[w] = (block[0] << 3) | (block[1] >> 2);
        out[w + 1] = ((block[1] & 0x03) << 6) | (block[2] << 1) | (block[3] >> 4);
        out[w + 2] = ((block[3] & 0x0F) << 4) | (block[4] >> 1);
        w += 3;
    } else if (block_len == 7) {
        if ((block[6] & 0x07) != 0) return Base32Error.InvalidPadding;
        if (w + 4 > out.len) return Base32Error.BufferTooSmall;
        out[w] = (block[0] << 3) | (block[1] >> 2);
        out[w + 1] = ((block[1] & 0x03) << 6) | (block[2] << 1) | (block[3] >> 4);
        out[w + 2] = ((block[3] & 0x0F) << 4) | (block[4] >> 1);
        out[w + 3] = ((block[4] & 0x01) << 7) | (block[5] << 2) | (block[6] >> 3);
        w += 4;
    }

    return out[0..w];
}

/// Encodes `input` using RFC 4648 §6 Standard Base32 with `=` padding.
pub fn encodeStd(input: []const u8, out: []u8): ([]const u8) ! Base32Error {
    return encode(input, Base32Alphabet.Rfc4648, true, out);
}

/// Decodes an RFC 4648 §6 Standard Base32 string (padded or unpadded, case-insensitive).
pub fn decodeStd(input: []const u8, out: []u8): ([]const u8) ! Base32Error {
    return decode(input, Base32Alphabet.Rfc4648, out);
}

/// Encodes `input` using RFC 4648 §6 Standard Base32 without `=` padding (e.g. TOTP / otpauth).
pub fn encodeStdNoPad(input: []const u8, out: []u8): ([]const u8) ! Base32Error {
    return encode(input, Base32Alphabet.Rfc4648, false, out);
}

/// Encodes `input` using RFC 4648 §7 Extended Hex Base32 with `=` padding (e.g. DNSSEC NSEC3).
pub fn encodeHex(input: []const u8, out: []u8): ([]const u8) ! Base32Error {
    return encode(input, Base32Alphabet.Rfc4648Hex, true, out);
}

/// Decodes an RFC 4648 §7 Extended Hex Base32 string (padded or unpadded, case-insensitive).
pub fn decodeHex(input: []const u8, out: []u8): ([]const u8) ! Base32Error {
    return decode(input, Base32Alphabet.Rfc4648Hex, out);
}

/// Encodes `input` using Douglas Crockford's Base32 without `=` padding.
pub fn encodeCrockford(input: []const u8, out: []u8): ([]const u8) ! Base32Error {
    return encode(input, Base32Alphabet.Crockford, false, out);
}

/// Decodes a Crockford Base32 string (case-insensitive, tolerates `I/L->1`, `O->0`, and `-`).
pub fn decodeCrockford(input: []const u8, out: []u8): ([]const u8) ! Base32Error {
    return decode(input, Base32Alphabet.Crockford, out);
}
