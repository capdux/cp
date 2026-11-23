// Reference-hardened against servo/rust-url (MIT/Apache-2.0)
// Source: cp/lib/idna/punycode.cp (Pure CP RFC 3492 Bootstring / Punycode Codec & UTF-8 / 16-Lane SIMD Helpers)
// Zero external C dependency implementation using pure CP, checked u32 arithmetic, and 16-lane @Vector(16, u8) SIMD.

const std = @import("std");

type IdnaVec16u8 = @Vector(16, u8);

/// Unified error set for RFC 3492 Punycode and IDNA domain processing.
pub err IdnaError {
    InvalidUtf8,
    Overflow,
    InvalidPunycode,
    LabelTooLong,
    DomainTooLong,
    EmptyLabel,
    InvalidChar,
    LeadingOrTrailingHyphen,
    DisallowedBidirection,
    OutOfMemory,
    ReservedHyphens,
}

// RFC 3492 Section 5 Bootstring parameters for Punycode
pub const PUNYCODE_BASE: u32 = 36;
pub const PUNYCODE_TMIN: u32 = 1;
pub const PUNYCODE_TMAX: u32 = 26;
pub const PUNYCODE_SKEW: u32 = 38;
pub const PUNYCODE_DAMP: u32 = 700;
pub const PUNYCODE_INITIAL_BIAS: u32 = 72;
pub const PUNYCODE_INITIAL_N: u32 = 128;
const DELIMITER: u8 = '-';
const MAX_U32: u32 = 0xFFFFFFFF;
const MAX_UNICODE: u32 = 0x10FFFF;

fn idnaLoadVec16(data: []const u8, off: usize): IdnaVec16u8 {
    const v: IdnaVec16u8 = [
        data[off], data[off + 1], data[off + 2], data[off + 3],
        data[off + 4], data[off + 5], data[off + 6], data[off + 7],
        data[off + 8], data[off + 9], data[off + 10], data[off + 11],
        data[off + 12], data[off + 13], data[off + 14], data[off + 15],
    ];
    return v;
}

/// 16-lane `@Vector(16, u8)` SIMD check returning `true` if all bytes in `data` are 7-bit ASCII (`< 0x80`).
pub fn simdIdnaIsAscii(data: []const u8): bool {
    let i: usize = 0;
    const n = data.len;
    const v_high: IdnaVec16u8 = @splat(0x80);
    const v_zero: IdnaVec16u8 = @splat(0);
    while (i + 16 <= n) {
        const chunk = idnaLoadVec16(data, i);
        const mask: u16 = @bitCast((chunk & v_high) != v_zero);
        if (mask != 0) return false;
        i += 16;
    }
    while (i < n) : (i += 1) {
        if (data[i] >= 0x80) return false;
    }
    return true;
}

/// Scalar reference for `simdIdnaIsAscii` (differential verification).
pub fn simdIdnaIsAsciiScalar(data: []const u8): bool {
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        if (data[i] >= 0x80) return false;
    }
    return true;
}

/// 16-lane `@Vector(16, u8)` SIMD scan returning the first index of `'.'` in `haystack`, or `null`.
pub fn simdIdnaIndexOfDot(haystack: []const u8): usize? {
    let i: usize = 0;
    const n = haystack.len;
    const v_dot: IdnaVec16u8 = @splat('.');
    while (i + 16 <= n) {
        const chunk = idnaLoadVec16(haystack, i);
        const mask: u16 = @bitCast(chunk == v_dot);
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        if (haystack[i] == '.') return i;
    }
    return null;
}

/// Scalar reference for `simdIdnaIndexOfDot` (differential verification).
pub fn simdIdnaIndexOfDotScalar(haystack: []const u8): usize? {
    let i: usize = 0;
    while (i < haystack.len) : (i += 1) {
        if (haystack[i] == '.') return i;
    }
    return null;
}

