// packages/@cap/base32-crockford/src/index.cp
// Zero-allocation Douglas Crockford Base32 codec (u64 + mod-37 check + bytes + grouping).
// MIT clean-room pure CP implementation.
// Accelerated with 16-lane @Vector(16, u8) SIMD hyphen scanning, comptime 256-entry LUTs,
// 5-byte/8-char bulk encoding, and @memcpy grouping.

type BcVec16 = @Vector(16, u8);

const BC_ALPHA32: string = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
const BC_ALPHA37: string = "0123456789ABCDEFGHJKMNPQRSTVWXYZ*~$=U";

fn bcBuildDecode32Lut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let tbl: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        tbl[i] = 0xFF as u8;
        i += 1;
    }
    let d: usize = 0;
    while (d < 10) {
        tbl[48 + d] = d as u8; // '0'..'9'
        d += 1;
    }
    // 'A'..'H' / 'a'..'h' -> 10..17
    let k: usize = 0;
    while (k < 8) {
        tbl[65 + k] = (10 + k) as u8;
        tbl[97 + k] = (10 + k) as u8;
        k += 1;
    }
    // 'O'/'o' -> 0, 'I'/'i'/'L'/'l' -> 1
    tbl[79] = 0 as u8; tbl[111] = 0 as u8;
    tbl[73] = 1 as u8; tbl[105] = 1 as u8;
    tbl[76] = 1 as u8; tbl[108] = 1 as u8;
    // 'J'->18, 'K'->19, 'M'->20, 'N'->21
    tbl[74] = 18 as u8; tbl[106] = 18 as u8;
    tbl[75] = 19 as u8; tbl[107] = 19 as u8;
    tbl[77] = 20 as u8; tbl[109] = 20 as u8;
    tbl[78] = 21 as u8; tbl[110] = 21 as u8;
    // 'P'..'T' / 'p'..'t' -> 22..26
    k = 0;
    while (k < 5) {
        tbl[80 + k] = (22 + k) as u8;
        tbl[112 + k] = (22 + k) as u8;
        k += 1;
    }
    // 'V'..'Z' / 'v'..'z' -> 27..31
    k = 0;
    while (k < 5) {
        tbl[86 + k] = (27 + k) as u8;
        tbl[118 + k] = (27 + k) as u8;
        k += 1;
    }
    return tbl;
}

fn bcBuildDecode37Lut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let tbl = bcBuildDecode32Lut();
    tbl[42] = 32 as u8;  // '*'
    tbl[126] = 33 as u8; // '~'
    tbl[36] = 34 as u8;  // '$'
    tbl[61] = 35 as u8;  // '='
    tbl[85] = 36 as u8;  // 'U'
    tbl[117] = 36 as u8; // 'u'
    return tbl;
}

const BC_DEC32_LUT: [256]u8 = bcBuildDecode32Lut();
const BC_DEC37_LUT: [256]u8 = bcBuildDecode37Lut();

fn bcEncodeDigit32(v: u8): u8 {
    return BC_ALPHA32[(v & (0x1F as u8)) as usize];
}

fn bcEncodeCheck37(v: u8): u8 {
    return BC_ALPHA37[v as usize];
}

/// Decode a Crockford Base32 data symbol (0..31), case-insensitive, `I/L -> 1`, `O -> 0`.
/// Returns `0xFF` on invalid character (O(1) comptime LUT).
fn bcDecodeDigit32(c: u8): u8 {
    return BC_DEC32_LUT[c as usize];
}

/// Decode a Crockford Base32 mod-37 check symbol (0..36).
/// Returns `0xFF` on invalid character (O(1) comptime LUT).
fn bcDecodeCheck37(c: u8): u8 {
    return BC_DEC37_LUT[c as usize];
}

