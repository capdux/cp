// @cap/leb128 — Zero-heap ULEB128 & SLEB128 variable-length integer codec
// Clean-room MIT implementation for CP (compatible with Rust leb128 / WebAssembly / DWARF).

pub struct UlebResult {
    pub value: u64,
    pub consumed: usize,
    pub valid: bool,
}

pub struct SlebResult {
    pub value: i64,
    pub consumed: usize,
    pub valid: bool,
}

/// Encode an unsigned 64-bit integer as ULEB128 into `out` (1..10 bytes).
/// Returns bytes written, or 0 if `out` is too small.
pub fn encodeUnsigned(val: u64, out: []u8): usize {
    let v: u64 = val;
    let pos: usize = 0;
    while (true) {
        if (pos >= out.len) { return 0; }
        let b: u8 = (v & 0x7f) as u8;
        v = v >> 7;
        if (v != 0) {
            out[pos] = b | 0x80;
            pos += 1;
        } else {
            out[pos] = b;
            pos += 1;
            return pos;
        }
    }
    return pos;
}

/// Decode an unsigned ULEB128 integer from `data[start..]`.
/// Rejects truncated streams and 64-bit overflow (> 10 bytes or 10th byte > 0x01).
pub fn decodeUnsigned(data: []const u8, start: usize): UlebResult {
    let invalid: UlebResult = UlebResult { value: 0, consumed: 0, valid: false };
    if (start >= data.len) { return invalid; }

    let result: u64 = 0;
    let shift: u6 = 0;
    let i: usize = start;
    let step: usize = 0;

    while (i < data.len) {
        let b: u8 = data[i];
        i += 1;
        step += 1;

        if (step == 10 && b > 0x01) {
            return invalid;
        }
        result = result | (((b & 0x7f) as u64) << shift);
        if ((b & 0x80) == 0) {
            return UlebResult { value: result, consumed: i - start, valid: true };
        }
        if (step >= 10) {
            return invalid;
        }
        shift += 7;
    }
    return invalid;
}

/// Encode a signed 64-bit integer as SLEB128 into `out` (1..10 bytes).
/// Returns bytes written, or 0 if `out` is too small.
pub fn encodeSigned(val: i64, out: []u8): usize {
    let v: i64 = val;
    let pos: usize = 0;
    while (true) {
        if (pos >= out.len) { return 0; }
        let b: u8 = (v & 0x7f) as u8;
        v = v >> 7;
        let sign_bit: bool = (b & 0x40) != 0;
        let done: bool = (v == 0 && !sign_bit) || (v == -1 && sign_bit);
        if (done) {
            out[pos] = b;
            pos += 1;
            return pos;
        }
        out[pos] = b | 0x80;
        pos += 1;
    }
    return pos;
}

/// Decode a signed SLEB128 integer from `data[start..]`.
/// Rejects truncated streams and 64-bit signed overflow.
pub fn decodeSigned(data: []const u8, start: usize): SlebResult {
    let invalid: SlebResult = SlebResult { value: 0, consumed: 0, valid: false };
    if (start >= data.len) { return invalid; }

    let result: u64 = 0;
    let shift: u6 = 0;
    let i: usize = start;
    let step: usize = 0;

    while (i < data.len) {
        let b: u8 = data[i];
        i += 1;
        step += 1;

        if (step == 10) {
            // For 10th byte of i64 SLEB128, valid non-continuation values are 0x00 (positive) or 0x7f (negative)
            if (b != 0x00 && b != 0x7f) {
                return invalid;
            }
        }
        result = result | (((b & 0x7f) as u64) << shift);
        if ((b & 0x80) == 0) {
            if (step < 10 && (b & 0x40) != 0) {
                let sign_shift: u6 = shift + 7;
                let all_ones: u64 = ((0xffffffff as u64) << 32) | (0xffffffff as u64);
                result = result | (all_ones << sign_shift);
            }
            return SlebResult { value: result as i64, consumed: i - start, valid: true };
        }
        if (step >= 10) {
            return invalid;
        }
        shift += 7;
    }
    return invalid;
}

/// Exact encoded byte length of `val` in ULEB128 (1..10).
pub fn unsignedEncodedLen(val: u64): usize {
    let v: u64 = val;
    let n: usize = 1;
    while (v >= 0x80) {
        v = v >> 7;
        n += 1;
    }
    return n;
}
