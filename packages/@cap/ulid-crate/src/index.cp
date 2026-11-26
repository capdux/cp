// packages/@cap/ulid-crate/src/index.cp
// Zero-allocation 128-bit lexicographically sortable ULID codec & monotonic incrementer.
// Clean-room MIT parity with crates.io `ulid` v1 & ULID Specification.
// Layout: 48-bit big-endian millisecond timestamp + 80-bit randomness stored as (hi: u64, lo: u64).

const ULD_ALPHABET: string = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
const ULD_MASK_48: u64 = 0x0000FFFFFFFFFFFF as u64;
const ULD_MASK_16: u64 = 0x000000000000FFFF as u64;
const ULD_MAX_U64: u64 = 0xFFFFFFFFFFFFFFFF as u64;

fn uldDecodeChar(ch: u8): u8 {
    // '0'..'9'
    if (ch >= (0x30 as u8) && ch <= (0x39 as u8)) {
        return ch - (0x30 as u8);
    }
    // Normalize lowercase 'a'..'z' to uppercase 'A'..'Z'
    let c = ch;
    if (c >= (0x61 as u8) && c <= (0x7A as u8)) {
        c = c - (32 as u8);
    }
    // Crockford Base32 tolerances: O -> 0, I/L -> 1
    if (c == (0x4F as u8)) { return 0; } // 'O'
    if (c == (0x49 as u8) || c == (0x4C as u8)) { return 1; } // 'I', 'L'
    // 'A'..'H' -> 10..17
    if (c >= (0x41 as u8) && c <= (0x48 as u8)) {
        return (10 as u8) + (c - (0x41 as u8));
    }
    // 'J'..'K' -> 18..19
    if (c == (0x4A as u8)) { return 18; }
    if (c == (0x4B as u8)) { return 19; }
    // 'M'..'N' -> 20..21
    if (c == (0x4D as u8)) { return 20; }
    if (c == (0x4E as u8)) { return 21; }
    // 'P'..'T' -> 22..26
    if (c >= (0x50 as u8) && c <= (0x54 as u8)) {
        return (22 as u8) + (c - (0x50 as u8));
    }
    // 'V'..'Z' -> 27..31 ('U' is invalid in Crockford Base32)
    if (c >= (0x56 as u8) && c <= (0x5A as u8)) {
        return (27 as u8) + (c - (0x56 as u8));
    }
    return 255 as u8;
}

/// 128-bit ULID stored as big-endian `(hi: u64, lo: u64)`.
/// `hi` holds the 48-bit millisecond timestamp in bits 63..16 and high 16 random bits in bits 15..0.
/// `lo` holds the low 64 random bits.
pub struct Ulid {
    pub hi: u64,
    pub lo: u64,

    /// Construct a ULID from a 48-bit millisecond timestamp and 80-bit random entropy (`rand_hi16`, `rand_lo64`).
    pub static fn fromParts(timestamp_ms: u64, rand_hi16: u16, rand_lo64: u64): Ulid {
        const hi = ((timestamp_ms & ULD_MASK_48) << (16 as u6)) | (rand_hi16 as u64);
        return Ulid { hi: hi, lo: rand_lo64 };
    }

    /// Parse a ULID from exactly 16 big-endian bytes. Returns `null` if `src.len != 16`.
    pub static fn fromBytes(src: []const u8): Ulid? {
        if (src.len != 16) {
            return null;
        }
        let hi: u64 = 0;
        let lo: u64 = 0;
        let i: usize = 0;
        while (i < 8) {
            hi = (hi << (8 as u6)) | (src[i] as u64);
            lo = (lo << (8 as u6)) | (src[i + 8] as u64);
            i += 1;
        }
        return Ulid { hi: hi, lo: lo };
    }

    /// Extract the 48-bit Unix millisecond timestamp.
    pub fn timestampMs(): u64 {
        return _.hi >> (16 as u6);
    }

