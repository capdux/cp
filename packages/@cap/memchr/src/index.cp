// @cap/memchr — Pure CP zero-allocation SWAR-accelerated byte & substring search engine
// Clean-room MIT implementation (Rust `memchr` v2 parity).
// Uses 8-byte parallel SWAR (u64) word scanning with Mycroft's zero-byte mask for 8x throughput.

fn mcLoMask(): u64 {
    return ((0x01010101 as u64) << 32) | (0x01010101 as u64);
}

fn mcHiMask(): u64 {
    return ((0x80808080 as u64) << 32) | (0x80808080 as u64);
}

fn mcSplatByte(b: u8): u64 {
    return (b as u64) * mcLoMask();
}

fn mcLoadU64Le(buf: []const u8, offset: usize): u64 {
    return (buf[offset] as u64)
        | ((buf[offset + 1] as u64) << 8)
        | ((buf[offset + 2] as u64) << 16)
        | ((buf[offset + 3] as u64) << 24)
        | ((buf[offset + 4] as u64) << 32)
        | ((buf[offset + 5] as u64) << 40)
        | ((buf[offset + 6] as u64) << 48)
        | ((buf[offset + 7] as u64) << 56);
}

/// Mycroft's SWAR zero-byte detector: sets bit 0x80 in every lane whose byte is 0x00.
fn mcHasZeroByte(x: u64): u64 {
    let lo = mcLoMask();
    let hi = mcHiMask();
    return (x -% lo) & (~x) & hi;
}

/// Returns the lowest byte lane index (0..7) with its 0x80 bit set in `mask != 0`.
fn mcFirstHitLane(mask: u64): usize {
    let tz: u64 = @ctz(mask) as u64;
    return (tz >> 3) as usize;
}

