// Source: bun/src/highway/highway.cp -> cp/lib/highway/highway.cp (Pure CP SIMD Vectorized Byte Scanning & ASCII Primitives)
//
// Pure CP implementation of Highway-grade SIMD primitives using @Vector(32, u8)
// and @Vector(16, u8). Zero C++ or external runtime dependencies.

const std = @import("std");

/// Find the index of the first occurrence of `needle` in `haystack` using
/// 32-byte and 16-byte SIMD vector comparisons, or `null` if not found.
pub fn indexOfByte(haystack: []const u8, needle: u8): usize? {
    if (haystack.len == 0) {
        return null;
    }

    let found: usize? = null;
    zig {
        const n = haystack.len;
        var i: usize = 0;
        const target32: @Vector(32, u8) = @splat(needle);
        const target16: @Vector(16, u8) = @splat(needle);

        while (i + 32 <= n) {
            const chunk: @Vector(32, u8) = haystack[i..][0..32].*;
            const mask: u32 = @bitCast(chunk == target32);
            if (mask != 0) {
                found = i + @as(usize, @ctz(mask));
                break;
            }
            i += 32;
        }

        if (found == null) {
            while (i + 16 <= n) {
                const chunk: @Vector(16, u8) = haystack[i..][0..16].*;
                const mask: u16 = @bitCast(chunk == target16);
                if (mask != 0) {
                    found = i + @as(usize, @ctz(mask));
                    break;
                }
                i += 16;
            }
        }

        if (found == null) {
            while (i < n) {
                if (haystack[i] == needle) {
                    found = i;
                    break;
                }
                i += 1;
            }
        }
    }
    return found;
}

/// Find the index of the last occurrence of `needle` in `haystack` using
/// reverse 32-byte and 16-byte SIMD vector comparisons, or `null` if not found.
pub fn lastIndexOfByte(haystack: []const u8, needle: u8): usize? {
    if (haystack.len == 0) {
        return null;
    }

    let found: usize? = null;
    zig {
        var rem: usize = haystack.len;
        const target32: @Vector(32, u8) = @splat(needle);
        const target16: @Vector(16, u8) = @splat(needle);

        while (rem >= 32) {
            const base = rem - 32;
            const chunk: @Vector(32, u8) = haystack[base..][0..32].*;
            const mask: u32 = @bitCast(chunk == target32);
            if (mask != 0) {
                const bit_from_end: usize = 31 - @as(usize, @clz(mask));
                found = base + bit_from_end;
                break;
            }
            rem = base;
        }

        if (found == null) {
            while (rem >= 16) {
                const base = rem - 16;
                const chunk: @Vector(16, u8) = haystack[base..][0..16].*;
                const mask: u16 = @bitCast(chunk == target16);
                if (mask != 0) {
                    const bit_from_end: usize = 15 - @as(usize, @clz(mask));
                    found = base + bit_from_end;
                    break;
                }
                rem = base;
            }
        }

        if (found == null) {
            while (rem > 0) {
                rem -= 1;
                if (haystack[rem] == needle) {
                    found = rem;
                    break;
                }
            }
        }
    }
    return found;
}