    /// Lexicographic / unsigned 128-bit comparison returning `-1`, `0`, or `1`.
    pub fn cmp(other: Ulid): i32 {
        if (_.hi < other.hi) { return -1; }
        if (_.hi > other.hi) { return 1; }
        if (_.lo < other.lo) { return -1; }
        if (_.lo > other.lo) { return 1; }
        return 0;
    }

    /// Value equality comparison.
    pub fn eq(other: Ulid): bool {
        return _.hi == other.hi && _.lo == other.lo;
    }

    /// Monotonically increment the 80-bit random component by 1 within the same millisecond.
    /// Returns `null` if all 80 random bits are `1` (overflow).
    pub fn increment(): Ulid? {
        if (_.lo != ULD_MAX_U64) {
            return Ulid { hi: _.hi, lo: _.lo + (1 as u64) };
        }
        if ((_.hi & ULD_MASK_16) == ULD_MASK_16) {
            return null;
        }
        return Ulid { hi: _.hi + (1 as u64), lo: 0 as u64 };
    }

    /// Write the 16-byte big-endian binary representation into `out`.
    /// Returns `16` on success, or `0` if `out.len < 16`.
    pub fn toBytes(out: []u8): usize {
        if (out.len < 16) {
            return 0;
        }
        out[0] = ((_.hi >> (56 as u6)) & (0xFF as u64)) as u8;
        out[1] = ((_.hi >> (48 as u6)) & (0xFF as u64)) as u8;
        out[2] = ((_.hi >> (40 as u6)) & (0xFF as u64)) as u8;
        out[3] = ((_.hi >> (32 as u6)) & (0xFF as u64)) as u8;
        out[4] = ((_.hi >> (24 as u6)) & (0xFF as u64)) as u8;
        out[5] = ((_.hi >> (16 as u6)) & (0xFF as u64)) as u8;
        out[6] = ((_.hi >> (8 as u6)) & (0xFF as u64)) as u8;
        out[7] = (_.hi & (0xFF as u64)) as u8;

        out[8] = ((_.lo >> (56 as u6)) & (0xFF as u64)) as u8;
        out[9] = ((_.lo >> (48 as u6)) & (0xFF as u64)) as u8;
        out[10] = ((_.lo >> (40 as u6)) & (0xFF as u64)) as u8;
        out[11] = ((_.lo >> (32 as u6)) & (0xFF as u64)) as u8;
        out[12] = ((_.lo >> (24 as u6)) & (0xFF as u64)) as u8;
        out[13] = ((_.lo >> (16 as u6)) & (0xFF as u64)) as u8;
        out[14] = ((_.lo >> (8 as u6)) & (0xFF as u64)) as u8;
        out[15] = (_.lo & (0xFF as u64)) as u8;
        return 16;
    }
}