fn mcBytesEq(a: []const u8, b: []const u8): bool {
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// Finds the first occurrence of `needle` in `haystack` using 8-byte SWAR acceleration.
/// Returns the byte offset, or -1 if not found.
pub fn memchr(needle: u8, haystack: []const u8): i64 {
    let len = haystack.len;
    if (len == 0) {
        return -1;
    }
    let splat = mcSplatByte(needle);
    let i: usize = 0;
    while (i + 8 <= len) {
        let word = mcLoadU64Le(haystack, i);
        let mask = mcHasZeroByte(word ^ splat);
        if (mask != 0) {
            return (i + mcFirstHitLane(mask)) as i64;
        }
        i = i + 8;
    }
    while (i < len) {
        if (haystack[i] == needle) {
            return i as i64;
        }
        i = i + 1;
    }
    return -1;
}

/// Finds the first occurrence of either `n1` or `n2` in `haystack` using SWAR acceleration.
/// Returns the byte offset, or -1 if neither is found.
pub fn memchr2(n1: u8, n2: u8, haystack: []const u8): i64 {
    let len = haystack.len;
    if (len == 0) {
        return -1;
    }
    let s1 = mcSplatByte(n1);
    let s2 = mcSplatByte(n2);
    let i: usize = 0;
    while (i + 8 <= len) {
        let word = mcLoadU64Le(haystack, i);
        let mask = mcHasZeroByte(word ^ s1) | mcHasZeroByte(word ^ s2);
        if (mask != 0) {
            return (i + mcFirstHitLane(mask)) as i64;
        }
        i = i + 8;
    }
    while (i < len) {
        let b = haystack[i];
        if (b == n1 || b == n2) {
            return i as i64;
        }
        i = i + 1;
    }
    return -1;
}

/// Finds the first occurrence of `n1`, `n2`, or `n3` in `haystack` using SWAR acceleration.
/// Returns the byte offset, or -1 if none is found.
pub fn memchr3(n1: u8, n2: u8, n3: u8, haystack: []const u8): i64 {
    let len = haystack.len;
    if (len == 0) {
        return -1;
    }
    let s1 = mcSplatByte(n1);
    let s2 = mcSplatByte(n2);
    let s3 = mcSplatByte(n3);
    let i: usize = 0;
    while (i + 8 <= len) {
        let word = mcLoadU64Le(haystack, i);
        let mask = mcHasZeroByte(word ^ s1)
            | mcHasZeroByte(word ^ s2)
            | mcHasZeroByte(word ^ s3);
        if (mask != 0) {
            return (i + mcFirstHitLane(mask)) as i64;
        }
        i = i + 8;
    }
    while (i < len) {
        let b = haystack[i];
        if (b == n1 || b == n2 || b == n3) {
            return i as i64;
        }
        i = i + 1;
    }
    return -1;
}

/// Finds the last occurrence of `needle` in `haystack` using reverse 8-byte SWAR acceleration.
/// Returns the byte offset, or -1 if not found.
pub fn memrchr(needle: u8, haystack: []const u8): i64 {
    let len = haystack.len;
    if (len == 0) {
        return -1;
    }
    let splat = mcSplatByte(needle);
    let i: usize = len;
    while (i >= 8) {
        i = i - 8;
        let word = mcLoadU64Le(haystack, i);
        let mask = mcHasZeroByte(word ^ splat);
        if (mask != 0) {
            let k: usize = 8;
            while (k > 0) {
                k = k - 1;
                if (haystack[i + k] == needle) {
                    return (i + k) as i64;
                }
            }
        }
    }
    while (i > 0) {
        i = i - 1;
        if (haystack[i] == needle) {
            return i as i64;
        }
    }
    return -1;
}

/// Finds the last occurrence of either `n1` or `n2` in `haystack` using reverse SWAR acceleration.
pub fn memrchr2(n1: u8, n2: u8, haystack: []const u8): i64 {
    let len = haystack.len;
    if (len == 0) {
        return -1;
    }
    let s1 = mcSplatByte(n1);
    let s2 = mcSplatByte(n2);
    let i: usize = len;
    while (i >= 8) {
        i = i - 8;
        let word = mcLoadU64Le(haystack, i);
        let mask = mcHasZeroByte(word ^ s1) | mcHasZeroByte(word ^ s2);
        if (mask != 0) {
            let k: usize = 8;
            while (k > 0) {
                k = k - 1;
                let b = haystack[i + k];
                if (b == n1 || b == n2) {
                    return (i + k) as i64;
                }
            }
        }
    }
    while (i > 0) {
        i = i - 1;
        let b = haystack[i];
        if (b == n1 || b == n2) {
            return i as i64;
        }
    }
    return -1;
}

/// Finds the last occurrence of `n1`, `n2`, or `n3` in `haystack` using reverse SWAR acceleration.
pub fn memrchr3(n1: u8, n2: u8, n3: u8, haystack: []const u8): i64 {
    let len = haystack.len;
    if (len == 0) {
        return -1;
    }
    let s1 = mcSplatByte(n1);
    let s2 = mcSplatByte(n2);
    let s3 = mcSplatByte(n3);
    let i: usize = len;
    while (i >= 8) {
        i = i - 8;
        let word = mcLoadU64Le(haystack, i);
        let mask = mcHasZeroByte(word ^ s1)
            | mcHasZeroByte(word ^ s2)
            | mcHasZeroByte(word ^ s3);
        if (mask != 0) {
            let k: usize = 8;
            while (k > 0) {
                k = k - 1;
                let b = haystack[i + k];
                if (b == n1 || b == n2 || b == n3) {
                    return (i + k) as i64;
                }
            }
        }
    }
    while (i > 0) {
        i = i - 1;
        let b = haystack[i];
        if (b == n1 || b == n2 || b == n3) {
            return i as i64;
        }
    }
    return -1;
}

/// Counts all occurrences of `needle` in `haystack` using 8-byte SWAR acceleration.
pub fn memchrCount(needle: u8, haystack: []const u8): usize {
    let len = haystack.len;
    if (len == 0) {
        return 0;
    }
    let splat = mcSplatByte(needle);
    let count: usize = 0;
    let i: usize = 0;
    while (i + 8 <= len) {
        let word = mcLoadU64Le(haystack, i);
        if (mcHasZeroByte(word ^ splat) != 0) {
            let k: usize = 0;
            while (k < 8) {
                if (haystack[i + k] == needle) {
                    count = count + 1;
                }
                k = k + 1;
            }
        }
        i = i + 8;
    }
    while (i < len) {
        if (haystack[i] == needle) {
            count = count + 1;
        }
        i = i + 1;
    }
    return count;
}

/// Locates all occurrences of `needle` in `haystack`, writing offsets into `out_offsets`.
/// Returns the number of offsets written (stops cleanly when `out_offsets` is full).
pub fn memchrFindAll(needle: u8, haystack: []const u8, out_offsets: []usize): usize {
    let len = haystack.len;
    let cap = out_offsets.len;
    if (len == 0 || cap == 0) {
        return 0;
    }
    let found: usize = 0;
    let pos: usize = 0;
    while (pos < len && found < cap) {
        let hit = memchr(needle, haystack[pos..len]);
        if (hit < 0) {
            break;
        }
        let abs_idx = pos + (hit as usize);
        out_offsets[found] = abs_idx;
        found = found + 1;
        pos = abs_idx + 1;
    }
    return found;
}

/// Finds the first occurrence of substring `needle` in `haystack` using first/last-byte
/// prefiltering and SWAR `memchr` skipping. Empty `needle` returns `0`; not found returns `-1`.
pub fn memmemFind(haystack: []const u8, needle: []const u8): i64 {
    let nlen = needle.len;
    let hlen = haystack.len;
    if (nlen == 0) {
        return 0;
    }
    if (nlen > hlen) {
        return -1;
    }
    if (nlen == 1) {
        return memchr(needle[0], haystack);
    }

    let first = needle[0];
    let last_idx = nlen - 1;
    let last_byte = needle[last_idx];
    let limit = hlen - nlen + 1;
    let pos: usize = 0;

    while (pos < limit) {
        let hit = memchr(first, haystack[pos..limit]);
        if (hit < 0) {
            return -1;
        }
        let cand = pos + (hit as usize);
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
/// prefiltering and SWAR `memrchr` skipping. Empty `needle` returns `haystack.len`; not found returns `-1`.
pub fn memmemRfind(haystack: []const u8, needle: []const u8): i64 {
    let nlen = needle.len;
    let hlen = haystack.len;
    if (nlen == 0) {
        return hlen as i64;
    }
    if (nlen > hlen) {
        return -1;
    }
    if (nlen == 1) {
        return memrchr(needle[0], haystack);
    }

    let first = needle[0];
    let last_idx = nlen - 1;
    let last_byte = needle[last_idx];
    let end: usize = hlen - nlen + 1;

    while (end > 0) {
        let hit = memrchr(first, haystack[0..end]);
        if (hit < 0) {
            return -1;
        }
        let cand = hit as usize;
        if (haystack[cand + last_idx] == last_byte) {
            if (nlen == 2 || mcBytesEq(haystack[(cand + 1)..(cand + last_idx)], needle[1..last_idx])) {
                return cand as i64;
            }
        }
        end = cand;
    }
    return -1;
}

export {
    memchr,
    memchr2,
    memchr3,
    memrchr,
    memrchr2,
    memrchr3,
    memchrCount,
    memchrFindAll,
    memmemFind,
    memmemRfind
};