/// Find the index of the first occurrence of any byte from `needles` in `haystack`.
/// Specializes 1..4 needle fast paths with SIMD mask ORing.
pub fn indexOfAnyByte(haystack: []const u8, needles: []const u8): usize? {
    if (haystack.len == 0 || needles.len == 0) {
        return null;
    }
    if (needles.len == 1) {
        return indexOfByte(haystack, needles[0]);
    }

    let found: usize? = null;
    zig {
        const n = haystack.len;
        var i: usize = 0;

        if (needles.len == 2) {
            const t0_32: @Vector(32, u8) = @splat(needles[0]);
            const t1_32: @Vector(32, u8) = @splat(needles[1]);
            while (i + 32 <= n) {
                const chunk: @Vector(32, u8) = haystack[i..][0..32].*;
                const m0: u32 = @bitCast(chunk == t0_32);
                const m1: u32 = @bitCast(chunk == t1_32);
                const mask = m0 | m1;
                if (mask != 0) {
                    found = i + @as(usize, @ctz(mask));
                    break;
                }
                i += 32;
            }
            if (found == null) {
                const t0_16: @Vector(16, u8) = @splat(needles[0]);
                const t1_16: @Vector(16, u8) = @splat(needles[1]);
                while (i + 16 <= n) {
                    const chunk: @Vector(16, u8) = haystack[i..][0..16].*;
                    const m0: u16 = @bitCast(chunk == t0_16);
                    const m1: u16 = @bitCast(chunk == t1_16);
                    const mask = m0 | m1;
                    if (mask != 0) {
                        found = i + @as(usize, @ctz(mask));
                        break;
                    }
                    i += 16;
                }
            }
        } else if (needles.len <= 4) {
            const b0 = needles[0];
            const b1 = needles[1];
            const b2 = needles[2];
            const b3 = if (needles.len == 4) needles[3] else b2;
            const t0_32: @Vector(32, u8) = @splat(b0);
            const t1_32: @Vector(32, u8) = @splat(b1);
            const t2_32: @Vector(32, u8) = @splat(b2);
            const t3_32: @Vector(32, u8) = @splat(b3);
            while (i + 32 <= n) {
                const chunk: @Vector(32, u8) = haystack[i..][0..32].*;
                const m0: u32 = @bitCast(chunk == t0_32);
                const m1: u32 = @bitCast(chunk == t1_32);
                const m2: u32 = @bitCast(chunk == t2_32);
                const m3: u32 = @bitCast(chunk == t3_32);
                const mask = m0 | m1 | m2 | m3;
                if (mask != 0) {
                    found = i + @as(usize, @ctz(mask));
                    break;
                }
                i += 32;
            }
            if (found == null) {
                const t0_16: @Vector(16, u8) = @splat(b0);
                const t1_16: @Vector(16, u8) = @splat(b1);
                const t2_16: @Vector(16, u8) = @splat(b2);
                const t3_16: @Vector(16, u8) = @splat(b3);
                while (i + 16 <= n) {
                    const chunk: @Vector(16, u8) = haystack[i..][0..16].*;
                    const m0: u16 = @bitCast(chunk == t0_16);
                    const m1: u16 = @bitCast(chunk == t1_16);
                    const m2: u16 = @bitCast(chunk == t2_16);
                    const m3: u16 = @bitCast(chunk == t3_16);
                    const mask = m0 | m1 | m2 | m3;
                    if (mask != 0) {
                        found = i + @as(usize, @ctz(mask));
                        break;
                    }
                    i += 16;
                }
            }
        } else {
            while (i + 16 <= n) {
                const chunk: @Vector(16, u8) = haystack[i..][0..16].*;
                var mask: u16 = 0;
                for (needles) |nb| {
                    const tv: @Vector(16, u8) = @splat(nb);
                    const m: u16 = @bitCast(chunk == tv);
                    mask |= m;
                }
                if (mask != 0) {
                    found = i + @as(usize, @ctz(mask));
                    break;
                }
                i += 16;
            }
        }

        if (found == null) {
            while (i < n) {
                const c = haystack[i];
                for (needles) |nb| {
                    if (c == nb) {
                        found = i;
                        break;
                    }
                }
                if (found != null) break;
                i += 1;
            }
        }
    }
    return found;
}

/// Count occurrences of `target` byte in `haystack` using SIMD `@popCount`.
pub fn countByte(haystack: []const u8, target: u8): usize {
    if (haystack.len == 0) {
        return 0;
    }

    let total: usize = 0;
    zig {
        const n = haystack.len;
        var i: usize = 0;
        const tv32: @Vector(32, u8) = @splat(target);
        const tv16: @Vector(16, u8) = @splat(target);

        while (i + 32 <= n) {
            const chunk: @Vector(32, u8) = haystack[i..][0..32].*;
            const mask: u32 = @bitCast(chunk == tv32);
            total += @as(usize, @popCount(mask));
            i += 32;
        }

        while (i + 16 <= n) {
            const chunk: @Vector(16, u8) = haystack[i..][0..16].*;
            const mask: u16 = @bitCast(chunk == tv16);
            total += @as(usize, @popCount(mask));
            i += 16;
        }

        while (i < n) {
            if (haystack[i] == target) {
                total += 1;
            }
            i += 1;
        }
    }
    return total;
}

