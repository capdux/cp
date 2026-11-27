// @cap/memmem — Zero-allocation BMH + KMP byte-substring search engine for CP
// Clean-room MIT implementation inspired by Rust `memchr::memmem` (1.82B downloads).

const MAX_NEEDLE: usize = 256;

pub struct Finder {
    pub needle_buf: [MAX_NEEDLE]u8,
    pub needle_len: usize,
    pub bad_char: [256]usize,
    pub lps: [MAX_NEEDLE]u16,
    pub is_periodic: bool,

    pub static fn init(needle: []const u8) -> Finder {
        let nbuf: [MAX_NEEDLE]u8 = undefined;
        let lps: [MAX_NEEDLE]u16 = undefined;
        let bc: [256]usize = undefined;

        let nlen = needle.len;
        if (nlen > MAX_NEEDLE) {
            nlen = MAX_NEEDLE;
        }

        let i: usize = 0;
        while (i < MAX_NEEDLE) {
            nbuf[i] = 0 as u8;
            lps[i] = 0 as u16;
            i += 1;
        }
        let k: usize = 0;
        while (k < nlen) {
            nbuf[k] = needle[k];
            k += 1;
        }

        // Build Boyer-Moore-Horspool bad-character shift table
        let b: usize = 0;
        while (b < 256) {
            bc[b] = nlen;
            b += 1;
        }
        if (nlen > (1 as usize)) {
            let j: usize = 0;
            const last = nlen - (1 as usize);
            while (j < last) {
                bc[nbuf[j] as usize] = last - j;
                j += 1;
            }
        }

        // Build KMP prefix function (longest proper prefix-suffix)
        let prefixLen: usize = 0;
        let idx: usize = 1;
        while (idx < nlen) {
            while (prefixLen > (0 as usize) && nbuf[idx] != nbuf[prefixLen]) {
                prefixLen = lps[prefixLen - (1 as usize)] as usize;
            }
            if (nbuf[idx] == nbuf[prefixLen]) {
                prefixLen += 1;
            }
            lps[idx] = prefixLen as u16;
            idx += 1;
        }

        let periodic = false;
        if (nlen >= (4 as usize)) {
            if ((lps[nlen - (1 as usize)] as usize) * (2 as usize) >= nlen) {
                periodic = true;
            }
        }

        return Finder{
            needle_buf: nbuf,
            needle_len: nlen,
            bad_char: bc,
            lps: lps,
            is_periodic: periodic,
        };
    }

    pub fn needleLen() -> usize {
        return _.needle_len;
    }

    fn findKmpFrom(haystack: []const u8, start: usize) -> i64 {
        const m = _.needle_len;
        let i = start;
        let j: usize = 0;
        while (i < haystack.len) {
            while (j > (0 as usize) && haystack[i] != _.needle_buf[j]) {
                j = _.lps[j - (1 as usize)] as usize;
            }
            if (haystack[i] == _.needle_buf[j]) {
                j += 1;
                if (j == m) {
                    return (i + (1 as usize) - m) as i64;
                }
            }
            i += 1;
        }
        return -1 as i64;
    }

    pub fn findFrom(haystack: []const u8, start: usize) -> i64 {
        const m = _.needle_len;
        if (m == (0 as usize)) {
            if (start <= haystack.len) {
                return start as i64;
            }
            return -1 as i64;
        }
        if (start > haystack.len || (haystack.len - start) < m) {
            return -1 as i64;
        }
        if (m == (1 as usize)) {
            const target = _.needle_buf[0];
            let i = start;
            while (i < haystack.len) {
                if (haystack[i] == target) {
                    return i as i64;
                }
                i += 1;
            }
            return -1 as i64;
        }
        if (_.is_periodic) {
            return _.findKmpFrom(haystack, start);
        }

        // Boyer-Moore-Horspool fast skip with KMP fallback guard against pathological inputs
        const last = m - (1 as usize);
        const lastByte = _.needle_buf[last];
        const limit = haystack.len - m;
        let pos = start;
        let cmpBudget = (haystack.len - start) * (2 as usize) + (64 as usize);
        let cmps: usize = 0;

        while (pos <= limit) {
            const c = haystack[pos + last];
            if (c == lastByte) {
                let k = last;
                let matched = true;
                while (k > (0 as usize)) {
                    k -= 1;
                    cmps += 1;
                    if (haystack[pos + k] != _.needle_buf[k]) {
                        matched = false;
                        break;
                    }
                }
                if (matched) {
                    return pos as i64;
                }
                if (cmps > cmpBudget) {
                    return _.findKmpFrom(haystack, pos);
                }
            }
            const step = _.bad_char[c as usize];
            if (step == (0 as usize)) {
                pos += 1;
            } else {
                pos += step;
            }
        }
        return -1 as i64;
    }

