// packages/@cap/ulid/src/index.cp
// Zero-allocation ULID (48-bit Unix ms + 80-bit entropy, Crockford Base32) engine.
// MIT clean-room pure CP implementation.

/// 128-bit big-endian ULID: `bytes[0..6]` = 48-bit Unix ms timestamp, `bytes[6..16]` = 80-bit entropy.
pub struct Ulid {
    pub bytes: [16]u8,
}

fn ulEncodeChar(v: u8): u8 {
    const alphabet = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
    return alphabet[(v & (0x1F as u8)) as usize];
}

/// Decode a Crockford Base32 character (case-insensitive, `I`/`L` -> `1`, `O` -> `0`).
/// Returns `0..31` on success, or `0xFF` on invalid character (e.g. `U`, punctuation).
fn ulDecodeChar(c: u8): u8 {
    if (c >= ('0' as u8) && c <= ('9' as u8)) {
        return c - ('0' as u8);
    }
    let u = c;
    if (u >= ('a' as u8) && u <= ('z' as u8)) {
        u = u - (32 as u8);
    }
    if (u == ('O' as u8)) { return 0 as u8; }
    if (u == ('I' as u8) || u == ('L' as u8)) { return 1 as u8; }
    if (u >= ('A' as u8) && u <= ('H' as u8)) { return (10 as u8) + (u - ('A' as u8)); }
    if (u == ('J' as u8)) { return 18 as u8; }
    if (u == ('K' as u8)) { return 19 as u8; }
    if (u == ('M' as u8)) { return 20 as u8; }
    if (u == ('N' as u8)) { return 21 as u8; }
    if (u >= ('P' as u8) && u <= ('T' as u8)) { return (22 as u8) + (u - ('P' as u8)); }
    if (u >= ('V' as u8) && u <= ('Z' as u8)) { return (27 as u8) + (u - ('V' as u8)); }
    return 0xFF as u8;
}

/// Construct a `Ulid` from a 48-bit Unix millisecond timestamp and 10 bytes (80 bits) of entropy.
pub fn fromParts(timestamp_ms: u64, rand_bytes: [10]u8): Ulid {
    let b: [16]u8 = undefined;
    b[0] = ((timestamp_ms >> (40 as u6)) & (0xFF as u64)) as u8;
    b[1] = ((timestamp_ms >> (32 as u6)) & (0xFF as u64)) as u8;
    b[2] = ((timestamp_ms >> (24 as u6)) & (0xFF as u64)) as u8;
    b[3] = ((timestamp_ms >> (16 as u6)) & (0xFF as u64)) as u8;
    b[4] = ((timestamp_ms >> (8 as u6)) & (0xFF as u64)) as u8;
    b[5] = (timestamp_ms & (0xFF as u64)) as u8;

    let i: usize = 0;
    while (i < 10) {
        b[6 + i] = rand_bytes[i];
        i += 1;
    }
    return Ulid{ bytes: b };
}

/// Extract the 48-bit Unix millisecond timestamp from `u`.
pub fn timestampMs(u: Ulid): u64 {
    let ts: u64 = 0 as u64;
    ts = (ts << (8 as u6)) | (u.bytes[0] as u64);
    ts = (ts << (8 as u6)) | (u.bytes[1] as u64);
    ts = (ts << (8 as u6)) | (u.bytes[2] as u64);
    ts = (ts << (8 as u6)) | (u.bytes[3] as u64);
    ts = (ts << (8 as u6)) | (u.bytes[4] as u64);
    ts = (ts << (8 as u6)) | (u.bytes[5] as u64);
    return ts;
}

fn ulEncodeBlock40(b0: u8, b1: u8, b2: u8, b3: u8, b4: u8, out: []u8, offset: usize) {
    out[offset + 0] = ulEncodeChar((b0 >> (3 as u3)) & (0x1F as u8));
    out[offset + 1] = ulEncodeChar(((b0 & (0x07 as u8)) << (2 as u3)) | (b1 >> (6 as u3)));
    out[offset + 2] = ulEncodeChar((b1 >> (1 as u3)) & (0x1F as u8));
    out[offset + 3] = ulEncodeChar(((b1 & (0x01 as u8)) << (4 as u3)) | (b2 >> (4 as u3)));
    out[offset + 4] = ulEncodeChar(((b2 & (0x0F as u8)) << (1 as u3)) | (b3 >> (7 as u3)));
    out[offset + 5] = ulEncodeChar((b3 >> (2 as u3)) & (0x1F as u8));
    out[offset + 6] = ulEncodeChar(((b3 & (0x03 as u8)) << (3 as u3)) | (b4 >> (5 as u3)));
    out[offset + 7] = ulEncodeChar(b4 & (0x1F as u8));
}

