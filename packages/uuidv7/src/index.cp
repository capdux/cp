// @cap/uuid-v7 — Zero-allocation RFC 9562 UUIDv7 Engine
// Clean-room MIT implementation in pure CP.
// Accelerated with 16-lane @Vector(16, u8) SIMD hex format/parse/compare, comptime 256-entry hex LUT, @memcpy, and undefined stack buffers.

type Uuid7Vec16 = @Vector(16, u8);
type Uuid7Shift16 = @Vector(16, u3);

fn uuid7BuildHexDecLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        lut[i] = 0xFF;
        i += 1;
    }
    let d: usize = 0;
    while (d < 10) {
        lut[48 + d] = d as u8; // 0-9
        d += 1;
    }
    let a: usize = 0;
    while (a < 6) {
        lut[65 + a] = (10 + a) as u8; // A-F
        lut[97 + a] = (10 + a) as u8; // a-f
        a += 1;
    }
    return lut;
}

const UUID7_HEX_DEC_LUT: [256]u8 = uuid7BuildHexDecLut();

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
    @memcpy(b[9..16], rand_bytes[3..10]);

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
    const vb: Uuid7Vec16 = u.bytes;
    const s4: Uuid7Shift16 = @splat(4 as u3);
    const v_mask: Uuid7Vec16 = @splat(0x0F as u8);
    const v_48: Uuid7Vec16 = @splat(48 as u8);
    const v_10: Uuid7Vec16 = @splat(10 as u8);
    const v_39: Uuid7Vec16 = @splat(39 as u8);
    const v_zero: Uuid7Vec16 = @splat(0 as u8);

    const v_hi: Uuid7Vec16 = vb >> s4;
    const v_lo: Uuid7Vec16 = vb & v_mask;
    const hi_ascii: [16]u8 = v_hi + v_48 + @select(u8, v_hi >= v_10, v_39, v_zero);
    const lo_ascii: [16]u8 = v_lo + v_48 + @select(u8, v_lo >= v_10, v_39, v_zero);

    let tmp: [36]u8 = undefined;
    // 8 chars (bytes 0..4)
    tmp[0] = hi_ascii[0]; tmp[1] = lo_ascii[0];
    tmp[2] = hi_ascii[1]; tmp[3] = lo_ascii[1];
    tmp[4] = hi_ascii[2]; tmp[5] = lo_ascii[2];
    tmp[6] = hi_ascii[3]; tmp[7] = lo_ascii[3];
    tmp[8] = 45;
    // 4 chars (bytes 4..6)
    tmp[9] = hi_ascii[4]; tmp[10] = lo_ascii[4];
    tmp[11] = hi_ascii[5]; tmp[12] = lo_ascii[5];
    tmp[13] = 45;
    // 4 chars (bytes 6..8)
    tmp[14] = hi_ascii[6]; tmp[15] = lo_ascii[6];
    tmp[16] = hi_ascii[7]; tmp[17] = lo_ascii[7];
    tmp[18] = 45;
    // 4 chars (bytes 8..10)
    tmp[19] = hi_ascii[8]; tmp[20] = lo_ascii[8];
    tmp[21] = hi_ascii[9]; tmp[22] = lo_ascii[9];
    tmp[23] = 45;
    // 12 chars (bytes 10..16)
    tmp[24] = hi_ascii[10]; tmp[25] = lo_ascii[10];
    tmp[26] = hi_ascii[11]; tmp[27] = lo_ascii[11];
    tmp[28] = hi_ascii[12]; tmp[29] = lo_ascii[12];
    tmp[30] = hi_ascii[13]; tmp[31] = lo_ascii[13];
    tmp[32] = hi_ascii[14]; tmp[33] = lo_ascii[14];
    tmp[34] = hi_ascii[15]; tmp[35] = lo_ascii[15];

    @memcpy(out[0..36], tmp[0..36]);
    return 36;
}

