// @cap/memchr — Pure CP zero-allocation 16-lane SIMD byte & substring search engine
// Clean-room MIT implementation (Rust `memchr` v2 parity).
// Uses 16-lane hardware SIMD (`@Vector(16, u8)`) with `@splat` + `@reduce(.Or, ...)` for 16x throughput.

type McVec16 = @Vector(16, u8);

fn mcBytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i + 16 <= a.len) {
        const va: McVec16 = a[i .. i + 16][0..16].*;
        const vb: McVec16 = b[i .. i + 16][0..16].*;
        if (@reduce(.Or, va ^ vb) != 0) {
            return false;
        }
        i += 16;
    }
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

/// Finds the first occurrence of `needle` in `haystack` using 16-lane hardware SIMD acceleration.
/// Returns the byte offset, or -1 if not found.
pub fn memchr(needle: u8, haystack: []const u8): i64 {
    const len = haystack.len;
    if (len == 0) {
        return -1;
    }
    const splat: McVec16 = @splat(needle);
    let i: usize = 0;
    while (i + 16 <= len) {
        const v: McVec16 = haystack[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == splat)) {
            let k: usize = 0;
            while (k < 16) : (k += 1) {
                if (haystack[i + k] == needle) {
                    return (i + k) as i64;
                }
            }
        }
        i += 16;
    }
    while (i < len) : (i += 1) {
        if (haystack[i] == needle) {
            return i as i64;
        }
    }
    return -1;
}

/// Finds the first occurrence of either `n1` or `n2` in `haystack` using 16-lane SIMD acceleration.
/// Returns the byte offset, or -1 if neither is found.
pub fn memchr2(n1: u8, n2: u8, haystack: []const u8): i64 {
    const len = haystack.len;
    if (len == 0) {
        return -1;
    }
    const s1: McVec16 = @splat(n1);
    const s2: McVec16 = @splat(n2);
    let i: usize = 0;
    while (i + 16 <= len) {
        const v: McVec16 = haystack[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == s1) || @reduce(.Or, v == s2)) {
            let k: usize = 0;
            while (k < 16) : (k += 1) {
                const b = haystack[i + k];
                if (b == n1 || b == n2) {
                    return (i + k) as i64;
                }
            }
        }
        i += 16;
    }
    while (i < len) : (i += 1) {
        const b = haystack[i];
        if (b == n1 || b == n2) {
            return i as i64;
        }
    }
    return -1;
}

/// Finds the first occurrence of `n1`, `n2`, or `n3` in `haystack` using 16-lane SIMD acceleration.
/// Returns the byte offset, or -1 if none is found.
pub fn memchr3(n1: u8, n2: u8, n3: u8, haystack: []const u8): i64 {
    const len = haystack.len;
    if (len == 0) {
        return -1;
    }
    const s1: McVec16 = @splat(n1);
    const s2: McVec16 = @splat(n2);
    const s3: McVec16 = @splat(n3);
    let i: usize = 0;
    while (i + 16 <= len) {
        const v: McVec16 = haystack[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == s1) || @reduce(.Or, v == s2) || @reduce(.Or, v == s3)) {
            let k: usize = 0;
            while (k < 16) : (k += 1) {
                const b = haystack[i + k];
                if (b == n1 || b == n2 || b == n3) {
                    return (i + k) as i64;
                }
            }
        }
        i += 16;
    }
    while (i < len) : (i += 1) {
        const b = haystack[i];
        if (b == n1 || b == n2 || b == n3) {
            return i as i64;
        }
    }
    return -1;
}

/// Finds the last occurrence of `needle` in `haystack` using reverse 16-lane SIMD acceleration.
/// Returns the byte offset, or -1 if not found.
pub fn memrchr(needle: u8, haystack: []const u8): i64 {
    const len = haystack.len;
    if (len == 0) {
        return -1;
    }
    const splat: McVec16 = @splat(needle);
    let i: usize = len;
    while (i >= 16) {
        i -= 16;
        const v: McVec16 = haystack[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == splat)) {
            let k: usize = 16;
            while (k > 0) {
                k -= 1;
                if (haystack[i + k] == needle) {
                    return (i + k) as i64;
                }
            }
        }
    }
    while (i > 0) {
        i -= 1;
        if (haystack[i] == needle) {
            return i as i64;
        }
    }
    return -1;
}

/// Finds the last occurrence of either `n1` or `n2` in `haystack` using reverse 16-lane SIMD acceleration.
pub fn memrchr2(n1: u8, n2: u8, haystack: []const u8): i64 {
    const len = haystack.len;
    if (len == 0) {
        return -1;
    }
    const s1: McVec16 = @splat(n1);
    const s2: McVec16 = @splat(n2);
    let i: usize = len;
    while (i >= 16) {
        i -= 16;
        const v: McVec16 = haystack[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == s1) || @reduce(.Or, v == s2)) {
            let k: usize = 16;
            while (k > 0) {
                k -= 1;
                const b = haystack[i + k];
                if (b == n1 || b == n2) {
                    return (i + k) as i64;
                }
            }
        }
    }
    while (i > 0) {
        i -= 1;
        const b = haystack[i];
        if (b == n1 || b == n2) {
            return i as i64;
        }
    }
    return -1;
}