/// 16-lane `@Vector(16, u8)` SIMD ASCII lowercase conversion into `out`. Returns bytes written.
pub fn simdIdnaToLowerAscii(input: []const u8, out: []u8): usize {
    const limit = if (input.len <= out.len) input.len else out.len;
    let i: usize = 0;
    const v_A: IdnaVec16u8 = @splat('A');
    const v_Z: IdnaVec16u8 = @splat('Z');
    const v_diff: IdnaVec16u8 = @splat(32);

    while (i + 16 <= limit) {
        const chunk = idnaLoadVec16(input, i);
        const is_upper = (chunk >= v_A) & (chunk <= v_Z);
        const lowered: IdnaVec16u8 = @select(u8, is_upper, chunk | v_diff, chunk);
        const arr: [16]u8 = lowered;
        @memcpy(out[i .. i + 16], arr[0..16]);
        i += 16;
    }
    while (i < limit) : (i += 1) {
        const c = input[i];
        out[i] = if (c >= 'A' && c <= 'Z') (c + 32) else c;
    }
    return limit;
}

/// Scalar reference for `simdIdnaToLowerAscii` (differential verification).
pub fn simdIdnaToLowerAsciiScalar(input: []const u8, out: []u8): usize {
    const limit = if (input.len <= out.len) input.len else out.len;
    let i: usize = 0;
    while (i < limit) : (i += 1) {
        const c = input[i];
        out[i] = if (c >= 'A' && c <= 'Z') (c + 32) else c;
    }
    return limit;
}

struct ByteBuf {
    items: []u8 = [],
    len: usize = 0,

    fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.items.len > 0) {
            alloc.free(_.items);
            _.items = [];
            _.len = 0;
        }
    }

    fn ensureCap(*_, alloc: std.mem.Allocator, needed: usize): (void) ! IdnaError {
        if (needed <= _.items.len) {
            return;
        }
        let new_cap: usize = if (_.items.len == 0) 32 else _.items.len * 2;
        while (new_cap < needed) {
            new_cap *= 2;
        }
        if (_.items.len == 0) {
            _.items = alloc.alloc(u8, new_cap) catch return IdnaError.OutOfMemory;
        } else {
            _.items = alloc.realloc(_.items, new_cap) catch return IdnaError.OutOfMemory;
        }
    }

    fn append(*_, alloc: std.mem.Allocator, b: u8): (void) ! IdnaError {
        _.ensureCap(alloc, _.len + 1)?;
        _.items[_.len] = b;
        _.len += 1;
    }

    fn appendSlice(*_, alloc: std.mem.Allocator, s: []const u8): (void) ! IdnaError {
        if (s.len == 0) {
            return;
        }
        _.ensureCap(alloc, _.len + s.len)?;
        @memcpy(_.items[_.len .. _.len + s.len], s);
        _.len += s.len;
    }

    fn toOwnedSlice(*_, alloc: std.mem.Allocator): ([]u8) ! IdnaError {
        const out = alloc.alloc(u8, _.len) catch return IdnaError.OutOfMemory;
        if (_.len > 0) {
            @memcpy(out, _.items[0.._.len]);
        }
        _.deinit(alloc);
        return out;
    }
}

struct U32Buf {
    items: []u32 = [],
    len: usize = 0,

    fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.items.len > 0) {
            alloc.free(_.items);
            _.items = [];
            _.len = 0;
        }
    }

    fn ensureCap(*_, alloc: std.mem.Allocator, needed: usize): (void) ! IdnaError {
        if (needed <= _.items.len) {
            return;
        }
        let new_cap: usize = if (_.items.len == 0) 32 else _.items.len * 2;
        while (new_cap < needed) {
            new_cap *= 2;
        }
        if (_.items.len == 0) {
            _.items = alloc.alloc(u32, new_cap) catch return IdnaError.OutOfMemory;
        } else {
            _.items = alloc.realloc(_.items, new_cap) catch return IdnaError.OutOfMemory;
        }
    }

    fn append(*_, alloc: std.mem.Allocator, val: u32): (void) ! IdnaError {
        _.ensureCap(alloc, _.len + 1)?;
        _.items[_.len] = val;
        _.len += 1;
    }

    fn insert(*_, alloc: std.mem.Allocator, idx: usize, val: u32): (void) ! IdnaError {
        if (idx > _.len) {
            return IdnaError.InvalidPunycode;
        }
        _.ensureCap(alloc, _.len + 1)?;
        let j = _.len;
        while (j > idx) {
            _.items[j] = _.items[j - 1];
            j -= 1;
        }
        _.items[idx] = val;
        _.len += 1;
    }
}

