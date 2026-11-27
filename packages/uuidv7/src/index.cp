// @cap/uuid-v7 — Zero-allocation RFC 9562 UUIDv7 Engine
// Clean-room MIT implementation in pure CP.

import { hexCharLower, hexVal } from "cap:ascii";

pub struct UuidV7 {
    pub bytes: [16]u8,
}

/// Constructs an RFC 9562 UUIDv7 from a 48-bit Unix millisecond timestamp and 10 random bytes.
/// Enforces version nibble `7` (`0111`) and RFC variant bits (`10`).
pub fn fromParts(timestamp_ms: u64, rand_bytes: [10]u8) -> UuidV7 {
    let b: [16]u8 = undefined;
    b[0] = ((timestamp_ms >> 40) & 0xFF) as u8;
    b[1] = ((timestamp_ms >> 32) & 0xFF) as u8;
    b[2] = ((timestamp_ms >> 24) & 0xFF) as u8;
    b[3] = ((timestamp_ms >> 16) & 0xFF) as u8;
    b[4] = ((timestamp_ms >> 8) & 0xFF) as u8;
    b[5] = (timestamp_ms & 0xFF) as u8;

    b[6] = (rand_bytes[0] & 0x0F) | 0x70;
    b[7] = rand_bytes[1];
    b[8] = (rand_bytes[2] & 0x3F) | 0x80;

    let i: usize = 3;
    while (i < 10) {
        b[6 + i] = rand_bytes[i];
        i += 1;
    }

    return UuidV7 { bytes: b };
}

/// Extracts the 48-bit Unix millisecond timestamp from a UUIDv7.
pub fn timestampMs(u: UuidV7) -> u64 {
    const b0: u64 = (u.bytes[0] as u64) << 40;
    const b1: u64 = (u.bytes[1] as u64) << 32;
    const b2: u64 = (u.bytes[2] as u64) << 24;
    const b3: u64 = (u.bytes[3] as u64) << 16;
    const b4: u64 = (u.bytes[4] as u64) << 8;
    const b5: u64 = u.bytes[5] as u64;
    return b0 | b1 | b2 | b3 | b4 | b5;
}

/// Extracts the 4-bit UUID version nibble (`(bytes[6] >> 4) & 0x0F`).
pub fn version(u: UuidV7) -> u8 {
    return (u.bytes[6] >> 4) & 0x0F;
}

/// Returns true if the UUID has the RFC 4122 / RFC 9562 variant (`10xx`).
pub fn isRfcVariant(u: UuidV7) -> bool {
    return (u.bytes[8] & 0xC0) == 0x80;
}

/// Formats `u` as a strict 36-byte lowercase hyphenated UUID (`xxxxxxxx-xxxx-7xxx-yxxx-xxxxxxxxxxxx`).
/// Returns `36`, or `0` if `out.len < 36`.
pub fn formatHyphenated(u: UuidV7, out: []u8) -> usize {
    if (out.len < 36) {
        return 0;
    }
    let w: usize = 0;
    let i: usize = 0;
    while (i < 16) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            out[w] = 45; // '-'
            w += 1;
        }
        const byte = u.bytes[i];
        out[w] = hexCharLower((byte >> 4) & 0x0F);
        out[w + 1] = hexCharLower(byte & 0x0F);
        w += 2;
        i += 1;
    }
    return 36;
}

/// Parses a 36-char hyphenated or 32-char compact hex UUIDv7 (case-insensitive) into `out`.
/// Enforces `version == 7` and RFC 9562 variant (`10xx`). Returns true on success.
pub fn parse(s: string, out: *UuidV7) -> bool {
    let hyphenated = false;
    if (s.len == 36) {
        if (s[8] != 45 || s[13] != 45 || s[18] != 45 || s[23] != 45) {
            return false;
        }
        hyphenated = true;
    } else if (s.len != 32) {
        return false;
    }

    let tmp: [16]u8 = undefined;
    let pos: usize = 0;
    let bi: usize = 0;
    while (bi < 16) {
        if (hyphenated && (pos == 8 || pos == 13 || pos == 18 || pos == 23)) {
            pos += 1;
        }
        const hi = hexVal(s[pos]);
        const lo = hexVal(s[pos + 1]);
        if (hi == null || lo == null) {
            return false;
        }
        tmp[bi] = (hi.? << 4) | lo.?;
        pos += 2;
        bi += 1;
    }

    const candidate = UuidV7 { bytes: tmp };
    if (version(candidate) != 7 || !isRfcVariant(candidate)) {
        return false;
    }

    let k: usize = 0;
    while (k < 16) {
        out.bytes[k] = tmp[k];
        k += 1;
    }
    return true;
}

/// Returns true if `s` is a valid RFC 9562 UUIDv7 string (hyphenated or compact 32-hex).
pub fn isValidUuidV7(s: string) -> bool {
    let dummy: UuidV7 = undefined;
    return parse(s, &dummy);
}

/// Lexicographical (chronological) comparison of two UUIDv7 values.
/// Returns `-1` if `a < b`, `0` if `a == b`, `1` if `a > b`.
pub fn compare(a: UuidV7, b: UuidV7) -> i32 {
    let i: usize = 0;
    while (i < 16) {
        if (a.bytes[i] < b.bytes[i]) {
            return -1;
        }
        if (a.bytes[i] > b.bytes[i]) {
            return 1;
        }
        i += 1;
    }
    return 0;
}