/// Parses a 36-char hyphenated or 32-char compact hex UUIDv7 (case-insensitive) into `out`.
/// Enforces `version == 7` and RFC 9562 variant (`10xx`). Returns true on success.
pub fn parse(s: string, out: *UuidV7) -> bool {
    let hi_arr: [16]u8 = undefined;
    let lo_arr: [16]u8 = undefined;

    if (s.len == 36) {
        if (s[8] != 45 || s[13] != 45 || s[18] != 45 || s[23] != 45) {
            return false;
        }
        hi_arr[0] = UUID7_HEX_DEC_LUT[s[0] as usize]; lo_arr[0] = UUID7_HEX_DEC_LUT[s[1] as usize];
        hi_arr[1] = UUID7_HEX_DEC_LUT[s[2] as usize]; lo_arr[1] = UUID7_HEX_DEC_LUT[s[3] as usize];
        hi_arr[2] = UUID7_HEX_DEC_LUT[s[4] as usize]; lo_arr[2] = UUID7_HEX_DEC_LUT[s[5] as usize];
        hi_arr[3] = UUID7_HEX_DEC_LUT[s[6] as usize]; lo_arr[3] = UUID7_HEX_DEC_LUT[s[7] as usize];
        hi_arr[4] = UUID7_HEX_DEC_LUT[s[9] as usize]; lo_arr[4] = UUID7_HEX_DEC_LUT[s[10] as usize];
        hi_arr[5] = UUID7_HEX_DEC_LUT[s[11] as usize]; lo_arr[5] = UUID7_HEX_DEC_LUT[s[12] as usize];
        hi_arr[6] = UUID7_HEX_DEC_LUT[s[14] as usize]; lo_arr[6] = UUID7_HEX_DEC_LUT[s[15] as usize];
        hi_arr[7] = UUID7_HEX_DEC_LUT[s[16] as usize]; lo_arr[7] = UUID7_HEX_DEC_LUT[s[17] as usize];
        hi_arr[8] = UUID7_HEX_DEC_LUT[s[19] as usize]; lo_arr[8] = UUID7_HEX_DEC_LUT[s[20] as usize];
        hi_arr[9] = UUID7_HEX_DEC_LUT[s[21] as usize]; lo_arr[9] = UUID7_HEX_DEC_LUT[s[22] as usize];
        hi_arr[10] = UUID7_HEX_DEC_LUT[s[24] as usize]; lo_arr[10] = UUID7_HEX_DEC_LUT[s[25] as usize];
        hi_arr[11] = UUID7_HEX_DEC_LUT[s[26] as usize]; lo_arr[11] = UUID7_HEX_DEC_LUT[s[27] as usize];
        hi_arr[12] = UUID7_HEX_DEC_LUT[s[28] as usize]; lo_arr[12] = UUID7_HEX_DEC_LUT[s[29] as usize];
        hi_arr[13] = UUID7_HEX_DEC_LUT[s[30] as usize]; lo_arr[13] = UUID7_HEX_DEC_LUT[s[31] as usize];
        hi_arr[14] = UUID7_HEX_DEC_LUT[s[32] as usize]; lo_arr[14] = UUID7_HEX_DEC_LUT[s[33] as usize];
        hi_arr[15] = UUID7_HEX_DEC_LUT[s[34] as usize]; lo_arr[15] = UUID7_HEX_DEC_LUT[s[35] as usize];
    } else if (s.len == 32) {
        let bi: usize = 0;
        while (bi < 16) {
            hi_arr[bi] = UUID7_HEX_DEC_LUT[s[bi * 2] as usize];
            lo_arr[bi] = UUID7_HEX_DEC_LUT[s[bi * 2 + 1] as usize];
            bi += 1;
        }
    } else {
        return false;
    }

    const vh: Uuid7Vec16 = hi_arr;
    const vl: Uuid7Vec16 = lo_arr;
    const v_inv: Uuid7Vec16 = @splat(16 as u8);
    if (@reduce(.Or, (vh | vl) >= v_inv)) {
        return false;
    }

    const s4: Uuid7Shift16 = @splat(4 as u3);
    const tmp: [16]u8 = (vh << s4) | vl;

    if (((tmp[6] >> 4) & 0x0F) != 7 || (tmp[8] & 0xC0) != 0x80) {
        return false;
    }

    @memcpy(out.bytes[0..16], tmp[0..16]);
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
    const va: Uuid7Vec16 = a.bytes;
    const vb: Uuid7Vec16 = b.bytes;
    if (@reduce(.And, va == vb)) {
        return 0;
    }
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