/// Strictly decodes a UTF-8 byte slice into Unicode scalar values (`u32`).
/// Rejects overlong sequences, UTF-16 surrogates (`0xD800..=0xDFFF`), and codepoints `> 0x10FFFF`.
pub fn decodeUtf8Codepoints(alloc: std.mem.Allocator, input: []const u8): ([]u32) ! IdnaError {
    let buf = U32Buf{};
    errdefer buf.deinit(alloc);

    let i: usize = 0;
    while (i < input.len) {
        const b0 = input[i];
        if (b0 < 0x80) {
            buf.append(alloc, b0 as u32)?;
            i += 1;
        } else if ((b0 & 0xE0) == 0xC0) {
            if (i + 1 >= input.len) return IdnaError.InvalidUtf8;
            const b1 = input[i + 1];
            if ((b1 & 0xC0) != 0x80) return IdnaError.InvalidUtf8;
            const cp: u32 = (((b0 & 0x1F) as u32) << 6) | ((b1 & 0x3F) as u32);
            if (cp < 0x80) return IdnaError.InvalidUtf8;
            buf.append(alloc, cp)?;
            i += 2;
        } else if ((b0 & 0xF0) == 0xE0) {
            if (i + 2 >= input.len) return IdnaError.InvalidUtf8;
            const b1 = input[i + 1];
            const b2 = input[i + 2];
            if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) return IdnaError.InvalidUtf8;
            const cp: u32 = (((b0 & 0x0F) as u32) << 12) | (((b1 & 0x3F) as u32) << 6) | ((b2 & 0x3F) as u32);
            if (cp < 0x800) return IdnaError.InvalidUtf8;
            if (cp >= 0xD800 && cp <= 0xDFFF) return IdnaError.InvalidUtf8;
            buf.append(alloc, cp)?;
            i += 3;
        } else if ((b0 & 0xF8) == 0xF0) {
            if (i + 3 >= input.len) return IdnaError.InvalidUtf8;
            const b1 = input[i + 1];
            const b2 = input[i + 2];
            const b3 = input[i + 3];
            if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) return IdnaError.InvalidUtf8;
            const cp: u32 = (((b0 & 0x07) as u32) << 18) | (((b1 & 0x3F) as u32) << 12) | (((b2 & 0x3F) as u32) << 6) | ((b3 & 0x3F) as u32);
            if (cp < 0x10000 || cp > MAX_UNICODE) return IdnaError.InvalidUtf8;
            buf.append(alloc, cp)?;
            i += 4;
        } else {
            return IdnaError.InvalidUtf8;
        }
    }

    const out = alloc.alloc(u32, buf.len) catch return IdnaError.OutOfMemory;
    if (buf.len > 0) {
        @memcpy(out, buf.items[0..buf.len]);
    }
    buf.deinit(alloc);
    return out;
}

/// Encodes a single Unicode scalar value `cp` into `ByteBuf` as UTF-8.
pub fn encodeUtf8Codepoint(alloc: std.mem.Allocator, out: *ByteBuf, cp: u32): (void) ! IdnaError {
    if (cp > MAX_UNICODE || (cp >= 0xD800 && cp <= 0xDFFF)) {
        return IdnaError.InvalidUtf8;
    }
    if (cp < 0x80) {
        out.append(alloc, cp as u8)?;
    } else if (cp < 0x800) {
        out.append(alloc, (0xC0 | (cp >> 6)) as u8)?;
        out.append(alloc, (0x80 | (cp & 0x3F)) as u8)?;
    } else if (cp < 0x10000) {
        out.append(alloc, (0xE0 | (cp >> 12)) as u8)?;
        out.append(alloc, (0x80 | ((cp >> 6) & 0x3F)) as u8)?;
        out.append(alloc, (0x80 | (cp & 0x3F)) as u8)?;
    } else {
        out.append(alloc, (0xF0 | (cp >> 18)) as u8)?;
        out.append(alloc, (0x80 | ((cp >> 12) & 0x3F)) as u8)?;
        out.append(alloc, (0x80 | ((cp >> 6) & 0x3F)) as u8)?;
        out.append(alloc, (0x80 | (cp & 0x3F)) as u8)?;
    }
}