/// Encode an unsigned 64-bit integer in Crockford Base32 into `out`.
/// When `with_check` is true, appends a mod-37 check symbol (`0..Z`, `*`, `~`, `$`, `=`, `U`).
/// Returns bytes written, or `0` if `out` is too small.
pub fn encodeU64(val: u64, with_check: bool, out: []u8): usize {
    let rev: [16]u8 = undefined;
    let n: usize = 0;

    if (val == (0 as u64)) {
        rev[0] = '0' as u8;
        n = 1;
    } else {
        let cur = val;
        while (cur > (0 as u64)) {
            rev[n] = bcEncodeDigit32((cur & (0x1F as u64)) as u8);
            n += 1;
            cur = cur >> (5 as u6);
        }
    }

    let total = n;
    if (with_check) {
        total += 1;
    }
    if (out.len < total) {
        return 0;
    }

    let i: usize = 0;
    while (i < n) {
        out[i] = rev[n - 1 - i];
        i += 1;
    }
    if (with_check) {
        const chk = (val % (37 as u64)) as u8;
        out[n] = bcEncodeCheck37(chk);
    }
    return total;
}

/// Decode a Crockford Base32 string into an unsigned 64-bit integer in `out_val.*`.
/// Ignores `-`, is case-insensitive, maps `I`/`L` -> `1` and `O` -> `0`.
/// When `with_check` is true, validates the trailing mod-37 check symbol.
/// Returns `false` on invalid characters, failed checksum, or 64-bit overflow.
pub fn decodeU64(input: string, with_check: bool, out_val: *u64): bool {
    // Find last non-hyphen index and count non-hyphen symbols using 16-lane SIMD.
    let nonHyphenCount: usize = 0;
    let lastNonHyphen: usize = 0;
    let scan: usize = 0;
    const v_dash: BcVec16 = @splat(45 as u8);
    while (scan + 16 <= input.len) {
        const v: BcVec16 = input[scan .. scan + 16][0..16].*;
        if (!@reduce(.Or, v == v_dash)) {
            nonHyphenCount += 16;
            lastNonHyphen = scan + 15;
            scan += 16;
            continue;
        }
        const blk_end = scan + 16;
        while (scan < blk_end) {
            if (input[scan] != ('-' as u8)) {
                nonHyphenCount += 1;
                lastNonHyphen = scan;
            }
            scan += 1;
        }
    }
    while (scan < input.len) {
        if (input[scan] != ('-' as u8)) {
            nonHyphenCount += 1;
            lastNonHyphen = scan;
        }
        scan += 1;
    }

    if (with_check) {
        if (nonHyphenCount < 2) {
            return false;
        }
    } else {
        if (nonHyphenCount == 0) {
            return false;
        }
    }

    let expectedCheck: u8 = 0 as u8;
    let dataEnd = input.len;
    if (with_check) {
        expectedCheck = bcDecodeCheck37(input[lastNonHyphen]);
        if (expectedCheck == (0xFF as u8)) {
            return false;
        }
        dataEnd = lastNonHyphen;
    }

    const maxBeforeShift: u64 = (((0x07FFFFFF as u64) << (32 as u6)) | (0xFFFFFFFF as u64));
    let acc: u64 = 0 as u64;
    let i: usize = 0;
    while (i < dataEnd) {
        const c = input[i];
        if (c == ('-' as u8)) {
            i += 1;
            continue;
        }
        const d = bcDecodeDigit32(c);
        if (d == (0xFF as u8)) {
            return false;
        }
        if (acc > maxBeforeShift) {
            return false;
        }
        acc = (acc << (5 as u6)) | (d as u64);
        i += 1;
    }

    if (with_check) {
        if ((acc % (37 as u64)) != (expectedCheck as u64)) {
            return false;
        }
    }

    out_val.* = acc;
    return true;
}

