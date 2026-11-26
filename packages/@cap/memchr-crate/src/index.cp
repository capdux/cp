pub fn memchr(needle: u8, haystack: []u8): usize? {
    let i: usize = 0;
    while (i < haystack.len) {
        if (haystack[i] == needle) { return i; }
        i = i + 1;
    }
    return null;
}

pub fn memchr2(n1: u8, n2: u8, haystack: []u8): usize? {
    let i: usize = 0;
    while (i < haystack.len) {
        let b = haystack[i];
        if (b == n1 || b == n2) { return i; }
        i = i + 1;
    }
    return null;
}

pub fn memchr3(n1: u8, n2: u8, n3: u8, haystack: []u8): usize? {
    let i: usize = 0;
    while (i < haystack.len) {
        let b = haystack[i];
        if (b == n1 || b == n2 || b == n3) { return i; }
        i = i + 1;
    }
    return null;
}

pub fn memrchr(needle: u8, haystack: []u8): usize? {
    let i = haystack.len;
    while (i > 0) {
        i = i - 1;
        if (haystack[i] == needle) { return i; }
    }
    return null;
}

pub fn memrchr2(n1: u8, n2: u8, haystack: []u8): usize? {
    let i = haystack.len;
    while (i > 0) {
        i = i - 1;
        let b = haystack[i];
        if (b == n1 || b == n2) { return i; }
    }
    return null;
}

pub fn memrchr3(n1: u8, n2: u8, n3: u8, haystack: []u8): usize? {
    let i = haystack.len;
    while (i > 0) {
        i = i - 1;
        let b = haystack[i];
        if (b == n1 || b == n2 || b == n3) { return i; }
    }
    return null;
}

pub fn countByte(needle: u8, haystack: []u8): usize {
    let cnt: usize = 0;
    let i: usize = 0;
    while (i < haystack.len) {
        if (haystack[i] == needle) { cnt = cnt + 1; }
        i = i + 1;
    }
    return cnt;
}

// Fast Boyer-Moore-Horspool substring search (zero-heap, O(1) 256-byte skip table)
pub fn memmem(haystack: []u8, needle: []u8): usize? {
    if (needle.len == 0) { return 0; }
    if (needle.len > haystack.len) { return null; }
    if (needle.len == 1) { return memchr(needle[0], haystack); }

    let m = needle.len;
    let skip: [256]usize = [m; 256];
    let i: usize = 0;
    while (i + 1 < m) {
        skip[needle[i] as usize] = m - 1 - i;
        i = i + 1;
    }

    let last = needle[m - 1];
    let pos: usize = 0;
    let limit = haystack.len - m;
    while (pos <= limit) {
        let tail = haystack[pos + m - 1];
        if (tail == last) {
            let j: usize = 0;
            let matched = true;
            while (j + 1 < m) {
                if (haystack[pos + j] != needle[j]) {
                    matched = false;
                    break;
                }
                j = j + 1;
            }
            if (matched) { return pos; }
        }
        pos = pos + skip[tail as usize];
    }
    return null;
}

pub fn memrmem(haystack: []u8, needle: []u8): usize? {
    if (needle.len == 0) { return haystack.len; }
    if (needle.len > haystack.len) { return null; }
    if (needle.len == 1) { return memrchr(needle[0], haystack); }

    let m = needle.len;
    let pos = haystack.len - m;
    while (true) {
        if (haystack[pos] == needle[0] && haystack[pos + m - 1] == needle[m - 1]) {
            let j: usize = 1;
            let matched = true;
            while (j + 1 < m) {
                if (haystack[pos + j] != needle[j]) {
                    matched = false;
                    break;
                }
                j = j + 1;
            }
            if (matched) { return pos; }
        }
        if (pos == 0) { break; }
        pos = pos - 1;
    }
    return null;
}

pub fn memmemStr(haystack: string, needle: string): usize? {
    if (needle.len == 0) { return 0; }
    if (needle.len > haystack.len) { return null; }
    let limit = haystack.len - needle.len;
    let pos: usize = 0;
    while (pos <= limit) {
        if (haystack[pos] == needle[0] && haystack[pos + needle.len - 1] == needle[needle.len - 1]) {
            let j: usize = 0;
            let ok = true;
            while (j < needle.len) {
                if (haystack[pos + j] != needle[j]) { ok = false; break; }
                j = j + 1;
            }
            if (ok) { return pos; }
        }
        pos = pos + 1;
    }
    return null;
}