/// Find the index of the first differing byte between `a` and `b`.
/// Returns `min(a.len, b.len)` if one slice is a strict prefix of the other,
/// or `null` if `a` and `b` are completely equal.
pub fn indexOfNeq(a: []const u8, b: []const u8): usize? {
    let diff: usize? = null;
    zig {
        const min_len = @min(a.len, b.len);
        var i: usize = 0;
        var found_inner = false;

        while (i + 32 <= min_len) {
            const ca: @Vector(32, u8) = a[i..][0..32].*;
            const cb: @Vector(32, u8) = b[i..][0..32].*;
            const mask: u32 = @bitCast(ca != cb);
            if (mask != 0) {
                diff = i + @as(usize, @ctz(mask));
                found_inner = true;
                break;
            }
            i += 32;
        }

        if (!found_inner) {
            while (i + 16 <= min_len) {
                const ca: @Vector(16, u8) = a[i..][0..16].*;
                const cb: @Vector(16, u8) = b[i..][0..16].*;
                const mask: u16 = @bitCast(ca != cb);
                if (mask != 0) {
                    diff = i + @as(usize, @ctz(mask));
                    found_inner = true;
                    break;
                }
                i += 16;
            }
        }

        if (!found_inner) {
            while (i < min_len) {
                if (a[i] != b[i]) {
                    diff = i;
                    found_inner = true;
                    break;
                }
                i += 1;
            }
        }

        if (!found_inner and a.len != b.len) {
            diff = min_len;
        }
    }
    return diff;
}

/// Compute the length of the longest common prefix of `a` and `b` using SIMD.
pub fn commonPrefixLen(a: []const u8, b: []const u8): usize {
    const neq = indexOfNeq(a, b);
    if (neq != null) {
        return neq.?;
    }
    return a.len;
}

/// Search for `needle` in `haystack` using a SIMD first-and-last byte filter
/// followed by middle verification. Returns the starting index or `null`.
pub fn indexOfSlice(haystack: []const u8, needle: []const u8): usize? {
    if (needle.len == 0) {
        return 0;
    }
    if (needle.len > haystack.len) {
        return null;
    }
    if (needle.len == 1) {
        return indexOfByte(haystack, needle[0]);
    }

    let found: usize? = null;
    zig {
        const limit = haystack.len - needle.len + 1;
        const first = needle[0];
        const last_off = needle.len - 1;
        const last = needle[last_off];
        var i: usize = 0;

        const v_first32: @Vector(32, u8) = @splat(first);
        const v_last32: @Vector(32, u8) = @splat(last);
        while (i + 32 <= limit) {
            const c_first: @Vector(32, u8) = haystack[i..][0..32].*;
            const c_last: @Vector(32, u8) = haystack[i + last_off ..][0..32].*;
            var mask: u32 = @bitCast((c_first == v_first32) & (c_last == v_last32));
            while (mask != 0) {
                const bit: usize = @as(usize, @ctz(mask));
                const pos = i + bit;
                if (needle.len == 2 or std.mem.eql(u8, haystack[pos + 1 .. pos + last_off], needle[1..last_off])) {
                    found = pos;
                    break;
                }
                mask &= mask - 1;
            }
            if (found != null) break;
            i += 32;
        }

        if (found == null) {
            const v_first16: @Vector(16, u8) = @splat(first);
            const v_last16: @Vector(16, u8) = @splat(last);
            while (i + 16 <= limit) {
                const c_first: @Vector(16, u8) = haystack[i..][0..16].*;
                const c_last: @Vector(16, u8) = haystack[i + last_off ..][0..16].*;
                var mask: u16 = @bitCast((c_first == v_first16) & (c_last == v_last16));
                while (mask != 0) {
                    const bit: usize = @as(usize, @ctz(mask));
                    const pos = i + bit;
                    if (needle.len == 2 or std.mem.eql(u8, haystack[pos + 1 .. pos + last_off], needle[1..last_off])) {
                        found = pos;
                        break;
                    }
                    mask &= mask - 1;
                }
                if (found != null) break;
                i += 16;
            }
        }

        if (found == null) {
            while (i < limit) {
                if (haystack[i] == first and haystack[i + last_off] == last) {
                    if (needle.len == 2 or std.mem.eql(u8, haystack[i + 1 .. i + last_off], needle[1..last_off])) {
                        found = i;
                        break;
                    }
                }
                i += 1;
            }
        }
    }
    return found;
}