/// Encode an arbitrary byte slice into unpadded Crockford Base32 (5-bit MSB-first chunks).
/// Returns bytes written, or `0` if `data` is empty or `out` is too small.
pub fn encodeBytes(data: []const u8, out: []u8): usize {
    if (data.len == 0) {
        return 0;
    }
    const outLen = (data.len * 8 + 4) / 5;
    if (out.len < outLen) {
        return 0;
    }

    let pos: usize = 0;
    let i: usize = 0;

    // 5-byte -> 8-char bulk encoding fast path
    while (i + 5 <= data.len) {
        const b0 = data[i] as u64;
        const b1 = data[i + 1] as u64;
        const b2 = data[i + 2] as u64;
        const b3 = data[i + 3] as u64;
        const b4 = data[i + 4] as u64;
        const v: u64 = (b0 << (32 as u6)) | (b1 << (24 as u6)) | (b2 << (16 as u6)) | (b3 << (8 as u6)) | b4;

        let blk: [8]u8 = undefined;
        blk[0] = BC_ALPHA32[((v >> (35 as u6)) & (0x1F as u64)) as usize];
        blk[1] = BC_ALPHA32[((v >> (30 as u6)) & (0x1F as u64)) as usize];
        blk[2] = BC_ALPHA32[((v >> (25 as u6)) & (0x1F as u64)) as usize];
        blk[3] = BC_ALPHA32[((v >> (20 as u6)) & (0x1F as u64)) as usize];
        blk[4] = BC_ALPHA32[((v >> (15 as u6)) & (0x1F as u64)) as usize];
        blk[5] = BC_ALPHA32[((v >> (10 as u6)) & (0x1F as u64)) as usize];
        blk[6] = BC_ALPHA32[((v >> (5 as u6)) & (0x1F as u64)) as usize];
        blk[7] = BC_ALPHA32[(v & (0x1F as u64)) as usize];
        @memcpy(out[pos .. pos + 8], blk[0..8]);
        pos += 8;
        i += 5;
    }

    let bitBuf: u32 = 0 as u32;
    let bits: u32 = 0 as u32;
    while (i < data.len) {
        bitBuf = (bitBuf << (8 as u5)) | (data[i] as u32);
        bits += 8 as u32;
        while (bits >= (5 as u32)) {
            bits -= 5 as u32;
            const idx = ((bitBuf >> (bits as u5)) & (0x1F as u32)) as u8;
            out[pos] = bcEncodeDigit32(idx);
            pos += 1;
        }
        i += 1;
    }

    if (bits > (0 as u32)) {
        const shift = (5 as u32) - bits;
        const idx = ((bitBuf << (shift as u5)) & (0x1F as u32)) as u8;
        out[pos] = bcEncodeDigit32(idx);
        pos += 1;
    }

    return pos;
}

