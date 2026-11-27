// @cap/base62 — Zero-heap URL-safe Base62 (0-9A-Za-z) Encoder & Decoder
// Clean-room MIT implementation in pure CP (no heap allocations, no FFI).
// Accelerated with comptime 256-entry decode LUT, 16-lane @Vector(16, u8) SIMD
// leading-zero scanning, and @memcpy.

type B62Vec16 = @Vector(16, u8);

const ALPHABET: []const u8 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

fn b62BuildDecLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [255; 256];
    let c: usize = 0;
    while (c < 256) {
        if (c >= 48 && c <= 57) {
            lut[c] = (c - 48) as u8;
        } else if (c >= 65 && c <= 90) {
            lut[c] = (c - 65 + 10) as u8;
        } else if (c >= 97 && c <= 122) {
            lut[c] = (c - 97 + 36) as u8;
        }
        c += 1;
    }
    return lut;
}

const B62_DEC_LUT: [256]u8 = b62BuildDecLut();

fn b62CountLeading(src: []const u8, target: u8): usize {
    let i: usize = 0;
    const len = src.len;
    const vt: B62Vec16 = @splat(target);
    while (i + 16 <= len) {
        const v: B62Vec16 = src[i .. i + 16][0..16].*;
        if (@reduce(.Or, v != vt)) {
            let k: usize = 0;
            while (k < 16 && src[i + k] == target) {
                k += 1;
            }
            return i + k;
        }
        i += 16;
    }
    while (i < len && src[i] == target) {
        i += 1;
    }
    return i;
}

pub struct DecodeU64Result {
    value: u64,
    ok: bool,
}

pub fn isValidBase62(src: []const u8): bool {
    if (src.len == 0) {
        return false;
    }
    let i: usize = 0;
    const len = src.len;
    while (i + 4 <= len) {
        if (((B62_DEC_LUT[src[i] as usize] |
            B62_DEC_LUT[src[i + 1] as usize] |
            B62_DEC_LUT[src[i + 2] as usize] |
            B62_DEC_LUT[src[i + 3] as usize]) & 0x80) != 0)
        {
            return false;
        }
        i += 4;
    }
    while (i < len) {
        if ((B62_DEC_LUT[src[i] as usize] & 0x80) != 0) {
            return false;
        }
        i += 1;
    }
    return true;
}

// Encode a u64 into Base62 (1..11 chars). Returns bytes written, or 0 if dst is too small.
pub fn encodeU64(value: u64, dst: []u8): usize {
    if (value == 0) {
        if (dst.len < 1) {
            return 0;
        }
        dst[0] = '0';
        return 1;
    }
    let tmp: [16]u8 = undefined;
    let n: usize = 0;
    let v: u64 = value;
    while (v > 0) {
        let rem: usize = (v % 62) as usize;
        tmp[n] = ALPHABET[rem];
        n += 1;
        v = v / 62;
    }
    if (dst.len < n) {
        return 0;
    }
    let i: usize = 0;
    while (i < n) {
        dst[i] = tmp[n - 1 - i];
        i += 1;
    }
    return n;
}

// Encode a u64 left-padded with '0' to exact fixed `width` (ideal for lexicographically sortable IDs)
pub fn encodeU64Padded(value: u64, width: usize, dst: []u8): usize {
    if (width == 0 || dst.len < width) {
        return 0;
    }
    let tmp: [16]u8 = undefined;
    let n: usize = encodeU64(value, tmp[0..16]);
    if (n == 0 || n > width) {
        return 0;
    }
    let pad: usize = width - n;
    let i: usize = 0;
    while (i < pad) {
        dst[i] = '0';
        i += 1;
    }
    @memcpy(dst[pad .. pad + n], tmp[0..n]);
    return width;
}

// Decode Base62 string into u64 with strict overflow and character validation
pub fn decodeU64(src: []const u8): DecodeU64Result {
    if (src.len == 0) {
        return DecodeU64Result { value: 0, ok: false };
    }
    const max_before_mul: u64 = ((0xFFFFFFFF as u64) << 32 | (0xFFFFFFFF as u64)) / 62;
    const max_u64: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < src.len) {
        const d: u8 = B62_DEC_LUT[src[i] as usize];
        if ((d & 0x80) != 0) {
            return DecodeU64Result { value: 0, ok: false };
        }
        if (acc > max_before_mul) {
            return DecodeU64Result { value: 0, ok: false };
        }
        let mul: u64 = acc * 62;
        let du: u64 = d as u64;
        if (max_u64 - mul < du) {
            return DecodeU64Result { value: 0, ok: false };
        }
        acc = mul + du;
        i += 1;
    }
    return DecodeU64Result { value: acc, ok: true };
}

// Encode arbitrary byte slice (<= 128 bytes) into Base62, preserving leading zero bytes as '0'
pub fn encodeBytes(src: []const u8, dst: []u8): i64 {
    if (src.len == 0) {
        return 0;
    }
    if (src.len > 128) {
        return -1;
    }
    const leading_zeros: usize = b62CountLeading(src, 0);
    let digits: [256]u8 = undefined;
    let dlen: usize = 0;
    let i: usize = leading_zeros;
    while (i < src.len) {
        let carry: u32 = src[i] as u32;
        let j: usize = 0;
        while (j < dlen) {
            let val: u32 = (digits[j] as u32) * 256 + carry;
            digits[j] = (val % 62) as u8;
            carry = val / 62;
            j += 1;
        }
        while (carry > 0) {
            if (dlen >= 256) {
                return -1;
            }
            digits[dlen] = (carry % 62) as u8;
            dlen += 1;
            carry = carry / 62;
        }
        i += 1;
    }
    let total: usize = leading_zeros + dlen;
    if (dst.len < total) {
        return -1;
    }
    let k: usize = 0;
    while (k < leading_zeros) {
        dst[k] = '0';
        k += 1;
    }
    let m: usize = 0;
    while (m < dlen) {
        dst[leading_zeros + m] = ALPHABET[digits[dlen - 1 - m] as usize];
        m += 1;
    }
    return total as i64;
}

// Decode Base62 string (<= 256 chars) back into original byte slice with leading zeros preserved
pub fn decodeBytes(src: []const u8, dst: []u8): i64 {
    if (src.len == 0) {
        return 0;
    }
    if (src.len > 256) {
        return -1;
    }
    const leading_zeros: usize = b62CountLeading(src, '0' as u8);
    let bytes: [256]u8 = undefined;
    let blen: usize = 0;
    let i: usize = leading_zeros;
    while (i < src.len) {
        const d: u8 = B62_DEC_LUT[src[i] as usize];
        if ((d & 0x80) != 0) {
            return -1;
        }
        let carry: u32 = d as u32;
        let j: usize = 0;
        while (j < blen) {
            let val: u32 = (bytes[j] as u32) * 62 + carry;
            bytes[j] = (val & 0xFF) as u8;
            carry = val >> 8;
            j += 1;
        }
        while (carry > 0) {
            if (blen >= 256) {
                return -1;
            }
            bytes[blen] = (carry & 0xFF) as u8;
            blen += 1;
            carry = carry >> 8;
        }
        i += 1;
    }
    let total: usize = leading_zeros + blen;
    if (dst.len < total) {
        return -1;
    }
    let k: usize = 0;
    while (k < leading_zeros) {
        dst[k] = 0;
        k += 1;
    }
    let m: usize = 0;
    while (m < blen) {
        dst[leading_zeros + m] = bytes[blen - 1 - m];
        m += 1;
    }
    return total as i64;
}
