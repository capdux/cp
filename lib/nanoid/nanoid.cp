// Source: cp/lib/nanoid/nanoid.cp (NanoID, Monotonic ULID & RFC 9562 UUID v7 Generator/Parser)
// Pure CP zero-dependency ID generation and parsing library:
//   - IdRng: deterministic seeded SplitMix64 PRNG
//   - NanoId: unbiased rejection-sampling generator with default/custom alphabets
//   - Ulid & MonotonicUlidGenerator: 128-bit Crockford Base32 encoding/decoding & monotonic 80-bit increment
//   - UuidV7: RFC 9562 48-bit Unix ms + version 7 + variant 2 generator/parser

const std = @import("std");

const MAX_TIMESTAMP_MS: u64 = 0xFFFF_FFFF_FFFF;
const RAND80_MASK: u128 = ((1 as u128) << 80) - 1;
const NANOID_DEFAULT_ALPHABET: []const u8 = "_-0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const CROCKFORD_ALPHABET: []const u8 = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
const HEX_LOWER: []const u8 = "0123456789abcdef";

/// Error set for `cap:nanoid` ID generation, encoding, and parsing operations.
pub err IdError {
    InvalidLength,
    InvalidAlphabet,
    InvalidUlid,
    InvalidUuid,
    MonotonicOverflow,
    TimestampOutOfRange,
    OutOfMemory,
}

/// Deterministic seeded SplitMix64 PRNG for reproducible ID generation.
pub struct IdRng {
    state: u64,

    /// Initializes a deterministic PRNG from a 64-bit `seed`.
    pub fn init(seed: u64): IdRng {
        return IdRng{ state: seed };
    }

    /// Advances the SplitMix64 state and returns the next 64-bit pseudorandom word.
    pub fn nextU64(*_): u64 {
        _.state = _.state +% 0x9e3779b97f4a7c15;
        let z: u64 = _.state;
        z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
        z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
        return z ^ (z >> 31);
    }

    /// Fills `buf` with pseudorandom bytes from `nextU64()`.
    pub fn fillBytes(*_, buf: []u8) {
        let i: usize = 0;
        while (i < buf.len) {
            let w = _.nextU64();
            let k: usize = 0;
            while (k < 8 && i < buf.len) {
                buf[i] = (w & 0xFF) as u8;
                w = w >> 8;
                i += 1;
                k += 1;
            }
        }
    }
}

/// Unbiased NanoID generator using bitmask rejection sampling.
pub static struct NanoId {
    const DEFAULT_ALPHABET: []const u8 = NANOID_DEFAULT_ALPHABET;
    const DEFAULT_SIZE: usize = 21;

    /// Generates a NanoID of length `len` using the default 64-character URL-safe alphabet.
    pub fn generate(allocator: std.mem.Allocator, rng: *IdRng, len: usize): ([]u8) ! IdError {
        return NanoId.generateCustom(allocator, rng, NANOID_DEFAULT_ALPHABET, len);
    }

    /// Generates an unbiased NanoID of length `len` using a custom `alphabet` (1..=256 bytes).
    pub fn generateCustom(
        allocator: std.mem.Allocator,
        rng: *IdRng,
        alphabet: []const u8,
        len: usize,
    ): ([]u8) ! IdError {
        if (alphabet.len == 0 || alphabet.len > 256) {
            return IdError.InvalidAlphabet;
        }
        if (len == 0) {
            return IdError.InvalidLength;
        }

        let out = allocator.alloc(u8, len) catch return IdError.OutOfMemory;

        if (alphabet.len == 1) {
            const ch = alphabet[0];
            for (i of 0..len) {
                out[i] = ch;
            }
            return out;
        }

        let mask_u16: u16 = 1;
        while (mask_u16 < ((alphabet.len - 1) as u16)) {
            mask_u16 = (mask_u16 << 1) | 1;
        }
        const mask: u8 = (mask_u16 & 0xFF) as u8;

        let entropy: [64]u8 = undefined;
        let written: usize = 0;
        while (written < len) {
            rng.fillBytes(&entropy);
            for (ei of 0..entropy.len) {
                const idx = (entropy[ei] & mask) as usize;
                if (idx < alphabet.len) {
                    out[written] = alphabet[idx];
                    written += 1;
                    if (written == len) break;
                }
            }
        }
        return out;
    }
}