/// Encode `u` into 26 uppercase Crockford Base32 characters in `out`.
/// Returns `26` on success, or `0` if `out.len < 26`.
pub fn encode(u: Ulid, out: []u8): usize {
    if (out.len < 26) {
        return 0;
    }
    const b = u.bytes;

    // 48-bit timestamp -> 10 chars (out[0..10])
    out[0] = ulEncodeChar((b[0] >> (5 as u3)) & (0x07 as u8));
    out[1] = ulEncodeChar(b[0] & (0x1F as u8));
    out[2] = ulEncodeChar((b[1] >> (3 as u3)) & (0x1F as u8));
    out[3] = ulEncodeChar(((b[1] & (0x07 as u8)) << (2 as u3)) | (b[2] >> (6 as u3)));
    out[4] = ulEncodeChar((b[2] >> (1 as u3)) & (0x1F as u8));
    out[5] = ulEncodeChar(((b[2] & (0x01 as u8)) << (4 as u3)) | (b[3] >> (4 as u3)));
    out[6] = ulEncodeChar(((b[3] & (0x0F as u8)) << (1 as u3)) | (b[4] >> (7 as u3)));
    out[7] = ulEncodeChar((b[4] >> (2 as u3)) & (0x1F as u8));
    out[8] = ulEncodeChar(((b[4] & (0x03 as u8)) << (3 as u3)) | (b[5] >> (5 as u3)));
    out[9] = ulEncodeChar(b[5] & (0x1F as u8));

    // 80-bit entropy -> 16 chars (two 40-bit blocks)
    ulEncodeBlock40(b[6], b[7], b[8], b[9], b[10], out, 10);
    ulEncodeBlock40(b[11], b[12], b[13], b[14], b[15], out, 18);
    return 26;
}

/// Decode a 26-character Crockford Base32 ULID string into `out.*`.
/// Case-insensitive, maps `I`/`L` -> `1` and `O` -> `0`, and rejects leading character > `'7'` (128-bit overflow).
pub fn decode(s: string, out: *Ulid): bool {
    if (s.len != 26) {
        return false;
    }

    let v: [26]u8 = undefined;
    let i: usize = 0;
    while (i < 26) {
        const d = ulDecodeChar(s[i]);
        if (d == (0xFF as u8)) {
            return false;
        }
        v[i] = d;
        i += 1;
    }

    // Reject 128-bit overflow: top char only holds 3 bits (0..7).
    if (v[0] > (7 as u8)) {
        return false;
    }

    let b: [16]u8 = undefined;
    // 48-bit timestamp
    b[0] = ((v[0] & (0x07 as u8)) << (5 as u3)) | v[1];
    b[1] = (v[2] << (3 as u3)) | (v[3] >> (2 as u3));
    b[2] = ((v[3] & (0x03 as u8)) << (6 as u3)) | (v[4] << (1 as u3)) | (v[5] >> (4 as u3));
    b[3] = ((v[5] & (0x0F as u8)) << (4 as u3)) | (v[6] >> (1 as u3));
    b[4] = ((v[6] & (0x01 as u8)) << (7 as u3)) | (v[7] << (2 as u3)) | (v[8] >> (3 as u3));
    b[5] = ((v[8] & (0x07 as u8)) << (5 as u3)) | v[9];

    // First 40-bit entropy block (v[10..18] -> b[6..11])
    b[6] = (v[10] << (3 as u3)) | (v[11] >> (2 as u3));
    b[7] = ((v[11] & (0x03 as u8)) << (6 as u3)) | (v[12] << (1 as u3)) | (v[13] >> (4 as u3));
    b[8] = ((v[13] & (0x0F as u8)) << (4 as u3)) | (v[14] >> (1 as u3));
    b[9] = ((v[14] & (0x01 as u8)) << (7 as u3)) | (v[15] << (2 as u3)) | (v[16] >> (3 as u3));
    b[10] = ((v[16] & (0x07 as u8)) << (5 as u3)) | v[17];

    // Second 40-bit entropy block (v[18..26] -> b[11..16])
    b[11] = (v[18] << (3 as u3)) | (v[19] >> (2 as u3));
    b[12] = ((v[19] & (0x03 as u8)) << (6 as u3)) | (v[20] << (1 as u3)) | (v[21] >> (4 as u3));
    b[13] = ((v[21] & (0x0F as u8)) << (4 as u3)) | (v[22] >> (1 as u3));
    b[14] = ((v[22] & (0x01 as u8)) << (7 as u3)) | (v[23] << (2 as u3)) | (v[24] >> (3 as u3));
    b[15] = ((v[24] & (0x07 as u8)) << (5 as u3)) | v[25];

    out.* = Ulid{ bytes: b };
    return true;
}

/// Validate whether `s` is a legal 26-character ULID string.
pub fn isValidUlid(s: string): bool {
    let dummyBytes: [16]u8 = undefined;
    let i: usize = 0;
    while (i < 16) {
        dummyBytes[i] = 0 as u8;
        i += 1;
    }
    let tmp = Ulid{ bytes: dummyBytes };
    return decode(s, &tmp);
}

/// Monotonically increment the 80-bit random portion of `u` within the same millisecond.
/// Returns `false` if all 80 entropy bits are `0xFF` (overflow).
pub fn increment(u: Ulid, out: *Ulid): bool {
    let next: [16]u8 = undefined;
    let ci: usize = 0;
    while (ci < 16) {
        next[ci] = u.bytes[ci];
        ci += 1;
    }

    let idx: usize = 16;
    while (idx > 6) {
        idx -= 1;
        if (next[idx] < (0xFF as u8)) {
            next[idx] = next[idx] + (1 as u8);
            out.* = Ulid{ bytes: next };
            return true;
        }
        next[idx] = 0 as u8;
    }
    return false;
}

/// Unsigned 16-byte lexicographical comparison between two ULIDs (`-1`, `0`, `1`).
pub fn compare(a: Ulid, b: Ulid): i32 {
    let i: usize = 0;
    while (i < 16) {
        if (a.bytes[i] < b.bytes[i]) {
            return -1 as i32;
        }
        if (a.bytes[i] > b.bytes[i]) {
            return 1 as i32;
        }
        i += 1;
    }
    return 0 as i32;
}

export {
    Ulid,
    fromParts,
    timestampMs,
    encode,
    decode,
    isValidUlid,
    increment,
    compare,
};
