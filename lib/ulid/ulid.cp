// Source: cp/lib/ulid/ulid.cp (128-bit Big-Endian ULID Model)
// Implements Ulid construction, field accessors, Crockford Base32 encoding/decoding, and UUID conversion.

import {
    ULID_BYTE_LEN,
    ULID_ENTROPY_LEN,
    ULID_MAX_TIMESTAMP,
    UlidError,
} from "./types.cp";
import {
    encodeBytes,
    decodeBytes,
    bytesToUuidString,
    uuidStringToBytes,
} from "./codec.cp";

pub struct Ulid {
    bytes: [16]u8 = [0; 16],

    /// Returns the Nil ULID (all zero bytes).
    pub static fn nil(): Ulid {
        return Ulid{ bytes: [0; 16] };
    }

    /// Returns the Max ULID (all 0xFF bytes, `7ZZZZZZZZZZZZZZZZZZZZZZZZZ`).
    pub static fn max(): Ulid {
        return Ulid{ bytes: [0xFF; 16] };
    }

    /// Constructs a Ulid directly from 16 big-endian bytes.
    pub static fn fromBytes(bytes: [16]u8): Ulid {
        return Ulid{ bytes: bytes };
    }

    /// Constructs a Ulid from a 48-bit millisecond timestamp and 80-bit (10-byte) entropy.
    pub static fn fromParts(timestamp_ms: u64, ent: [10]u8): Ulid ! UlidError {
        if (timestamp_ms > ULID_MAX_TIMESTAMP) {
            return UlidError.TimestampOverflow;
        }
        let b: [16]u8 = [0; 16];
        b[0] = ((timestamp_ms >> 40) & 0xFF) as u8;
        b[1] = ((timestamp_ms >> 32) & 0xFF) as u8;
        b[2] = ((timestamp_ms >> 24) & 0xFF) as u8;
        b[3] = ((timestamp_ms >> 16) & 0xFF) as u8;
        b[4] = ((timestamp_ms >> 8) & 0xFF) as u8;
        b[5] = (timestamp_ms & 0xFF) as u8;
        for (i of 0..ULID_ENTROPY_LEN) {
            b[6 + i] = ent[i];
        }
        return Ulid{ bytes: b };
    }

    /// Returns the raw 16-byte big-endian representation.
    pub fn toBytes(): [16]u8 {
        return _.bytes;
    }

    /// Extracts the 48-bit Unix epoch millisecond timestamp.
    pub fn timestampMs(): u64 {
        let ts: u64 = 0;
        for (i of 0..6) {
            ts = (ts << 8) | (_.bytes[i] as u64);
        }
        return ts;
    }

    /// Extracts the 80-bit (10-byte) entropy component.
    pub fn entropy(): [10]u8 {
        let out: [10]u8 = [0; 10];
        for (i of 0..ULID_ENTROPY_LEN) {
            out[i] = _.bytes[6 + i];
        }
        return out;
    }

    /// Lexicographically compares this ULID with `other`, returning -1, 0, or 1.
    pub fn compare(other: Ulid): i8 {
        for (i of 0..ULID_BYTE_LEN) {
            if (_.bytes[i] < other.bytes[i]) {
                return -1;
            }
            if (_.bytes[i] > other.bytes[i]) {
                return 1;
            }
        }
        return 0;
    }

    /// Returns true if both ULIDs have identical 128-bit representations.
    pub fn equals(other: Ulid): bool {
        for (i of 0..ULID_BYTE_LEN) {
            if (_.bytes[i] != other.bytes[i]) {
                return false;
            }
        }
        return true;
    }

    /// Returns true if all 16 bytes are zero.
    pub fn isZero(): bool {
        for (i of 0..ULID_BYTE_LEN) {
            if (_.bytes[i] != 0) {
                return false;
            }
        }
        return true;
    }

    /// Encodes the ULID as a 26-character uppercase Crockford Base32 array without heap allocation.
    pub fn encode(): [26]u8 {
        return encodeBytes(_.bytes, true);
    }

    /// Encodes the ULID as a 26-character lowercase Crockford Base32 array without heap allocation.
    pub fn encodeLower(): [26]u8 {
        return encodeBytes(_.bytes, false);
    }

    /// Decodes a Crockford Base32 string (case-insensitive, supports I/L->1, O->0, optional hyphens).
    pub static fn decode(str: []const u8): Ulid ! UlidError {
        const b = decodeBytes(str, false)?;
        return Ulid{ bytes: b };
    }

    /// Strictly decodes a 26-character Crockford Base32 string (no hyphens, no I/L/O aliases).
    pub static fn decodeStrict(str: []const u8): Ulid ! UlidError {
        const b = decodeBytes(str, true)?;
        return Ulid{ bytes: b };
    }

    /// Converts this ULID to a canonical 36-character lowercase 8-4-4-4-12 UUID string.
    pub fn toUuidString(): [36]u8 {
        return bytesToUuidString(_.bytes);
    }

    /// Parses a canonical 36-character 8-4-4-4-12 UUID string into a Ulid.
    pub static fn fromUuidString(s: []const u8): Ulid ! UlidError {
        const b = uuidStringToBytes(s)?;
        return Ulid{ bytes: b };
    }
}