    pub fn findIn(haystack: []const u8) -> i64 {
        return _.findFrom(haystack, 0 as usize);
    }

    pub fn countIn(haystack: []const u8) -> usize {
        const m = _.needle_len;
        if (m == (0 as usize)) {
            return haystack.len + (1 as usize);
        }
        let total: usize = 0;
        let pos: usize = 0;
        while (pos <= haystack.len) {
            const hit = _.findFrom(haystack, pos);
            if (hit < (0 as i64)) {
                break;
            }
            total += 1;
            pos = (hit as usize) + m;
        }
        return total;
    }
}

fn findLongNeedle(haystack: []const u8, needle: []const u8) -> i64 {
    if (needle.len > haystack.len) {
        return -1 as i64;
    }
    const limit = haystack.len - needle.len;
    const last = needle.len - (1 as usize);
    let pos: usize = 0;
    while (pos <= limit) {
        if (haystack[pos] == needle[0] && haystack[pos + last] == needle[last]) {
            let k: usize = 1;
            let ok = true;
            while (k < last) {
                if (haystack[pos + k] != needle[k]) {
                    ok = false;
                    break;
                }
                k += 1;
            }
            if (ok) {
                return pos as i64;
            }
        }
        pos += 1;
    }
    return -1 as i64;
}

pub fn find(haystack: []const u8, needle: []const u8) -> i64 {
    if (needle.len == (0 as usize)) {
        return 0 as i64;
    }
    if (needle.len > haystack.len) {
        return -1 as i64;
    }
    if (needle.len == (1 as usize)) {
        const target = needle[0];
        let i: usize = 0;
        while (i < haystack.len) {
            if (haystack[i] == target) {
                return i as i64;
            }
            i += 1;
        }
        return -1 as i64;
    }
    if (needle.len <= MAX_NEEDLE) {
        const f = Finder.init(needle);
        return f.findIn(haystack);
    }
    return findLongNeedle(haystack, needle);
}

pub fn rfind(haystack: []const u8, needle: []const u8) -> i64 {
    if (needle.len == (0 as usize)) {
        return haystack.len as i64;
    }
    if (needle.len > haystack.len) {
        return -1 as i64;
    }
    const last = needle.len - (1 as usize);
    const firstByte = needle[0];
    const lastByte = needle[last];
    let pos = haystack.len - needle.len;
    while (true) {
        if (haystack[pos] == firstByte && haystack[pos + last] == lastByte) {
            let k: usize = 1;
            let ok = true;
            while (k < last) {
                if (haystack[pos + k] != needle[k]) {
                    ok = false;
                    break;
                }
                k += 1;
            }
            if (ok) {
                return pos as i64;
            }
        }
        if (pos == (0 as usize)) {
            break;
        }
        pos -= 1;
    }
    return -1 as i64;
}

pub fn contains(haystack: []const u8, needle: []const u8) -> bool {
    return find(haystack, needle) >= (0 as i64);
}

pub fn count(haystack: []const u8, needle: []const u8) -> usize {
    if (needle.len == (0 as usize)) {
        return haystack.len + (1 as usize);
    }
    if (needle.len > haystack.len) {
        return 0 as usize;
    }
    if (needle.len <= MAX_NEEDLE) {
        const f = Finder.init(needle);
        return f.countIn(haystack);
    }
    let total: usize = 0;
    let pos: usize = 0;
    while (pos + needle.len <= haystack.len) {
        const sub = findLongNeedle(haystack[pos..haystack.len], needle);
        if (sub < (0 as i64)) {
            break;
        }
        total += 1;
        pos = pos + (sub as usize) + needle.len;
    }
    return total;
}

export {
    Finder,
    find,
    rfind,
    contains,
    count,
};
