// @cap/itoa — Fast zero-heap two-digits-per-step integer-to-ASCII formatter
// Clean-room MIT CP implementation of crates.io `itoa` (1.7B+ downloads)
// Uses a 200-byte two-digit lookup table to halve integer divisions, handles i64::MIN safely.

const IT_DEC_DIGITS_LUT: string = "00010203040506070809101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263646566676869707172737475767778798081828384858687888990919293949596979899";

const IT_HEX_LOWER: string = "0123456789abcdef";
const IT_HEX_UPPER: string = "0123456789ABCDEF";

pub const ITOA_MAX_U64_LEN: usize = 20;
pub const ITOA_MAX_I64_LEN: usize = 20;

/// Formats `val: u64` as decimal ASCII into `out` using a 2-digits-per-division LUT.
/// Returns written byte count (0 if `out` is too small).
pub fn formatU64(val: u64, out: []u8): usize {
    let tmp: [24]u8 = undefined;
    let pos: usize = 24;
    let n = val;

    while (n >= 100) {
        let rem = (n % 100) as usize;
        n = n / 100;
        pos = pos - 2;
        tmp[pos] = IT_DEC_DIGITS_LUT[rem * 2];
        tmp[pos + 1] = IT_DEC_DIGITS_LUT[rem * 2 + 1];
    }
    if (n >= 10) {
        let rem = n as usize;
        pos = pos - 2;
        tmp[pos] = IT_DEC_DIGITS_LUT[rem * 2];
        tmp[pos + 1] = IT_DEC_DIGITS_LUT[rem * 2 + 1];
    } else {
        pos = pos - 1;
        tmp[pos] = 48 + (n as u8);
    }

    let len = 24 - pos;
    if (len > out.len) { return 0; }
    let i: usize = 0;
    while (i < len) {
        out[i] = tmp[pos + i];
        i = i + 1;
    }
    return len;
}

/// Formats `val: i64` as signed decimal ASCII into `out` (safely handles `i64::MIN = -9223372036854775808`).
pub fn formatI64(val: i64, out: []u8): usize {
    if (val >= 0) {
        return formatU64(val as u64, out);
    }
    // Compute magnitude safely without overflowing i64::MIN
    let mag: u64 = (0 -% (val as u64));
    let tmp: [24]u8 = undefined;
    let dlen = formatU64(mag, tmp[0..24]);
    if (dlen + 1 > out.len) { return 0; }
    out[0] = 45; // '-'
    let i: usize = 0;
    while (i < dlen) {
        out[1 + i] = tmp[i];
        i = i + 1;
    }
    return dlen + 1;
}

pub fn formatU32(val: u32, out: []u8): usize {
    return formatU64(val as u64, out);
}

pub fn formatI32(val: i32, out: []u8): usize {
    return formatI64(val as i64, out);
}

pub fn formatUsize(val: usize, out: []u8): usize {
    return formatU64(val as u64, out);
}

/// Formats `val: u64` as hexadecimal ASCII (`uppercase = false` -> lowercase, `true` -> UPPERCASE).
pub fn formatHexU64(val: u64, uppercase: bool, out: []u8): usize {
    let lut = if (uppercase) { IT_HEX_UPPER } else { IT_HEX_LOWER };
    let tmp: [16]u8 = undefined;
    let pos: usize = 16;
    let n = val;
    let done = false;
    while (!done) {
        pos = pos - 1;
        tmp[pos] = lut[(n & 15) as usize];
        n = n >> 4;
        if (n == 0) { done = true; }
    }
    let len = 16 - pos;
    if (len > out.len) { return 0; }
    let i: usize = 0;
    while (i < len) {
        out[i] = tmp[pos + i];
        i = i + 1;
    }
    return len;
}

/// Formats `val: u64` in any base `2..=36` (lowercase digits `0-9a-z`).
pub fn formatRadixU64(val: u64, radix: u8, out: []u8): usize {
    if (radix < 2 || radix > 36) { return 0; }
    let digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    let tmp: [64]u8 = undefined;
    let pos: usize = 64;
    let n = val;
    let base = radix as u64;
    let done = false;
    while (!done) {
        pos = pos - 1;
        tmp[pos] = digits[(n % base) as usize];
        n = n / base;
        if (n == 0) { done = true; }
    }
    let len = 64 - pos;
    if (len > out.len) { return 0; }
    let i: usize = 0;
    while (i < len) {
        out[i] = tmp[pos + i];
        i = i + 1;
    }
    return len;
}