fn adaptBias(delta_in: u32, numpoints: u32, firsttime: bool): u32 {
    if (numpoints == 0) return PUNYCODE_INITIAL_BIAS;
    let delta: u32 = if (firsttime) (delta_in / PUNYCODE_DAMP) else (delta_in / 2);
    delta += delta / numpoints;
    let k: u32 = 0;
    const threshold: u32 = ((PUNYCODE_BASE - PUNYCODE_TMIN) * PUNYCODE_TMAX) / 2;
    while (delta > threshold) {
        delta /= (PUNYCODE_BASE - PUNYCODE_TMIN);
        k += PUNYCODE_BASE;
    }
    return k + (((PUNYCODE_BASE - PUNYCODE_TMIN + 1) * delta) / (delta + PUNYCODE_SKEW));
}

fn encodeDigit(d: u32): u8 {
    if (d < 26) {
        return ('a' as u32 + d) as u8;
    }
    return ('0' as u32 + (d - 26)) as u8;
}

fn decodeDigit(c: u8): (u32) ! IdnaError {
    if (c >= '0' && c <= '9') {
        return (c - '0' + 26) as u32;
    }
    if (c >= 'A' && c <= 'Z') {
        return (c - 'A') as u32;
    }
    if (c >= 'a' && c <= 'z') {
        return (c - 'a') as u32;
    }
    return IdnaError.InvalidPunycode;
}

fn thresholdForK(k: u32, bias: u32): u32 {
    if (k <= bias) {
        return PUNYCODE_TMIN;
    }
    if (k >= bias + PUNYCODE_TMAX) {
        return PUNYCODE_TMAX;
    }
    return k - bias;
}

/// Encodes a UTF-8 label into an ASCII Punycode string per RFC 3492 Section 6.3.
/// Caller owns the returned slice and must free it with `alloc.free(...)`.
pub fn punycodeEncode(alloc: std.mem.Allocator, utf8_label: []const u8): ([]u8) ! IdnaError {
    if (utf8_label.len == 0) {
        return alloc.alloc(u8, 0) catch return IdnaError.OutOfMemory;
    }

    const cps = decodeUtf8Codepoints(alloc, utf8_label)?;
    defer alloc.free(cps);

    let out = ByteBuf{};
    errdefer out.deinit(alloc);

    let basic_count: u32 = 0;
    let idx: usize = 0;
    while (idx < cps.len) : (idx += 1) {
        if (cps[idx] < PUNYCODE_INITIAL_N) {
            out.append(alloc, cps[idx] as u8)?;
            basic_count += 1;
        }
    }

    let h: u32 = basic_count;
    const b: u32 = basic_count;
    if (b > 0) {
        out.append(alloc, DELIMITER)?;
    }

    let n: u32 = PUNYCODE_INITIAL_N;
    let delta: u32 = 0;
    let bias: u32 = PUNYCODE_INITIAL_BIAS;
    const total_cps: u32 = cps.len as u32;

    while (h < total_cps) {
        let m: u32 = MAX_U32;
        idx = 0;
        while (idx < cps.len) : (idx += 1) {
            const c = cps[idx];
            if (c >= n && c < m) {
                m = c;
            }
        }

        const diff_n = m - n;
        const h_plus_1 = h + 1;
        if (diff_n > (MAX_U32 - delta) / h_plus_1) {
            return IdnaError.Overflow;
        }
        delta += diff_n * h_plus_1;
        n = m;

        idx = 0;
        while (idx < cps.len) : (idx += 1) {
            const c = cps[idx];
            if (c < n) {
                if (delta == MAX_U32) {
                    return IdnaError.Overflow;
                }
                delta += 1;
            } else if (c == n) {
                let q = delta;
                let k: u32 = PUNYCODE_BASE;
                while (true) {
                    const t = thresholdForK(k, bias);
                    if (q < t) {
                        break;
                    }
                    const base_minus_t = PUNYCODE_BASE - t;
                    const q_minus_t = q - t;
                    out.append(alloc, encodeDigit(t + (q_minus_t % base_minus_t)))?;
                    q = q_minus_t / base_minus_t;
                    if (k > MAX_U32 - PUNYCODE_BASE) {
                        return IdnaError.Overflow;
                    }
                    k += PUNYCODE_BASE;
                }
                out.append(alloc, encodeDigit(q))?;
                bias = adaptBias(delta, h + 1, h == b);
                delta = 0;
                h += 1;
            }
        }

        if (h < total_cps) {
            if (delta == MAX_U32 || n >= MAX_UNICODE) {
                return IdnaError.Overflow;
            }
            delta += 1;
            n += 1;
        }
    }

    return out.toOwnedSlice(alloc);
}