/// Find the first non-ASCII byte (`>= 0x80`) in `bytes`, or `null` if pure ASCII.
fn firstNonAsciiIndex(bytes: []const u8): usize? {
    if (bytes.len == 0) {
        return null;
    }

    let found: usize? = null;
    zig {
        const n = bytes.len;
        var i: usize = 0;
        const hi32: @Vector(32, u8) = @splat(0x80);
        const z32: @Vector(32, u8) = @splat(0);
        while (i + 32 <= n) {
            const chunk: @Vector(32, u8) = bytes[i..][0..32].*;
            const mask: u32 = @bitCast((chunk & hi32) != z32);
            if (mask != 0) {
                found = i + @as(usize, @ctz(mask));
                break;
            }
            i += 32;
        }

        if (found == null) {
            const hi16: @Vector(16, u8) = @splat(0x80);
            const z16: @Vector(16, u8) = @splat(0);
            while (i + 16 <= n) {
                const chunk: @Vector(16, u8) = bytes[i..][0..16].*;
                const mask: u16 = @bitCast((chunk & hi16) != z16);
                if (mask != 0) {
                    found = i + @as(usize, @ctz(mask));
                    break;
                }
                i += 16;
            }
        }

        if (found == null) {
            while (i < n) {
                if (bytes[i] >= 0x80) {
                    found = i;
                    break;
                }
                i += 1;
            }
        }
    }
    return found;
}

/// Returns `true` if all bytes in `bytes` are 7-bit ASCII (`< 0x80`).
pub fn isAscii(bytes: []const u8): bool {
    return firstNonAsciiIndex(bytes) == null;
}

/// Fast UTF-8 validation: uses SIMD to skip leading pure-ASCII chunks and falls
/// back to `std.unicode.utf8ValidateSlice` only from the first non-ASCII byte.
pub fn isUtf8ValidFast(bytes: []const u8): bool {
    const first_non_ascii = firstNonAsciiIndex(bytes);
    if (first_non_ascii == null) {
        return true;
    }
    const idx = first_non_ascii.?;
    let valid = false;
    zig {
        valid = std.unicode.utf8ValidateSlice(bytes[idx..]);
    }
    return valid;
}

/// Returns `true` if `bytes` contains any ASCII control character (`< 0x20`).
pub fn containsControlChars(bytes: []const u8): bool {
    if (bytes.len == 0) {
        return false;
    }

    let has_ctrl = false;
    zig {
        const n = bytes.len;
        var i: usize = 0;
        const v32: @Vector(32, u8) = @splat(0x20);
        while (i + 32 <= n) {
            const chunk: @Vector(32, u8) = bytes[i..][0..32].*;
            const mask: u32 = @bitCast(chunk < v32);
            if (mask != 0) {
                has_ctrl = true;
                break;
            }
            i += 32;
        }

        if (!has_ctrl) {
            const v16: @Vector(16, u8) = @splat(0x20);
            while (i + 16 <= n) {
                const chunk: @Vector(16, u8) = bytes[i..][0..16].*;
                const mask: u16 = @bitCast(chunk < v16);
                if (mask != 0) {
                    has_ctrl = true;
                    break;
                }
                i += 16;
            }
        }

        if (!has_ctrl) {
            while (i < n) {
                if (bytes[i] < 0x20) {
                    has_ctrl = true;
                    break;
                }
                i += 1;
            }
        }
    }
    return has_ctrl;
}

