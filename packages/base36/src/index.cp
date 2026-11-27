// @cap/base36 — Zero-heap Base36 & arbitrary radix 2..36 u64 codec
// MIT License (clean-room CP implementation)
// Accelerated with comptime 256-entry decode LUT, 16-lane @Vector(16, u8) SIMD
// leading-zero scanning, @memcpy, and uninitialized stack workspaces.

type B36Vec16 = @Vector(16, u8);

const B36_DIGITS_LOWER: string = "0123456789abcdefghijklmnopqrstuvwxyz";
const B36_DIGITS_UPPER: string = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

fn b36BuildDecLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [255; 256];
    let c: usize = 0;
    while (c < 256) {
        if (c >= 48 && c <= 57) {
            lut[c] = (c - 48) as u8;
        } else if (c >= 97 && c <= 122) {
            lut[c] = (c - 97 + 10) as u8;
        } else if (c >= 65 && c <= 90) {
            lut[c] = (c - 65 + 10) as u8;
        }
        c += 1;
    }
    return lut;
}

const B36_DEC_LUT: [256]u8 = b36BuildDecLut();

fn b36CountLeading(data: []const u8, target: u8) -> usize {
    let i: usize = 0;
    const len = data.len;
    const vt: B36Vec16 = @splat(target);
    while (i + 16 <= len) {
        const v: B36Vec16 = data[i .. i + 16][0..16].*;
        if (@reduce(.Or, v != vt)) {
            let k: usize = 0;
            while (k < 16 && data[i + k] == target) {
                k += 1;
            }
            return i + k;
        }
        i += 16;
    }
    while (i < len && data[i] == target) {
        i += 1;
    }
    return i;
}

pub struct ParseU64Result {
    pub value: u64,
    pub valid: bool,
}

fn digitChar(v: u8, uppercase: bool) -> u8 {
    if (uppercase) {
        return B36_DIGITS_UPPER[v as usize];
    }
    return B36_DIGITS_LOWER[v as usize];
}

pub fn encodeRadix(val: u64, radix: u8, uppercase: bool, out: []u8) -> usize {
    if (radix < 2 || radix > 36) {
        return 0;
    }
    let digits: [64]u8 = undefined;
    let count: usize = 0;
    const base: u64 = radix as u64;

    if (val == 0) {
        digits[0] = '0' as u8;
        count = 1;
    } else {
        let cur: u64 = val;
        while (cur > 0) {
            const rem: u8 = (cur % base) as u8;
            digits[count] = digitChar(rem, uppercase);
            count += 1;
            cur = cur / base;
        }
    }

    if (count > out.len) {
        return 0;
    }
    let pos: usize = 0;
    let k: usize = count;
    while (k > 0) {
        k -= 1;
        out[pos] = digits[k];
        pos += 1;
    }
    return pos;
}

pub fn decodeRadix(input: string, radix: u8) -> ParseU64Result {
    const fail = ParseU64Result { value: 0, valid: false };
    if (radix < 2 || radix > 36 || input.len == 0) {
        return fail;
    }

    const max_u64: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
    const base: u64 = radix as u64;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < input.len) {
        const d_u8: u8 = B36_DEC_LUT[input[i] as usize];
        if (d_u8 >= radix) {
            return fail;
        }
        const d: u64 = d_u8 as u64;
        if (acc > (max_u64 - d) / base) {
            return fail;
        }
        acc = acc * base + d;
        i += 1;
    }
    return ParseU64Result { value: acc, valid: true };
}

pub fn encodeBase36(val: u64, out: []u8) -> usize {
    return encodeRadix(val, 36, false, out);
}

pub fn decodeBase36(input: string) -> ParseU64Result {
    return decodeRadix(input, 36);
}

pub fn isValidBase36(input: string) -> bool {
    return decodeRadix(input, 36).valid;
}

pub fn encodeU64(val: u64, out: []u8) -> usize {
    return encodeBase36(val, out);
}

pub fn encodeU64Upper(val: u64, out: []u8) -> usize {
    return encodeRadix(val, 36, true, out);
}

pub fn decodeU64(input: string) -> ParseU64Result {
    return decodeBase36(input);
}

pub fn encodedLenU64(val: u64, radix: u8) -> usize {
    if (radix < 2 || radix > 36) {
        return 0;
    }
    if (val == 0) {
        return 1;
    }
    const base: u64 = radix as u64;
    let cur: u64 = val;
    let count: usize = 0;
    while (cur > 0) {
        count += 1;
        cur = cur / base;
    }
    return count;
}