/// Decodes an ASCII Punycode string into a UTF-8 label per RFC 3492 Section 6.2.
/// Rejects non-ASCII input, trailing delimiter with no encoded digits, surrogates,
/// truncated variable-length integers, and checked `u32` overflows.
/// Caller owns the returned slice and must free it with `alloc.free(...)`.
pub fn punycodeDecode(alloc: std.mem.Allocator, ascii_punycode: []const u8): ([]u8) ! IdnaError {
    if (ascii_punycode.len == 0) {
        return alloc.alloc(u8, 0) catch return IdnaError.OutOfMemory;
    }

    // 16-lane SIMD fast-reject of any non-ASCII byte
    if (!simdIdnaIsAscii(ascii_punycode)) {
        return IdnaError.InvalidPunycode;
    }

    let last_dash: usize = 0;
    let has_dash = false;
    let idx: usize = 0;
    while (idx < ascii_punycode.len) : (idx += 1) {
        if (ascii_punycode[idx] == DELIMITER) {
            last_dash = idx;
            has_dash = true;
        }
    }

    // RFC 3492 / servo/rust-url hardening: reject trailing '-' with no encoded delta digits
    if (has_dash && last_dash + 1 >= ascii_punycode.len) {
        return IdnaError.InvalidPunycode;
    }

    let cps = U32Buf{};
    defer cps.deinit(alloc);

    if (has_dash) {
        idx = 0;
        while (idx < last_dash) : (idx += 1) {
            cps.append(alloc, ascii_punycode[idx] as u32)?;
        }
    }

    let in_idx: usize = if (has_dash) (last_dash + 1) else 0;
    let n: u32 = PUNYCODE_INITIAL_N;
    let i: u32 = 0;
    let bias: u32 = PUNYCODE_INITIAL_BIAS;

    while (in_idx < ascii_punycode.len) {
        const oldi = i;
        let w: u32 = 1;
        let k: u32 = PUNYCODE_BASE;

        while (true) {
            if (in_idx >= ascii_punycode.len) {
                return IdnaError.InvalidPunycode;
            }
            const digit = decodeDigit(ascii_punycode[in_idx])?;
            in_idx += 1;

            if (digit > (MAX_U32 - i) / w) {
                return IdnaError.Overflow;
            }
            i += digit * w;

            const t = thresholdForK(k, bias);
            if (digit < t) {
                break;
            }

            const base_minus_t = PUNYCODE_BASE - t;
            if (w > MAX_U32 / base_minus_t) {
                return IdnaError.Overflow;
            }
            w *= base_minus_t;
            if (k > MAX_U32 - PUNYCODE_BASE) {
                return IdnaError.Overflow;
            }
            k += PUNYCODE_BASE;
        }

        if (cps.len >= (MAX_U32 as usize) - 1) {
            return IdnaError.Overflow;
        }
        const out_len_plus_1: u32 = (cps.len as u32) + 1;
        bias = adaptBias(i - oldi, out_len_plus_1, oldi == 0);

        const n_inc = i / out_len_plus_1;
        if (n_inc > MAX_U32 - n) {
            return IdnaError.Overflow;
        }
        n += n_inc;
        if (n > MAX_UNICODE) {
            return IdnaError.Overflow;
        }
        if (n >= 0xD800 && n <= 0xDFFF) {
            return IdnaError.InvalidPunycode;
        }
        i %= out_len_plus_1;

        cps.insert(alloc, i as usize, n)?;
        if (i == MAX_U32) {
            return IdnaError.Overflow;
        }
        i += 1;
    }

    let out = ByteBuf{};
    errdefer out.deinit(alloc);

    idx = 0;
    while (idx < cps.len) : (idx += 1) {
        encodeUtf8Codepoint(alloc, &out, cps.items[idx])?;
    }

    return out.toOwnedSlice(alloc);
}