/// Decode an unpadded Crockford Base32 string (ignoring `-`, case-insensitive, `I/L->1`, `O->0`)
/// back into raw bytes in `out`. Returns bytes written, or `-1` on invalid input or insufficient `out`.
pub fn decodeBytes(input: string, out: []u8): i64 {
    let bitBuf: u32 = 0 as u32;
    let bits: u32 = 0 as u32;
    let pos: usize = 0;
    let i: usize = 0;
    const v_dash: BcVec16 = @splat(45 as u8);

    // 16-byte SIMD hyphen-free -> 10-byte bulk decode fast path (when bit-aligned)
    while (bits == (0 as u32) && i + 16 <= input.len && pos + 10 <= out.len) {
        const v: BcVec16 = input[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_dash)) {
            break;
        }
        const d0 = BC_DEC32_LUT[input[i] as usize];
        const d1 = BC_DEC32_LUT[input[i + 1] as usize];
        const d2 = BC_DEC32_LUT[input[i + 2] as usize];
        const d3 = BC_DEC32_LUT[input[i + 3] as usize];
        const d4 = BC_DEC32_LUT[input[i + 4] as usize];
        const d5 = BC_DEC32_LUT[input[i + 5] as usize];
        const d6 = BC_DEC32_LUT[input[i + 6] as usize];
        const d7 = BC_DEC32_LUT[input[i + 7] as usize];
        const e0 = BC_DEC32_LUT[input[i + 8] as usize];
        const e1 = BC_DEC32_LUT[input[i + 9] as usize];
        const e2 = BC_DEC32_LUT[input[i + 10] as usize];
        const e3 = BC_DEC32_LUT[input[i + 11] as usize];
        const e4 = BC_DEC32_LUT[input[i + 12] as usize];
        const e5 = BC_DEC32_LUT[input[i + 13] as usize];
        const e6 = BC_DEC32_LUT[input[i + 14] as usize];
        const e7 = BC_DEC32_LUT[input[i + 15] as usize];

        if (((d0 | d1 | d2 | d3 | d4 | d5 | d6 | d7 | e0 | e1 | e2 | e3 | e4 | e5 | e6 | e7) & (0x80 as u8)) != (0 as u8)) {
            return -1 as i64;
        }

        const hi: u64 = ((d0 as u64) << (35 as u6))
            | ((d1 as u64) << (30 as u6))
            | ((d2 as u64) << (25 as u6))
            | ((d3 as u64) << (20 as u6))
            | ((d4 as u64) << (15 as u6))
            | ((d5 as u64) << (10 as u6))
            | ((d6 as u64) << (5 as u6))
            | (d7 as u64);
        const lo: u64 = ((e0 as u64) << (35 as u6))
            | ((e1 as u64) << (30 as u6))
            | ((e2 as u64) << (25 as u6))
            | ((e3 as u64) << (20 as u6))
            | ((e4 as u64) << (15 as u6))
            | ((e5 as u64) << (10 as u6))
            | ((e6 as u64) << (5 as u6))
            | (e7 as u64);

        let blk: [10]u8 = undefined;
        blk[0] = ((hi >> (32 as u6)) & (0xFF as u64)) as u8;
        blk[1] = ((hi >> (24 as u6)) & (0xFF as u64)) as u8;
        blk[2] = ((hi >> (16 as u6)) & (0xFF as u64)) as u8;
        blk[3] = ((hi >> (8 as u6)) & (0xFF as u64)) as u8;
        blk[4] = (hi & (0xFF as u64)) as u8;
        blk[5] = ((lo >> (32 as u6)) & (0xFF as u64)) as u8;
        blk[6] = ((lo >> (24 as u6)) & (0xFF as u64)) as u8;
        blk[7] = ((lo >> (16 as u6)) & (0xFF as u64)) as u8;
        blk[8] = ((lo >> (8 as u6)) & (0xFF as u64)) as u8;
        blk[9] = (lo & (0xFF as u64)) as u8;
        @memcpy(out[pos .. pos + 10], blk[0..10]);
        pos += 10;
        i += 16;
    }

    while (i < input.len) {
        const c = input[i];
        if (c == ('-' as u8)) {
            i += 1;
            continue;
        }
        const d = bcDecodeDigit32(c);
        if (d == (0xFF as u8)) {
            return -1 as i64;
        }
        bitBuf = (bitBuf << (5 as u5)) | (d as u32);
        bits += 5 as u32;
        if (bits >= (8 as u32)) {
            bits -= 8 as u32;
            if (pos >= out.len) {
                return -1 as i64;
            }
            out[pos] = ((bitBuf >> (bits as u5)) & (0xFF as u32)) as u8;
            pos += 1;
        }
        i += 1;
    }

    if (bits > (0 as u32)) {
        const mask = ((1 as u32) << (bits as u5)) - (1 as u32);
        if ((bitBuf & mask) != (0 as u32)) {
            return -1 as i64;
        }
    }

    return pos as i64;
}

/// Insert `'-'` every `group_size` characters (e.g. `"ABCDEFGH"` with `4` -> `"ABCD-EFGH"`).
/// Returns bytes written, or `0` if `raw` is empty or `out` is too small.
pub fn formatGrouped(raw: string, group_size: usize, out: []u8): usize {
    if (raw.len == 0) {
        return 0;
    }
    if (group_size == 0) {
        if (out.len < raw.len) {
            return 0;
        }
        @memcpy(out[0..raw.len], raw[0..raw.len]);
        return raw.len;
    }

    const seps = (raw.len - 1) / group_size;
    const total = raw.len + seps;
    if (out.len < total) {
        return 0;
    }

    let pos: usize = 0;
    let i: usize = 0;
    while (i < raw.len) {
        if (i > 0) {
            out[pos] = '-' as u8;
            pos += 1;
        }
        const rem = raw.len - i;
        const chunk = if (rem < group_size) rem else group_size;
        @memcpy(out[pos .. pos + chunk], raw[i .. i + chunk]);
        pos += chunk;
        i += chunk;
    }
    return pos;
}

export {
    encodeU64,
    decodeU64,
    encodeBytes,
    decodeBytes,
    formatGrouped,
};
