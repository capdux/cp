// @cap/nanoid — Cryptographically secure, URL-friendly, zero-allocation unique ID generator
// Clean-room pure-CP implementation modeled after the NanoID algorithm specification (MIT License).

const std = @import("std");

/// Default NanoID length (21 characters = ~126 bits of entropy with 64-char URL-safe alphabet).
pub const DEFAULT_SIZE: usize = 21;

/// Default URL-safe 64-character alphabet (`A-Za-z0-9_-`), matching the NanoID specification.
pub const URL_ALPHABET: []const u8 = "useandom-26T198340PX75pxJACKVERYMINDBUSHWOLF_GQZbfghjklqvwyzrict";

/// Common preset alphabets for convenience.
pub const ALPHABET_ALPHANUMERIC: []const u8 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
pub const ALPHABET_LOWER_ALPHANUMERIC: []const u8 = "0123456789abcdefghijklmnopqrstuvwxyz";
pub const ALPHABET_NUMBERS: []const u8 = "0123456789";
pub const ALPHABET_HEX_LOWER: []const u8 = "0123456789abcdef";
pub const ALPHABET_NOLOOKEALIKE: []const u8 = "346789ABCDEFGHJKLMNPQRTUVWXYabcdefghijkmnpqrtwxyz";

pub err NanoidError {
    InvalidAlphabet,
    InvalidSize,
    BufferTooSmall,
    EntropyExhausted,
}

fn fillCsprng(buf: []u8) {
    * = std.os.linux.getrandom(buf.ptr, buf.len, 0);
}

/// Computes the smallest bitmask `(2^k - 1) >= (alphabet_len - 1)` for unbiased rejection sampling.
/// Guarantees every symbol in `alphabet` has strictly equal probability (zero modulo bias).
pub fn calcBitmask(alphabet_len: usize): u8 {
    if (alphabet_len <= 1) return 0;
    let m: usize = alphabet_len - 1;
    m |= m >> 1;
    m |= m >> 2;
    m |= m >> 4;
    return (m & 0xFF) as u8;
}

/// Computes the entropy in bits for an ID of `size` symbols drawn uniformly from an alphabet of `alphabet_len`.
pub fn entropyBits(alphabet_len: usize, size: usize): f64 {
    if (alphabet_len <= 1 || size == 0) return 0.0;
    return (@as(f64, @floatFromInt(size))) * std.math.log2(@as(f64, @floatFromInt(alphabet_len)));
}

/// Generates a NanoID using a caller-supplied random byte pool (deterministic, ideal for unit tests & reproducible builds).
/// Uses unbiased bitmask rejection sampling. Returns `EntropyExhausted` if `random_bytes` is depleted before filling `size`.
pub fn generateFromBytes(
    alphabet: []const u8,
    size: usize,
    random_bytes: []const u8,
    out: []u8,
): ([]const u8) ! NanoidError {
    if (alphabet.len < 2 || alphabet.len > 256) return NanoidError.InvalidAlphabet;
    if (size == 0) return NanoidError.InvalidSize;
    if (out.len < size) return NanoidError.BufferTooSmall;

    const mask = calcBitmask(alphabet.len);
    let written: usize = 0;
    let rpos: usize = 0;

    while (written < size) {
        if (rpos >= random_bytes.len) return NanoidError.EntropyExhausted;
        const idx: usize = (random_bytes[rpos] & mask) as usize;
        rpos += 1;
        if (idx < alphabet.len) {
            out[written] = alphabet[idx];
            written += 1;
        }
    }
    return out[0..size];
}

/// Generates a cryptographically secure NanoID of `size` characters using a custom `alphabet` (2..=256 bytes).
/// Uses unbiased bitmask rejection sampling backed by OS CSPRNG (`getrandom`).
pub fn nanoidCustom(alphabet: []const u8, size: usize, out: []u8): ([]const u8) ! NanoidError {
    if (alphabet.len < 2 || alphabet.len > 256) return NanoidError.InvalidAlphabet;
    if (size == 0) return NanoidError.InvalidSize;
    if (out.len < size) return NanoidError.BufferTooSmall;

    const mask = calcBitmask(alphabet.len);
    let rand_buf: [128]u8 = [0; 128];
    let written: usize = 0;

    while (written < size) {
        fillCsprng(rand_buf[0..128]);
        let i: usize = 0;
        while (i < 128 && written < size) : (i += 1) {
            const idx: usize = (rand_buf[i] & mask) as usize;
            if (idx < alphabet.len) {
                out[written] = alphabet[idx];
                written += 1;
            }
        }
    }
    return out[0..size];
}

/// Generates a cryptographically secure URL-friendly NanoID of custom length `size` into `out`.
/// Fast-path for the 64-char default alphabet (mask `0x3F`, zero rejections).
pub fn nanoidLen(size: usize, out: []u8): ([]const u8) ! NanoidError {
    if (size == 0) return NanoidError.InvalidSize;
    if (out.len < size) return NanoidError.BufferTooSmall;

    let rand_buf: [128]u8 = [0; 128];
    let written: usize = 0;

    while (written < size) {
        const chunk = if (size - written < 128) (size - written) else 128;
        fillCsprng(rand_buf[0..chunk]);
        let i: usize = 0;
        while (i < chunk) : (i += 1) {
            out[written] = URL_ALPHABET[(rand_buf[i] & 63) as usize];
            written += 1;
        }
    }
    return out[0..size];
}

/// Generates a standard 21-character URL-friendly NanoID directly on the stack (zero heap allocation).
pub fn nanoid(): [21]u8 {
    let out: [21]u8 = [0; 21];
    let rand_buf: [21]u8 = [0; 21];
    fillCsprng(rand_buf[0..21]);
    let i: usize = 0;
    while (i < 21) : (i += 1) {
        out[i] = URL_ALPHABET[(rand_buf[i] & 63) as usize];
    }
    return out;
}

/// Validates whether `id` consists solely of characters in `alphabet` and has length `expected_len`
/// (if `expected_len == 0`, any non-empty length is accepted).
pub fn isValidCustom(id: []const u8, alphabet: []const u8, expected_len: usize): bool {
    if (id.len == 0 || alphabet.len == 0) return false;
    if (expected_len > 0 && id.len != expected_len) return false;
    let i: usize = 0;
    while (i < id.len) : (i += 1) {
        const c = id[i];
        let found = false;
        let j: usize = 0;
        while (j < alphabet.len) : (j += 1) {
            if (alphabet[j] == c) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}

/// Validates whether `id` is a valid standard 21-character URL-safe NanoID (`[A-Za-z0-9_-]{21}`).
pub fn isValidNanoid(id: []const u8): bool {
    return isValidCustom(id, URL_ALPHABET, DEFAULT_SIZE);
}
