// packages/@cap/base32-crockford/src/index.cp
// Zero-allocation Douglas Crockford Base32 codec (u64 + mod-37 check + bytes + grouping).
// MIT clean-room pure CP implementation.

fn bcEncodeDigit32(v: u8): u8 {
    const alpha = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
    return alpha[(v & (0x1F as u8)) as usize];
}

fn bcEncodeCheck37(v: u8): u8 {
    const alpha37 = "0123456789ABCDEFGHJKMNPQRSTVWXYZ*~$=U";
    return alpha37[(v as usize)];
}

/// Decode a Crockford Base32 data symbol (0..31), case-insensitive, `I/L -> 1`, `O -> 0`.
/// Returns `0xFF` on invalid character.
fn bcDecodeDigit32(c: u8): u8 {
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

/// Decode a Crockford Base32 mod-37 check symbol (0..36).
/// Returns `0xFF` on invalid character.
fn bcDecodeCheck37(c: u8): u8 {
    const d32 = bcDecodeDigit32(c);
    if (d32 != (0xFF as u8)) {
        return d32;
    }
    if (c == ('*' as u8)) { return 32 as u8; }
    if (c == ('~' as u8)) { return 33 as u8; }
    if (c == ('$' as u8)) { return 34 as u8; }
    if (c == ('=' as u8)) { return 35 as u8; }
    if (c == ('U' as u8) || c == ('u' as u8)) { return 36 as u8; }
    return 0xFF as u8;
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
    // Find last non-hyphen index and count non-hyphen symbols.
    let nonHyphenCount: usize = 0;
    let lastNonHyphen: usize = 0;
    let scan: usize = 0;
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

    let bitBuf: u32 = 0 as u32;
    let bits: u32 = 0 as u32;
    let pos: usize = 0;
    let i: usize = 0;

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
        let ci: usize = 0;
        while (ci < raw.len) {
            out[ci] = raw[ci];
            ci += 1;
        }
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
        if (i > 0 && (i % group_size) == 0) {
            out[pos] = '-' as u8;
            pos += 1;
        }
        out[pos] = raw[i];
        pos += 1;
        i += 1;
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
