// packages/@cap/kmp-search/src/index.cp
// Zero-allocation Knuth-Morris-Pratt (KMP) O(N+M) substring search & LPS engine.
// MIT clean-room pure CP implementation.

/// Compute the KMP Longest Proper Prefix-Suffix (LPS / failure function) table for `pattern` into `out_lps`.
/// Returns `pattern.len` on success, or `0` if `pattern` is empty or `out_lps.len < pattern.len`.
pub fn computeLps(pattern: []const u8, out_lps: []usize): usize {
    if (pattern.len == 0 || out_lps.len < pattern.len) {
        return 0;
    }
    out_lps[0] = 0;
    let prefLen: usize = 0;
    let i: usize = 1;
    while (i < pattern.len) {
        if (pattern[i] == pattern[prefLen]) {
            prefLen += 1;
            out_lps[i] = prefLen;
            i += 1;
        } else if (prefLen > 0) {
            prefLen = out_lps[prefLen - 1];
        } else {
            out_lps[i] = 0;
            i += 1;
        }
    }
    return pattern.len;
}

/// Find the first byte offset of `needle` inside `haystack` using KMP O(N+M) linear search.
/// Returns `0` for empty `needle`, or `-1` if not found.
/// Uses a zero-heap stack LPS table for patterns up to 256 bytes (with exact fallback for longer patterns).
pub fn findFirst(haystack: []const u8, needle: []const u8): i64 {
    if (needle.len == 0) {
        return 0 as i64;
    }
    if (needle.len > haystack.len) {
        return -1 as i64;
    }

    if (needle.len <= 256) {
        let lps: [256]usize = undefined;
        * = computeLps(needle, lps[0..256]);
        let i: usize = 0;
        let j: usize = 0;
        while (i < haystack.len) {
            if (haystack[i] == needle[j]) {
                i += 1;
                j += 1;
                if (j == needle.len) {
                    return (i - j) as i64;
                }
            } else if (j > 0) {
                j = lps[j - 1];
            } else {
                i += 1;
            }
        }
        return -1 as i64;
    }

    // Fallback for patterns > 256 bytes.
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        let k: usize = 0;
        while (k < needle.len && haystack[i + k] == needle[k]) {
            k += 1;
        }
        if (k == needle.len) {
            return i as i64;
        }
        i += 1;
    }
    return -1 as i64;
}

/// Collect all overlapping or non-overlapping match byte offsets of `needle` in `haystack` into `out_offsets`.
/// Returns number of offsets written into `out_offsets`.
pub fn findAll(haystack: []const u8, needle: []const u8, overlapping: bool, out_offsets: []usize): usize {
    if (needle.len == 0 || needle.len > haystack.len || out_offsets.len == 0) {
        return 0;
    }

    let count: usize = 0;
    if (needle.len <= 256) {
        let lps: [256]usize = undefined;
        * = computeLps(needle, lps[0..256]);
        let i: usize = 0;
        let j: usize = 0;
        while (i < haystack.len) {
            if (haystack[i] == needle[j]) {
                i += 1;
                j += 1;
                if (j == needle.len) {
                    out_offsets[count] = i - j;
                    count += 1;
                    if (count >= out_offsets.len) {
                        break;
                    }
                    if (overlapping) {
                        j = lps[j - 1];
                    } else {
                        j = 0;
                    }
                }
            } else if (j > 0) {
                j = lps[j - 1];
            } else {
                i += 1;
            }
        }
        return count;
    }

    let i: usize = 0;
    while (i + needle.len <= haystack.len && count < out_offsets.len) {
        let k: usize = 0;
        while (k < needle.len && haystack[i + k] == needle[k]) {
            k += 1;
        }
        if (k == needle.len) {
            out_offsets[count] = i;
            count += 1;
            if (overlapping) {
                i += 1;
            } else {
                i += needle.len;
            }
        } else {
            i += 1;
        }
    }
    return count;
}

/// Count all overlapping or non-overlapping occurrences of `needle` in `haystack`.
pub fn countMatches(haystack: []const u8, needle: []const u8, overlapping: bool): usize {
    if (needle.len == 0 || needle.len > haystack.len) {
        return 0;
    }

    let count: usize = 0;
    if (needle.len <= 256) {
        let lps: [256]usize = undefined;
        * = computeLps(needle, lps[0..256]);
        let i: usize = 0;
        let j: usize = 0;
        while (i < haystack.len) {
            if (haystack[i] == needle[j]) {
                i += 1;
                j += 1;
                if (j == needle.len) {
                    count += 1;
                    if (overlapping) {
                        j = lps[j - 1];
                    } else {
                        j = 0;
                    }
                }
            } else if (j > 0) {
                j = lps[j - 1];
            } else {
                i += 1;
            }
        }
        return count;
    }

    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        let k: usize = 0;
        while (k < needle.len && haystack[i + k] == needle[k]) {
            k += 1;
        }
        if (k == needle.len) {
            count += 1;
            if (overlapping) {
                i += 1;
            } else {
                i += needle.len;
            }
        } else {
            i += 1;
        }
    }
    return count;
}

/// Returns true if `needle` occurs anywhere within `haystack`.
pub fn contains(haystack: []const u8, needle: []const u8): bool {
    return findFirst(haystack, needle) >= (0 as i64);
}

export {
    computeLps,
    findFirst,
    findAll,
    countMatches,
    contains,
};
