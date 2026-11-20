// Source: cp/lib/ulid/codec.cp (Crockford Base32 and UUID Hex Zero-Alloc Codec)
// Pure CP zero-heap-allocation Crockford Base32 (26-char) and UUID (36-char) codecs.

const std = @import("std");
import {
    ULID_BYTE_LEN,
    ULID_ENCODED_LEN,
    ULID_UUID_LEN,
    UlidError,
} from "./types.cp";

const CROCKFORD_UPPER: []const u8 = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
const CROCKFORD_LOWER: []const u8 = "0123456789abcdefghjkmnpqrstvwxyz";
const HEX_LOWER: []const u8 = "0123456789abcdef";

fn decodeCrockfordCharStrict(c: u8): u8? {
    return switch (c) {
        '0' => 0,
        '1' => 1,
        '2' => 2,
        '3' => 3,
        '4' => 4,
        '5' => 5,
        '6' => 6,
        '7' => 7,
        '8' => 8,
        '9' => 9,
        'A' | 'a' => 10,
        'B' | 'b' => 11,
        'C' | 'c' => 12,
        'D' | 'd' => 13,
        'E' | 'e' => 14,
        'F' | 'f' => 15,
        'G' | 'g' => 16,
        'H' | 'h' => 17,
        'J' | 'j' => 18,
        'K' | 'k' => 19,
        'M' | 'm' => 20,
        'N' | 'n' => 21,
        'P' | 'p' => 22,
        'Q' | 'q' => 23,
        'R' | 'r' => 24,
        'S' | 's' => 25,
        'T' | 't' => 26,
        'V' | 'v' => 27,
        'W' | 'w' => 28,
        'X' | 'x' => 29,
        'Y' | 'y' => 30,
        'Z' | 'z' => 31,
        else => null,
    };
}

fn decodeCrockfordCharPermissive(c: u8): u8? {
    return switch (c) {
        '0' | 'O' | 'o' => 0,
        '1' | 'I' | 'i' | 'L' | 'l' => 1,
        else => decodeCrockfordCharStrict(c),
    };
}

fn decodeHexNibble(c: u8): u8? {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return (c - 'a') + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return (c - 'A') + 10;
    }
    return null;
}

/// Encodes 16 big-endian ULID bytes into a 26-char Crockford Base32 array without heap allocation.
pub fn encodeBytes(bytes: [16]u8, uppercase: bool): [26]u8 {
    const alphabet = if (uppercase) CROCKFORD_UPPER else CROCKFORD_LOWER;
    const val: u128 = std.mem.readInt(u128, &bytes, .big);
    let out: [26]u8 = [0; 26];
    for (i of 0..ULID_ENCODED_LEN) {
        const shift: u7 = @intCast((25 - i) * 5);
        const idx: usize = @intCast((val >> shift) & 0x1F);
        out[i] = alphabet[idx];
    }
    return out;
}

/// Decodes a Crockford Base32 ULID string into 16 big-endian bytes.
/// When `strict` is true, hyphens and I/L/O aliases are rejected and length must be exactly 26.
pub fn decodeBytes(str: []const u8, strict: bool): [16]u8 ! UlidError {
    let digits: [26]u8 = [0; 26];
    let count: usize = 0;

    if (strict) {
        if (str.len != ULID_ENCODED_LEN) {
            return UlidError.InvalidLength;
        }
        for (i of 0..ULID_ENCODED_LEN) {
            const v = decodeCrockfordCharStrict(str[i]) ?? {
                return UlidError.InvalidChar;
            };
            digits[i] = v;
        }
        count = ULID_ENCODED_LEN;
    } else {
        for (i of 0..str.len) {
            const ch = str[i];
            if (ch == '-') {
                continue;
            }
            const v = decodeCrockfordCharPermissive(ch) ?? {
                return UlidError.InvalidChar;
            };
            if (count >= ULID_ENCODED_LEN) {
                return UlidError.InvalidLength;
            }
            digits[count] = v;
            count += 1;
        }
        if (count != ULID_ENCODED_LEN) {
            return UlidError.InvalidLength;
        }
    }

    // First Crockford Base32 digit encodes bits 129..125; top 2 bits must be zero (max '7').
    if (digits[0] > 7) {
        return UlidError.TimestampOverflow;
    }

    let acc: u128 = 0;
    for (i of 0..ULID_ENCODED_LEN) {
        acc = (acc << 5) | (digits[i] as u128);
    }

    let out: [16]u8 = [0; 16];
    std.mem.writeInt(u128, &out, acc, .big);
    return out;
}

/// Formats 16 big-endian bytes as a canonical 36-char lowercase 8-4-4-4-12 UUID string.
pub fn bytesToUuidString(bytes: [16]u8): [36]u8 {
    let out: [36]u8 = [0; 36];
    let out_idx: usize = 0;
    for (i of 0..ULID_BYTE_LEN) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            out[out_idx] = '-';
            out_idx += 1;
        }
        const b = bytes[i];
        out[out_idx] = HEX_LOWER[(b >> 4) & 0x0F];
        out[out_idx + 1] = HEX_LOWER[b & 0x0F];
        out_idx += 2;
    }
    return out;
}

/// Parses a canonical 36-char 8-4-4-4-12 UUID hex string into 16 big-endian bytes.
pub fn uuidStringToBytes(s: []const u8): [16]u8 ! UlidError {
    if (s.len != ULID_UUID_LEN) {
        return UlidError.InvalidUuid;
    }
    if (s[8] != '-' || s[13] != '-' || s[18] != '-' || s[23] != '-') {
        return UlidError.InvalidUuid;
    }

    let out: [16]u8 = [0; 16];
    let byte_idx: usize = 0;
    let i: usize = 0;
    while (i < ULID_UUID_LEN) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            i += 1;
            continue;
        }
        const hi = decodeHexNibble(s[i]) ?? {
            return UlidError.InvalidUuid;
        };
        const lo = decodeHexNibble(s[i + 1]) ?? {
            return UlidError.InvalidUuid;
        };
        out[byte_idx] = (hi << 4) | lo;
        byte_idx += 1;
        i += 2;
    }
    return out;
}