/// Encode `u` as a 26-character uppercase Crockford Base32 string into `out`.
/// Returns `26` on success, or `0` if `out.len < 26`.
pub fn encodeUlid(u: Ulid, out: []u8): usize {
    if (out.len < 26) {
        return 0;
    }
    const hi = u.hi;
    const lo = u.lo;

    // 10 timestamp characters (bits 127..80 -> hi bits 63..16)
    out[0] = ULD_ALPHABET[((hi >> (61 as u6)) & (0x07 as u64)) as usize];
    out[1] = ULD_ALPHABET[((hi >> (56 as u6)) & (0x1F as u64)) as usize];
    out[2] = ULD_ALPHABET[((hi >> (51 as u6)) & (0x1F as u64)) as usize];
    out[3] = ULD_ALPHABET[((hi >> (46 as u6)) & (0x1F as u64)) as usize];
    out[4] = ULD_ALPHABET[((hi >> (41 as u6)) & (0x1F as u64)) as usize];
    out[5] = ULD_ALPHABET[((hi >> (36 as u6)) & (0x1F as u64)) as usize];
    out[6] = ULD_ALPHABET[((hi >> (31 as u6)) & (0x1F as u64)) as usize];
    out[7] = ULD_ALPHABET[((hi >> (26 as u6)) & (0x1F as u64)) as usize];
    out[8] = ULD_ALPHABET[((hi >> (21 as u6)) & (0x1F as u64)) as usize];
    out[9] = ULD_ALPHABET[((hi >> (16 as u6)) & (0x1F as u64)) as usize];

    // 16 randomness characters (bits 79..0 -> hi bits 15..0 + lo bits 63..0)
    out[10] = ULD_ALPHABET[((hi >> (11 as u6)) & (0x1F as u64)) as usize];
    out[11] = ULD_ALPHABET[((hi >> (6 as u6)) & (0x1F as u64)) as usize];
    out[12] = ULD_ALPHABET[((hi >> (1 as u6)) & (0x1F as u64)) as usize];
    out[13] = ULD_ALPHABET[((((hi & (1 as u64)) << (4 as u6)) | ((lo >> (60 as u6)) & (0x0F as u64)))) as usize];
    out[14] = ULD_ALPHABET[((lo >> (55 as u6)) & (0x1F as u64)) as usize];
    out[15] = ULD_ALPHABET[((lo >> (50 as u6)) & (0x1F as u64)) as usize];
    out[16] = ULD_ALPHABET[((lo >> (45 as u6)) & (0x1F as u64)) as usize];
    out[17] = ULD_ALPHABET[((lo >> (40 as u6)) & (0x1F as u64)) as usize];
    out[18] = ULD_ALPHABET[((lo >> (35 as u6)) & (0x1F as u64)) as usize];
    out[19] = ULD_ALPHABET[((lo >> (30 as u6)) & (0x1F as u64)) as usize];
    out[20] = ULD_ALPHABET[((lo >> (25 as u6)) & (0x1F as u64)) as usize];
    out[21] = ULD_ALPHABET[((lo >> (20 as u6)) & (0x1F as u64)) as usize];
    out[22] = ULD_ALPHABET[((lo >> (15 as u6)) & (0x1F as u64)) as usize];
    out[23] = ULD_ALPHABET[((lo >> (10 as u6)) & (0x1F as u64)) as usize];
    out[24] = ULD_ALPHABET[((lo >> (5 as u6)) & (0x1F as u64)) as usize];
    out[25] = ULD_ALPHABET[(lo & (0x1F as u64)) as usize];
    return 26;
}

/// Strictly decode a 26-character Crockford Base32 ULID string.
/// Supports case-insensitivity and Crockford aliases (`I`/`L` -> `1`, `O` -> `0`).
/// Rejects invalid length, invalid characters (`U`, symbols), or 128-bit overflow (first char > `'7'`).
pub fn decodeUlid(s: string): Ulid? {
    if (s.len != 26) {
        return null;
    }

    let vals: [26]u8 = undefined;
    let i: usize = 0;
    while (i < 26) {
        const v = uldDecodeChar(s[i]);
        if (v == (255 as u8)) {
            return null;
        }
        vals[i] = v;
        i += 1;
    }

    // First character holds only 3 bits (0..7); >= 8 overflows 128 bits.
    if (vals[0] > (7 as u8)) {
        return null;
    }

    let hi: u64 = vals[0] as u64;
    let k: usize = 1;
    while (k <= 12) {
        hi = (hi << (5 as u6)) | (vals[k] as u64);
        k += 1;
    }
    hi = (hi << (1 as u6)) | (((vals[13] >> (4 as u3)) & (1 as u8)) as u64);

    let lo: u64 = (vals[13] & (0x0F as u8)) as u64;
    k = 14;
    while (k < 26) {
        lo = (lo << (5 as u6)) | (vals[k] as u64);
        k += 1;
    }

    return Ulid { hi: hi, lo: lo };
}

/// Return `true` if `s` is a syntactically and numerically valid 26-character ULID.
pub fn isValidUlid(s: string): bool {
    return decodeUlid(s) != null;
}

export {
    Ulid,
    encodeUlid,
    decodeUlid,
    isValidUlid,
};
