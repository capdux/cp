// @cap/bmh-search — Zero-heap Boyer-Moore-Horspool sublinear substring search
// MIT License (clean-room CP implementation)

import { toLowerByte } from "cap:ascii";

pub struct BmhPattern {
    pub shift: [256]usize,
    pub pat_len: usize,
}

pub fn compilePattern(needle: []const u8) -> BmhPattern {
    let pat = BmhPattern {
        shift: undefined,
        pat_len: needle.len,
    };
    let def_shift: usize = needle.len;
    if (def_shift == 0) {
        def_shift = 1;
    }
    let i: usize = 0;
    while (i < 256) {
        pat.shift[i] = def_shift;
        i += 1;
    }
    if (needle.len > 1) {
        const last: usize = needle.len - 1;
        i = 0;
        while (i < last) {
            pat.shift[needle[i] as usize] = last - i;
            i += 1;
        }
    }
    return pat;
}

pub fn findWithCompiled(haystack: []const u8, needle: []const u8, compiled: BmhPattern) -> i64 {
    if (needle.len == 0) {
        return 0;
    }
    if (needle.len > haystack.len || compiled.pat_len != needle.len) {
        return -1;
    }
    const m: usize = needle.len;
    const last: usize = m - 1;
    let pos: usize = 0;

    while (pos + m <= haystack.len) {
        let j: usize = m;
        let matched: bool = true;
        while (j > 0) {
            j -= 1;
            if (haystack[pos + j] != needle[j]) {
                matched = false;
                break;
            }
        }
        if (matched) {
            return pos as i64;
        }
        pos += compiled.shift[haystack[pos + last] as usize];
    }
    return -1;
}

pub fn findFirst(haystack: []const u8, needle: []const u8) -> i64 {
    if (needle.len == 0) {
        return 0;
    }
    if (needle.len > haystack.len) {
        return -1;
    }
    const compiled: BmhPattern = compilePattern(needle);
    return findWithCompiled(haystack, needle, compiled);
}

pub fn findLast(haystack: []const u8, needle: []const u8) -> i64 {
    if (needle.len == 0) {
        return haystack.len as i64;
    }
    if (needle.len > haystack.len) {
        return -1;
    }
    const m: usize = needle.len;

    // Reverse Horspool shift table indexed by haystack[pos] (first char of window)
    let rshift: [256]usize = undefined;
    let i: usize = 0;
    while (i < 256) {
        rshift[i] = m;
        i += 1;
    }
    if (m > 1) {
        let k: usize = m;
        while (k > 1) {
            k -= 1;
            rshift[needle[k] as usize] = k;
        }
    }

    let pos: usize = haystack.len - m;
    while (true) {
        let j: usize = 0;
        let matched: bool = true;
        while (j < m) {
            if (haystack[pos + j] != needle[j]) {
                matched = false;
                break;
            }
            j += 1;
        }
        if (matched) {
            return pos as i64;
        }
        const step: usize = rshift[haystack[pos] as usize];
        if (pos < step) {
            break;
        }
        pos -= step;
    }
    return -1;
}

pub fn countNonOverlapping(haystack: []const u8, needle: []const u8) -> usize {
    if (needle.len == 0 || needle.len > haystack.len) {
        return 0;
    }
    const compiled: BmhPattern = compilePattern(needle);
    let count: usize = 0;
    let pos: usize = 0;
    while (pos + needle.len <= haystack.len) {
        const idx: i64 = findWithCompiled(haystack[pos..haystack.len], needle, compiled);
        if (idx < 0) {
            break;
        }
        count += 1;
        pos += (idx as usize) + needle.len;
    }
    return count;
}

pub fn findIgnoreCaseAscii(haystack: string, needle: string) -> i64 {
    if (needle.len == 0) {
        return 0;
    }
    if (needle.len > haystack.len) {
        return -1;
    }
    const m: usize = needle.len;
    const last: usize = m - 1;

    let shift: [256]usize = undefined;
    let i: usize = 0;
    while (i < 256) {
        shift[i] = m;
        i += 1;
    }
    if (m > 1) {
        i = 0;
        while (i < last) {
            const lc: u8 = toLowerByte(needle[i]);
            shift[lc as usize] = last - i;
            if (lc >= 'a' as u8 && lc <= 'z' as u8) {
                shift[(lc - 32) as usize] = last - i;
            }
            i += 1;
        }
    }

    let pos: usize = 0;
    while (pos + m <= haystack.len) {
        let j: usize = m;
        let matched: bool = true;
        while (j > 0) {
            j -= 1;
            if (toLowerByte(haystack[pos + j]) != toLowerByte(needle[j])) {
                matched = false;
                break;
            }
        }
        if (matched) {
            return pos as i64;
        }
        pos += shift[haystack[pos + last] as usize];
    }
    return -1;
}