/// Branchless SIMD conversion of ASCII `'A'..'Z'` in `src` to lowercase `'a'..'z'`
/// into `dst` (processes `min(dst.len, src.len)` bytes).
pub fn toLowerAscii(dst: []u8, src: []const u8) {
    zig {
        const limit = @min(dst.len, src.len);
        var i: usize = 0;
        const v_A32: @Vector(32, u8) = @splat('A');
        const v_Z32: @Vector(32, u8) = @splat('Z');
        const v_bit32: @Vector(32, u8) = @splat(0x20);

        while (i + 32 <= limit) {
            const chunk: @Vector(32, u8) = src[i..][0..32].*;
            const is_upper = (chunk >= v_A32) & (chunk <= v_Z32);
            const converted: @Vector(32, u8) = @select(u8, is_upper, chunk | v_bit32, chunk);
            dst[i..][0..32].* = converted;
            i += 32;
        }

        const v_A16: @Vector(16, u8) = @splat('A');
        const v_Z16: @Vector(16, u8) = @splat('Z');
        const v_bit16: @Vector(16, u8) = @splat(0x20);
        while (i + 16 <= limit) {
            const chunk: @Vector(16, u8) = src[i..][0..16].*;
            const is_upper = (chunk >= v_A16) & (chunk <= v_Z16);
            const converted: @Vector(16, u8) = @select(u8, is_upper, chunk | v_bit16, chunk);
            dst[i..][0..16].* = converted;
            i += 16;
        }

        while (i < limit) {
            const c = src[i];
            dst[i] = if (c >= 'A' and c <= 'Z') (c | 0x20) else c;
            i += 1;
        }
    }
}

/// Branchless SIMD conversion of ASCII `'a'..'z'` in `src` to uppercase `'A'..'Z'`
/// into `dst` (processes `min(dst.len, src.len)` bytes).
pub fn toUpperAscii(dst: []u8, src: []const u8) {
    zig {
        const limit = @min(dst.len, src.len);
        var i: usize = 0;
        const v_a32: @Vector(32, u8) = @splat('a');
        const v_z32: @Vector(32, u8) = @splat('z');
        const v_mask32: @Vector(32, u8) = @splat(@as(u8, 0xDF));

        while (i + 32 <= limit) {
            const chunk: @Vector(32, u8) = src[i..][0..32].*;
            const is_lower = (chunk >= v_a32) & (chunk <= v_z32);
            const converted: @Vector(32, u8) = @select(u8, is_lower, chunk & v_mask32, chunk);
            dst[i..][0..32].* = converted;
            i += 32;
        }

        const v_a16: @Vector(16, u8) = @splat('a');
        const v_z16: @Vector(16, u8) = @splat('z');
        const v_mask16: @Vector(16, u8) = @splat(@as(u8, 0xDF));
        while (i + 16 <= limit) {
            const chunk: @Vector(16, u8) = src[i..][0..16].*;
            const is_lower = (chunk >= v_a16) & (chunk <= v_z16);
            const converted: @Vector(16, u8) = @select(u8, is_lower, chunk & v_mask16, chunk);
            dst[i..][0..16].* = converted;
            i += 16;
        }

        while (i < limit) {
            const c = src[i];
            dst[i] = if (c >= 'a' and c <= 'z') (c & @as(u8, 0xDF)) else c;
            i += 1;
        }
    }
}

/// Replace all occurrences of `old_b` with `new_b` in `buf` in-place using SIMD.
pub fn replaceByte(buf: []u8, old_b: u8, new_b: u8) {
    if (buf.len == 0 || old_b == new_b) {
        return;
    }

    zig {
        const n = buf.len;
        var i: usize = 0;
        const v_old32: @Vector(32, u8) = @splat(old_b);
        const v_new32: @Vector(32, u8) = @splat(new_b);
        while (i + 32 <= n) {
            const chunk: @Vector(32, u8) = buf[i..][0..32].*;
            const eq = chunk == v_old32;
            const out: @Vector(32, u8) = @select(u8, eq, v_new32, chunk);
            buf[i..][0..32].* = out;
            i += 32;
        }

        const v_old16: @Vector(16, u8) = @splat(old_b);
        const v_new16: @Vector(16, u8) = @splat(new_b);
        while (i + 16 <= n) {
            const chunk: @Vector(16, u8) = buf[i..][0..16].*;
            const eq = chunk == v_old16;
            const out: @Vector(16, u8) = @select(u8, eq, v_new16, chunk);
            buf[i..][0..16].* = out;
            i += 16;
        }

        while (i < n) {
            if (buf[i] == old_b) {
                buf[i] = new_b;
            }
            i += 1;
        }
    }
}