/// Returns true if `input` is non-empty and every character is a valid Base36 digit (`0-9`, `a-z`, `A-Z`),
/// regardless of whether the value fits in `u64`.
pub fn isValidBase36String(input: string) -> bool {
    if (input.len == 0) {
        return false;
    }
    let i: usize = 0;
    const len = input.len;
    while (i + 4 <= len) {
        if (((B36_DEC_LUT[input[i] as usize] |
            B36_DEC_LUT[input[i + 1] as usize] |
            B36_DEC_LUT[input[i + 2] as usize] |
            B36_DEC_LUT[input[i + 3] as usize]) & 0x80) != 0)
        {
            return false;
        }
        i += 4;
    }
    while (i < len) {
        if ((B36_DEC_LUT[input[i] as usize] & 0x80) != 0) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub struct DecodeBytesResult {
    pub len: usize,
    pub valid: bool,
}

/// Encodes an arbitrary big-endian byte slice into Base36 ASCII characters (preserving leading `0x00` bytes as `'0'`).
/// Supports inputs up to 256 bytes without heap allocation. Returns bytes written, or `0` on empty input or overflow.
pub fn encodeBytes(data: []const u8, uppercase: bool, out: []u8) -> usize {
    if (data.len == 0 || data.len > 256) {
        return 0;
    }

    const leading_zeros: usize = b36CountLeading(data, 0);

    if (leading_zeros == data.len) {
        if (leading_zeros > out.len) {
            return 0;
        }
        let z: usize = 0;
        while (z < leading_zeros) {
            out[z] = '0' as u8;
            z += 1;
        }
        return leading_zeros;
    }

    // Copy non-zero tail into working buffer for repeated base-36 division
    let work: [256]u8 = undefined;
    const work_len: usize = data.len - leading_zeros;
    @memcpy(work[0..work_len], data[leading_zeros..data.len]);

    // Max base36 digits for 256 bytes: ceil(256 * log(256)/log(36)) = 397
    let rev: [512]u8 = undefined;
    let rev_len: usize = 0;
    let start: usize = 0;
    while (start < work_len) {
        let rem: u32 = 0;
        let idx: usize = start;
        let new_start: usize = work_len;
        while (idx < work_len) {
            const acc: u32 = (rem << 8) | (work[idx] as u32);
            const q: u8 = (acc / 36) as u8;
            rem = acc % 36;
            work[idx] = q;
            if (q != 0 && new_start == work_len) {
                new_start = idx;
            }
            idx += 1;
        }
        if (rev_len >= 512) {
            return 0;
        }
        rev[rev_len] = digitChar(rem as u8, uppercase);
        rev_len += 1;
        start = new_start;
    }

    const total: usize = leading_zeros + rev_len;
    if (total > out.len) {
        return 0;
    }

    let pos: usize = 0;
    let z2: usize = 0;
    while (z2 < leading_zeros) {
        out[pos] = '0' as u8;
        pos += 1;
        z2 += 1;
    }
    let k: usize = rev_len;
    while (k > 0) {
        k -= 1;
        out[pos] = rev[k];
        pos += 1;
    }
    return pos;
}

/// Decodes a case-insensitive Base36 string into big-endian bytes (preserving leading `'0'`s as `0x00`).
/// Supports decoded outputs up to 256 bytes without heap allocation.
pub fn decodeBytes(input: string, out: []u8) -> DecodeBytesResult {
    const fail = DecodeBytesResult { len: 0, valid: false };
    if (input.len == 0) {
        return fail;
    }

    const leading_zeros: usize = b36CountLeading(input, '0' as u8);

    if (leading_zeros == input.len) {
        if (leading_zeros > out.len) {
            return fail;
        }
        let z: usize = 0;
        while (z < leading_zeros) {
            out[z] = 0;
            z += 1;
        }
        return DecodeBytesResult { len: leading_zeros, valid: true };
    }

    // Little-endian base-256 accumulator (uninitialized: only 0..acc_len is read/written)
    let acc: [256]u8 = undefined;
    let acc_len: usize = 0;
    let i: usize = leading_zeros;
    while (i < input.len) {
        const d_u8: u8 = B36_DEC_LUT[input[i] as usize];
        if ((d_u8 & 0x80) != 0) {
            return fail;
        }
        let carry: u32 = d_u8 as u32;
        let j: usize = 0;
        while (j < acc_len) {
            const val: u32 = (acc[j] as u32) * 36 + carry;
            acc[j] = (val & 0xFF) as u8;
            carry = val >> 8;
            j += 1;
        }
        while (carry > 0) {
            if (acc_len >= 256) {
                return fail;
            }
            acc[acc_len] = (carry & 0xFF) as u8;
            acc_len += 1;
            carry = carry >> 8;
        }
        i += 1;
    }

    const total: usize = leading_zeros + acc_len;
    if (total > out.len) {
        return fail;
    }

    let pos: usize = 0;
    let z2: usize = 0;
    while (z2 < leading_zeros) {
        out[pos] = 0;
        pos += 1;
        z2 += 1;
    }
    let k: usize = acc_len;
    while (k > 0) {
        k -= 1;
        out[pos] = acc[k];
        pos += 1;
    }
    return DecodeBytesResult { len: pos, valid: true };
}