fn decodeCrockfordChar(c: u8): u8 ! IdError {
    if (c >= '0' && c <= '9') return c - '0';
    if (c == 'O' || c == 'o') return 0;
    if (c == 'I' || c == 'i' || c == 'L' || c == 'l') return 1;
    let upper = c;
    if (upper >= 'a' && upper <= 'z') {
        upper = upper - ('a' - 'A');
    }
    if (upper >= 'A' && upper <= 'H') return 10 + (upper - 'A');
    if (upper == 'J' || upper == 'K') return 18 + (upper - 'J');
    if (upper == 'M' || upper == 'N') return 20 + (upper - 'M');
    if (upper >= 'P' && upper <= 'T') return 22 + (upper - 'P');
    if (upper >= 'V' && upper <= 'Z') return 27 + (upper - 'V');
    return IdError.InvalidUlid;
}

fn readRand80(rng: *IdRng): u128 {
    let buf: [10]u8 = undefined;
    rng.fillBytes(&buf);
    let r: u128 = 0;
    for (i of 0..10) {
        r = (r << 8) | (buf[i] as u128);
    }
    return r & RAND80_MASK;
}

/// 128-bit Universally Unique Lexicographically Sortable Identifier (ULID).
pub struct Ulid {
    value: u128,

    /// Constructs a `Ulid` from a 48-bit `timestamp_ms` and 80-bit `rand_bits`.
    pub fn fromParts(timestamp_ms: u64, rand_bits: u128): Ulid ! IdError {
        if (timestamp_ms > MAX_TIMESTAMP_MS) {
            return IdError.TimestampOutOfRange;
        }
        const v = ((timestamp_ms as u128) << 80) | (rand_bits & RAND80_MASK);
        return Ulid{ value: v };
    }

    /// Generates a `Ulid` for `timestamp_ms` using `rng` for the 80-bit random component.
    pub fn generate(rng: *IdRng, timestamp_ms: u64): Ulid ! IdError {
        if (timestamp_ms > MAX_TIMESTAMP_MS) {
            return IdError.TimestampOutOfRange;
        }
        const r = readRand80(rng);
        return Ulid.fromParts(timestamp_ms, r);
    }

    /// Returns the 48-bit Unix millisecond timestamp.
    pub fn timestampMs(): u64 {
        return ((_.value >> 80) & (MAX_TIMESTAMP_MS as u128)) as u64;
    }

    /// Returns the 80-bit random payload.
    pub fn rand80(): u128 {
        return _.value & RAND80_MASK;
    }

    /// Encodes this `Ulid` as a 26-character Crockford Base32 array.
    pub fn encode(): [26]u8 {
        let out: [26]u8 = undefined;
        let v = _.value;
        let i: usize = 26;
        while (i > 0) {
            i -= 1;
            out[i] = CROCKFORD_ALPHABET[(v & 0x1F) as usize];
            v = v >> 5;
        }
        return out;
    }

    /// Decodes a 26-character Crockford Base32 string (case-insensitive, I/L->1, O->0).
    pub fn decode(str: []const u8): Ulid ! IdError {
        if (str.len != 26) {
            return IdError.InvalidUlid;
        }
        const first = decodeCrockfordChar(str[0])?;
        if (first > 7) {
            return IdError.InvalidUlid;
        }
        let val: u128 = first as u128;
        for (i of 1..26) {
            const d = decodeCrockfordChar(str[i])?;
            val = (val << 5) | (d as u128);
        }
        return Ulid{ value: val };
    }
}

/// Monotonic ULID generator that increments the 80-bit random component within the same millisecond.
pub struct MonotonicUlidGenerator {
    last_timestamp_ms: u64 = 0,
    last_rand80: u128 = 0,
    has_last: bool = false,

    /// Creates a fresh `MonotonicUlidGenerator`.
    pub fn init(): MonotonicUlidGenerator {
        return MonotonicUlidGenerator{
            last_timestamp_ms: 0,
            last_rand80: 0,
            has_last: false,
        };
    }

    /// Restores generator state from a previous `(last_timestamp_ms, last_rand80)` pair.
    pub fn fromState(last_timestamp_ms: u64, last_rand80: u128): MonotonicUlidGenerator ! IdError {
        if (last_timestamp_ms > MAX_TIMESTAMP_MS) {
            return IdError.TimestampOutOfRange;
        }
        return MonotonicUlidGenerator{
            last_timestamp_ms: last_timestamp_ms,
            last_rand80: last_rand80 & RAND80_MASK,
            has_last: true,
        };
    }

