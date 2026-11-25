// @cap/base58 — Zero-allocation Base58 & Base58Check Encoder/Decoder
// Clean-room pure-CP implementation modeled after the public Bitcoin/IPFS/Flickr Base58 & Base58Check specifications (MIT License).

const std = @import("std");

/// Standard Bitcoin / Solana / IPFS CIDv0 Base58 alphabet.
pub const ALPHABET_BITCOIN: []const u8 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

/// Flickr short-URL Base58 alphabet.
pub const ALPHABET_FLICKR: []const u8 = "123456789abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ";

/// Ripple (XRP) Base58 alphabet.
pub const ALPHABET_RIPPLE: []const u8 = "rpshnaf39wBUDNEGHJKLM4PQRST7VWXYZ2bcdeCg65jkm8oFqi1tuvAxyz";

/// Maximum supported input payload bytes for stack-only zero-heap encoding/decoding.
pub const MAX_BASE58_INPUT: usize = 512;

pub err Base58Error {
    InvalidAlphabet,
    InvalidCharacter,
    BufferTooSmall,
    InputTooLarge,
    ChecksumMismatch,
    InvalidLength,
}

fn findInAlphabet(alphabet: []const u8, ch: u8): u8? {
    let i: usize = 0;
    while (i < 58) : (i += 1) {
        if (alphabet[i] == ch) return i as u8;
    }
    return null;
}

/// Encodes `input` bytes into Base58 using the specified 58-byte `alphabet`.
/// Preserves leading `0x00` bytes as leading `alphabet[0]` characters (`'1'` in Bitcoin).
pub fn encodeWithAlphabet(input: []const u8, alphabet: []const u8, out: []u8): ([]const u8) ! Base58Error {
    if (alphabet.len != 58) return Base58Error.InvalidAlphabet;
    if (input.len == 0) return out[0..0];
    if (input.len > MAX_BASE58_INPUT) return Base58Error.InputTooLarge;

    let zeros: usize = 0;
    while (zeros < input.len && input[zeros] == 0) : (zeros += 1) {}

    // Upper bound: ceil(len * log(256) / log(58)) <= len * 138 / 100 + 1
    let digits: [768]u8 = [0; 768];
    let digits_len: usize = 0;

    let i: usize = zeros;
    while (i < input.len) : (i += 1) {
        let carry: u32 = input[i] as u32;
        let j: usize = 0;
        while (j < digits_len) : (j += 1) {
            carry += (digits[j] as u32) * 256;
            digits[j] = (carry % 58) as u8;
            carry /= 58;
        }
        while (carry > 0) {
            if (digits_len >= 768) return Base58Error.InputTooLarge;
            digits[digits_len] = (carry % 58) as u8;
            digits_len += 1;
            carry /= 58;
        }
    }

    const total = zeros + digits_len;
    if (out.len < total) return Base58Error.BufferTooSmall;

    let w: usize = 0;
    while (w < zeros) : (w += 1) {
        out[w] = alphabet[0];
    }
    let k: usize = digits_len;
    while (k > 0) {
        k -= 1;
        out[w] = alphabet[digits[k] as usize];
        w += 1;
    }
    return out[0..total];
}

/// Decodes a Base58 string `input` using the specified 58-byte `alphabet`.
pub fn decodeWithAlphabet(input: []const u8, alphabet: []const u8, out: []u8): ([]const u8) ! Base58Error {
    if (alphabet.len != 58) return Base58Error.InvalidAlphabet;
    if (input.len == 0) return out[0..0];
    if (input.len > 768) return Base58Error.InputTooLarge;

    let zeros: usize = 0;
    while (zeros < input.len && input[zeros] == alphabet[0]) : (zeros += 1) {}

    let bytes: [560]u8 = [0; 560];
    let bytes_len: usize = 0;

    let i: usize = zeros;
    while (i < input.len) : (i += 1) {
        const val = findInAlphabet(alphabet, input[i]) ?? return Base58Error.InvalidCharacter;
        let carry: u32 = val as u32;
        let j: usize = 0;
        while (j < bytes_len) : (j += 1) {
            carry += (bytes[j] as u32) * 58;
            bytes[j] = (carry & 0xFF) as u8;
            carry >>= 8;
        }
        while (carry > 0) {
            if (bytes_len >= 560) return Base58Error.InputTooLarge;
            bytes[bytes_len] = (carry & 0xFF) as u8;
            bytes_len += 1;
            carry >>= 8;
        }
    }

    const total = zeros + bytes_len;
    if (out.len < total) return Base58Error.BufferTooSmall;

    let w: usize = 0;
    while (w < zeros) : (w += 1) {
        out[w] = 0;
    }
    let k: usize = bytes_len;
    while (k > 0) {
        k -= 1;
        out[w] = bytes[k];
        w += 1;
    }
    return out[0..total];
}

/// Encodes `input` using the standard Bitcoin / Solana / IPFS Base58 alphabet.
pub fn encode(input: []const u8, out: []u8): ([]const u8) ! Base58Error {
    return encodeWithAlphabet(input, ALPHABET_BITCOIN, out);
}

/// Decodes `input` using the standard Bitcoin / Solana / IPFS Base58 alphabet.
pub fn decode(input: []const u8, out: []u8): ([]const u8) ! Base58Error {
    return decodeWithAlphabet(input, ALPHABET_BITCOIN, out);
}

fn doubleSha256(data: []const u8): [32]u8 {
    let first: [32]u8 = [0; 32];
    let h1 = std.crypto.hash.sha2.Sha256.init(.{});
    h1.update(data);
    h1.final(&first);

    let second: [32]u8 = [0; 32];
    let h2 = std.crypto.hash.sha2.Sha256.init(.{});
    h2.update(first[0..32]);
    h2.final(&second);
    return second;
}

/// Encodes `payload` with a 4-byte double-SHA-256 checksum suffix (Base58Check, used by Bitcoin addresses & WIF).
pub fn encodeCheck(payload: []const u8, out: []u8): ([]const u8) ! Base58Error {
    if (payload.len + 4 > MAX_BASE58_INPUT) return Base58Error.InputTooLarge;
    let buf: [MAX_BASE58_INPUT]u8 = [0; MAX_BASE58_INPUT];
    @memcpy(buf[0..payload.len], payload);
    const digest = doubleSha256(payload);
    buf[payload.len] = digest[0];
    buf[payload.len + 1] = digest[1];
    buf[payload.len + 2] = digest[2];
    buf[payload.len + 3] = digest[3];
    return encode(buf[0 .. payload.len + 4], out);
}

/// Decodes a Base58Check string, verifies its 4-byte double-SHA-256 checksum in constant time,
/// and returns the payload without the checksum suffix.
pub fn decodeCheck(input: []const u8, out: []u8): ([]const u8) ! Base58Error {
    let tmp: [MAX_BASE58_INPUT]u8 = [0; MAX_BASE58_INPUT];
    const decoded = decode(input, tmp[0..])?;
    if (decoded.len < 4) return Base58Error.InvalidLength;

    const payload_len = decoded.len - 4;
    const expected = doubleSha256(decoded[0..payload_len]);
    let diff: u8 = 0;
    diff |= decoded[payload_len] ^ expected[0];
    diff |= decoded[payload_len + 1] ^ expected[1];
    diff |= decoded[payload_len + 2] ^ expected[2];
    diff |= decoded[payload_len + 3] ^ expected[3];
    if (diff != 0) return Base58Error.ChecksumMismatch;

    if (out.len < payload_len) return Base58Error.BufferTooSmall;
    @memcpy(out[0..payload_len], decoded[0..payload_len]);
    return out[0..payload_len];
}