/// Finds the last occurrence of `n1`, `n2`, or `n3` in `haystack` using reverse 16-lane SIMD acceleration.
pub fn memrchr3(n1: u8, n2: u8, n3: u8, haystack: []const u8): i64 {
    const len = haystack.len;
    if (len == 0) {
        return -1;
    }
    const s1: McVec16 = @splat(n1);
    const s2: McVec16 = @splat(n2);
    const s3: McVec16 = @splat(n3);
    let i: usize = len;
    while (i >= 16) {
        i -= 16;
        const v: McVec16 = haystack[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == s1) || @reduce(.Or, v == s2) || @reduce(.Or, v == s3)) {
            let k: usize = 16;
            while (k > 0) {
                k -= 1;
                const b = haystack[i + k];
                if (b == n1 || b == n2 || b == n3) {
                    return (i + k) as i64;
                }
            }
        }
    }
    while (i > 0) {
        i -= 1;
        const b = haystack[i];
        if (b == n1 || b == n2 || b == n3) {
            return i as i64;
        }
    }
    return -1;
}

/// Counts all occurrences of `needle` in `haystack` using 16-lane hardware SIMD acceleration.
pub fn memchrCount(needle: u8, haystack: []const u8): usize {
    const len = haystack.len;
    if (len == 0) {
        return 0;
    }
    const splat: McVec16 = @splat(needle);
    let hits: usize = 0;
    let i: usize = 0;
    while (i + 16 <= len) {
        const v: McVec16 = haystack[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == splat)) {
            let k: usize = 0;
            while (k < 16) : (k += 1) {
                if (haystack[i + k] == needle) {
                    hits += 1;
                }
            }
        }
        i += 16;
    }
    while (i < len) : (i += 1) {
        if (haystack[i] == needle) {
            hits += 1;
        }
    }
    return hits;
}

/// Locates all occurrences of `needle` in `haystack`, writing offsets into `out_offsets`.
/// Returns the number of offsets written (stops cleanly when `out_offsets` is full).
pub fn memchrFindAll(needle: u8, haystack: []const u8, out_offsets: []usize): usize {
    const len = haystack.len;
    const cap = out_offsets.len;
    if (len == 0 || cap == 0) {
        return 0;
    }
    let found: usize = 0;
    let pos: usize = 0;
    while (pos < len && found < cap) {
        const hit = memchr(needle, haystack[pos..len]);
        if (hit < 0) {
            break;
        }
        const abs_idx = pos + (hit as usize);
        out_offsets[found] = abs_idx;
        found += 1;
        pos = abs_idx + 1;
    }
    return found;
}

/// Finds the first occurrence of substring `needle` in `haystack` using first/last-byte
/// prefiltering and 16-lane SIMD `memchr` skipping. Empty `needle` returns `0`; not found returns `-1`.
pub fn memmemFind(haystack: []const u8, needle: []const u8): i64 {
    const nlen = needle.len;
    const hlen = haystack.len;
    if (nlen == 0) {
        return 0;
    }
    if (nlen > hlen) {
        return -1;
    }
    if (nlen == 1) {
        return memchr(needle[0], haystack);
    }

    const first = needle[0];
    const last_idx = nlen - 1;
    const last_byte = needle[last_idx];
    const limit = hlen - nlen + 1;
    let pos: usize = 0;

    while (pos < limit) {
        const hit = memchr(first, haystack[pos..limit]);
        if (hit < 0) {
            return -1;
        }
        const cand = pos + (hit as usize);
        if (haystack[cand + last_idx] == last_byte) {
            if (nlen == 2 || mcBytesEq(haystack[(cand + 1)..(cand + last_idx)], needle[1..last_idx])) {
                return cand as i64;
            }
        }
        pos = cand + 1;
    }
    return -1;
}

/// Finds the last occurrence of substring `needle` in `haystack` using first/last-byte
/// prefiltering and 16-lane SIMD `memrchr` skipping. Empty `needle` returns `haystack.len`; not found returns `-1`.
pub fn memmemRfind(haystack: []const u8, needle: []const u8): i64 {
    const nlen = needle.len;
    const hlen = haystack.len;
    if (nlen == 0) {
        return hlen as i64;
    }
    if (nlen > hlen) {
        return -1;
    }
    if (nlen == 1) {
        return memrchr(needle[0], haystack);
    }

    const first = needle[0];
    const last_idx = nlen - 1;
    const last_byte = needle[last_idx];
    let end: usize = hlen - nlen + 1;

    while (end > 0) {
        const hit = memrchr(first, haystack[0..end]);
        if (hit < 0) {
            return -1;
        }
        const cand = hit as usize;
        if (haystack[cand + last_idx] == last_byte) {
            if (nlen == 2 || mcBytesEq(haystack[(cand + 1)..(cand + last_idx)], needle[1..last_idx])) {
                return cand as i64;
            }
        }
        end = cand;
    }
    return -1;
}

/// Alias for `memchrCount`, counting all occurrences of `needle` in `haystack`.
pub fn countByte(needle: u8, haystack: []const u8): usize {
    return memchrCount(needle, haystack);
}

export {
    memchr,
    memchr2,
    memchr3,
    memrchr,
    memrchr2,
    memrchr3,
    memchrCount,
    countByte,
    memchrFindAll,
    memmemFind,
    memmemRfind
};