    /// Generates the next strictly monotonic `Ulid` for `timestamp_ms`.
    pub fn next(*_, rng: *IdRng, timestamp_ms: u64): Ulid ! IdError {
        if (timestamp_ms > MAX_TIMESTAMP_MS) {
            return IdError.TimestampOutOfRange;
        }
        if (!_.has_last || timestamp_ms > _.last_timestamp_ms) {
            const r = readRand80(rng);
            _.last_timestamp_ms = timestamp_ms;
            _.last_rand80 = r;
            _.has_last = true;
            return Ulid.fromParts(timestamp_ms, r);
        }
        if (_.last_rand80 == RAND80_MASK) {
            return IdError.MonotonicOverflow;
        }
        _.last_rand80 += 1;
        return Ulid.fromParts(_.last_timestamp_ms, _.last_rand80);
    }
}

fn decodeHexNibble(c: u8): u8 ! IdError {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return IdError.InvalidUuid;
}

/// RFC 9562 UUID v7 (48-bit Unix ms + version `0b0111` + 12-bit rand_a + variant `0b10` + 62-bit rand_b).
pub struct UuidV7 {
    bytes: [16]u8,

    /// Generates an RFC 9562 UUID v7 for `timestamp_ms` using `rng`.
    pub fn generate(rng: *IdRng, timestamp_ms: u64): UuidV7 ! IdError {
        if (timestamp_ms > MAX_TIMESTAMP_MS) {
            return IdError.TimestampOutOfRange;
        }
        let b: [16]u8 = undefined;
        rng.fillBytes(&b);
        b[0] = ((timestamp_ms >> 40) & 0xFF) as u8;
        b[1] = ((timestamp_ms >> 32) & 0xFF) as u8;
        b[2] = ((timestamp_ms >> 24) & 0xFF) as u8;
        b[3] = ((timestamp_ms >> 16) & 0xFF) as u8;
        b[4] = ((timestamp_ms >> 8) & 0xFF) as u8;
        b[5] = (timestamp_ms & 0xFF) as u8;
        b[6] = (b[6] & 0x0F) | 0x70;
        b[8] = (b[8] & 0x3F) | 0x80;
        return UuidV7{ bytes: b };
    }

    /// Returns the 48-bit Unix millisecond timestamp.
    pub fn timestampMs(): u64 {
        return ((_.bytes[0] as u64) << 40) |
            ((_.bytes[1] as u64) << 32) |
            ((_.bytes[2] as u64) << 24) |
            ((_.bytes[3] as u64) << 16) |
            ((_.bytes[4] as u64) << 8) |
            (_.bytes[5] as u64);
    }

    /// Returns the 4-bit UUID version nibble (7 for UUID v7).
    pub fn version(): u8 {
        return (_.bytes[6] >> 4) & 0x0F;
    }

    /// Returns the 2-bit UUID variant field (2 = `0b10` for RFC 9562).
    pub fn variant(): u8 {
        return (_.bytes[8] >> 6) & 0x03;
    }

    /// Formats this UUID v7 into a canonical lowercase 36-byte `8-4-4-4-12` string.
    pub fn format(): [36]u8 {
        let out: [36]u8 = undefined;
        let pos: usize = 0;
        for (i of 0..16) {
            if (i == 4 || i == 6 || i == 8 || i == 10) {
                out[pos] = '-';
                pos += 1;
            }
            out[pos] = HEX_LOWER[((_.bytes[i] >> 4) & 0x0F) as usize];
            out[pos + 1] = HEX_LOWER[(_.bytes[i] & 0x0F) as usize];
            pos += 2;
        }
        return out;
    }

    /// Parses a 36-byte canonical UUID v7 string and validates version=7 and variant=2.
    pub fn parse(str: []const u8): UuidV7 ! IdError {
        if (str.len != 36) {
            return IdError.InvalidUuid;
        }
        if (str[8] != '-' || str[13] != '-' || str[18] != '-' || str[23] != '-') {
            return IdError.InvalidUuid;
        }
        let b: [16]u8 = undefined;
        let si: usize = 0;
        for (bi of 0..16) {
            if (bi == 4 || bi == 6 || bi == 8 || bi == 10) {
                si += 1;
            }
            const hi = decodeHexNibble(str[si])?;
            const lo = decodeHexNibble(str[si + 1])?;
            b[bi] = (hi << 4) | lo;
            si += 2;
        }
        if (((b[6] >> 4) & 0x0F) != 7 || ((b[8] >> 6) & 0x03) != 2) {
            return IdError.InvalidUuid;
        }
        return UuidV7{ bytes: b };
    }
}
