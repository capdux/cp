// @cap/unicase — Zero-allocation case-insensitive string wrapper, search & FNV-1a hashing for CP
// Clean-room MIT implementation inspired by Rust `unicase` (516M downloads).

fn foldAsciiLower(b: u8) -> u8 {
    if (b >= 'A' && b <= 'Z') {
        return b + (32 as u8);
    }
    return b;
}

fn foldAsciiUpper(b: u8) -> u8 {
    if (b >= 'a' && b <= 'z') {
        return b - (32 as u8);
    }
    return b;
}

fn mulWrap64(a: u64, b: u64) -> u64 {
    const mask32: u64 = 0xFFFFFFFF;
    const shift32: u64 = 0x100000000;
    const a_lo = a & mask32;
    const a_hi = a / shift32;
    const b_lo = b & mask32;
    const b_hi = b / shift32;
    const p_lo = a_lo * b_lo;
    const mid1 = (a_lo * b_hi) & mask32;
    const mid2 = (a_hi * b_lo) & mask32;
    const hi_sum = ((p_lo / shift32) + mid1 + mid2) & mask32;
    return (hi_sum * shift32) | (p_lo & mask32);
}

pub fn eqIgnoreAsciiCase(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (foldAsciiLower(a[i]) != foldAsciiLower(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn cmpIgnoreAsciiCase(a: string, b: string) -> i32 {
    const minLen = if (a.len < b.len) { a.len } else { b.len };
    let i: usize = 0;
    while (i < minLen) {
        const ca = foldAsciiLower(a[i]);
        const cb = foldAsciiLower(b[i]);
        if (ca < cb) {
            return -1 as i32;
        }
        if (ca > cb) {
            return 1 as i32;
        }
        i += 1;
    }
    if (a.len < b.len) {
        return -1 as i32;
    }
    if (a.len > b.len) {
        return 1 as i32;
    }
    return 0 as i32;
}

pub fn startsWithIgnoreCase(s: string, prefix: string) -> bool {
    if (prefix.len > s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (foldAsciiLower(s[i]) != foldAsciiLower(prefix[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn endsWithIgnoreCase(s: string, suffix: string) -> bool {
    if (suffix.len > s.len) {
        return false;
    }
    const offset = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) {
        if (foldAsciiLower(s[offset + i]) != foldAsciiLower(suffix[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn indexOfIgnoreCase(haystack: string, needle: string) -> usize? {
    if (needle.len == 0) {
        return 0 as usize;
    }
    if (needle.len > haystack.len) {
        return null;
    }
    const limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) {
        let j: usize = 0;
        let ok = true;
        while (j < needle.len) {
            if (foldAsciiLower(haystack[i + j]) != foldAsciiLower(needle[j])) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) {
            return i;
        }
        i += 1;
    }
    return null;
}

pub fn containsIgnoreCase(haystack: string, needle: string) -> bool {
    return indexOfIgnoreCase(haystack, needle) != null;
}

pub fn hashIgnoreCase64(s: string) -> u64 {
    const shift32: u64 = 0x100000000;
    let h: u64 = ((0xCBF29CE4 as u64) * shift32) | (0x84222325 as u64);
    const prime: u64 = ((0x00000100 as u64) * shift32) | (0x000001B3 as u64);
    let i: usize = 0;
    while (i < s.len) {
        h = h ^ (foldAsciiLower(s[i]) as u64);
        h = mulWrap64(h, prime);
        i += 1;
    }
    return h;
}

pub fn toLowerAscii(s: string, out: []u8) -> string {
    let n = if (s.len < out.len) { s.len } else { out.len };
    let i: usize = 0;
    while (i < n) {
        out[i] = foldAsciiLower(s[i]);
        i += 1;
    }
    return out[0..n];
}

pub fn toUpperAscii(s: string, out: []u8) -> string {
    let n = if (s.len < out.len) { s.len } else { out.len };
    let i: usize = 0;
    while (i < n) {
        out[i] = foldAsciiUpper(s[i]);
        i += 1;
    }
    return out[0..n];
}

pub struct UniCase {
    pub inner: string,

    pub static fn new(s: string) -> UniCase {
        return UniCase{ inner: s };
    }

    pub fn asStr() -> string {
        return _.inner;
    }

    pub fn equals(other: UniCase) -> bool {
        return eqIgnoreAsciiCase(_.inner, other.inner);
    }

    pub fn cmp(other: UniCase) -> i32 {
        return cmpIgnoreAsciiCase(_.inner, other.inner);
    }

    pub fn hash64() -> u64 {
        return hashIgnoreCase64(_.inner);
    }
}

export {
    UniCase,
    eqIgnoreAsciiCase,
    cmpIgnoreAsciiCase,
    startsWithIgnoreCase,
    endsWithIgnoreCase,
    indexOfIgnoreCase,
    containsIgnoreCase,
    hashIgnoreCase64,
    toLowerAscii,
    toUpperAscii,
};
