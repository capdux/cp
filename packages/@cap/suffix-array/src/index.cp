// @cap/suffix-array — Zero-heap static Suffix Array + Kasai O(N) LCP array
// MIT License (clean-room CP implementation)

pub const MAX_TEXT: usize = 256;

pub struct LrsResult {
    pub start: usize,
    pub len: usize,
    pub found: bool,
}

fn compareSuffixes(buf: []const u8, n: usize, a: usize, b: usize) -> i32 {
    if (a == b) {
        return 0;
    }
    let i: usize = a;
    let j: usize = b;
    while (i < n && j < n) {
        if (buf[i] < buf[j]) {
            return -1;
        }
        if (buf[i] > buf[j]) {
            return 1;
        }
        i += 1;
        j += 1;
    }
    if (i == n && j == n) {
        return 0;
    }
    if (i == n) {
        return -1;
    }
    return 1;
}

pub struct SuffixArray {
    pub text: [256]u8,
    pub sa: [256]u16,
    pub lcp: [256]u16,
    pub text_len: usize,

    pub static fn build(src: []const u8) -> SuffixArray {
        let res = SuffixArray {
            text: undefined,
            sa: undefined,
            lcp: undefined,
            text_len: 0,
        };

        let n: usize = src.len;
        if (n > MAX_TEXT) {
            n = MAX_TEXT;
        }
        res.text_len = n;

        let i: usize = 0;
        while (i < n) {
            res.text[i] = src[i];
            res.sa[i] = i as u16;
            res.lcp[i] = 0;
            i += 1;
        }

        if (n <= 1) {
            return res;
        }

        // Shell sort with Ciura gap sequence (fast & zero-heap for N <= 256)
        const gaps: [6]usize = [132, 57, 23, 10, 4, 1];
        const text_slice: []const u8 = res.text[0..n];
        let g: usize = 0;
        while (g < 6) {
            const gap: usize = gaps[g];
            let idx: usize = gap;
            while (idx < n) {
                const key: u16 = res.sa[idx];
                let pos: usize = idx;
                while (pos >= gap) {
                    const prev: u16 = res.sa[pos - gap];
                    if (compareSuffixes(text_slice, n, prev as usize, key as usize) <= 0) {
                        break;
                    }
                    res.sa[pos] = prev;
                    pos -= gap;
                }
                res.sa[pos] = key;
                idx += 1;
            }
            g += 1;
        }

        // Kasai's O(N) LCP array construction
        let rank: [256]u16 = undefined;
        let r_idx: usize = 0;
        while (r_idx < n) {
            rank[res.sa[r_idx] as usize] = r_idx as u16;
            r_idx += 1;
        }

        let k: usize = 0;
        let pos_i: usize = 0;
        while (pos_i < n) {
            const r: usize = rank[pos_i] as usize;
            if (r == 0) {
                k = 0;
                pos_i += 1;
                continue;
            }
            const pos_j: usize = res.sa[r - 1] as usize;
            while (pos_i + k < n && pos_j + k < n && res.text[pos_i + k] == res.text[pos_j + k]) {
                k += 1;
            }
            res.lcp[r] = k as u16;
            if (k > 0) {
                k -= 1;
            }
            pos_i += 1;
        }

        return res;
    }

    pub fn len() -> usize {
        return _.text_len;
    }

    pub fn suffixAt(rank: usize) -> u16 {
        if (rank >= _.text_len) {
            return 0;
        }
        return _.sa[rank];
    }

    pub fn lcpAt(rank: usize) -> u16 {
        if (rank == 0 || rank >= _.text_len) {
            return 0;
        }
        return _.lcp[rank];
    }

    fn comparePrefix(suffix_pos: usize, pat: []const u8) -> i32 {
        let k: usize = 0;
        while (k < pat.len) {
            if (suffix_pos + k >= _.text_len) {
                return -1;
            }
            const a: u8 = _.text[suffix_pos + k];
            const b: u8 = pat[k];
            if (a < b) {
                return -1;
            }
            if (a > b) {
                return 1;
            }
            k += 1;
        }
        return 0;
    }

    fn lowerBound(pat: []const u8) -> usize {
        let lo: usize = 0;
        let hi: usize = _.text_len;
        while (lo < hi) {
            const mid: usize = lo + (hi - lo) / 2;
            if (_.comparePrefix(_.sa[mid] as usize, pat) < 0) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    }

    fn upperBound(pat: []const u8) -> usize {
        let lo: usize = 0;
        let hi: usize = _.text_len;
        while (lo < hi) {
            const mid: usize = lo + (hi - lo) / 2;
            if (_.comparePrefix(_.sa[mid] as usize, pat) <= 0) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    }

    pub fn contains(pat: []const u8) -> bool {
        if (pat.len == 0) {
            return true;
        }
        if (_.text_len == 0 || pat.len > _.text_len) {
            return false;
        }
        const idx: usize = _.lowerBound(pat);
        if (idx >= _.text_len) {
            return false;
        }
        return _.comparePrefix(_.sa[idx] as usize, pat) == 0;
    }

    pub fn count(pat: []const u8) -> usize {
        if (pat.len == 0) {
            return _.text_len + 1;
        }
        if (_.text_len == 0 || pat.len > _.text_len) {
            return 0;
        }
        const lo: usize = _.lowerBound(pat);
        const hi: usize = _.upperBound(pat);
        return hi - lo;
    }

    pub fn longestRepeatedSubstring() -> LrsResult {
        let best_len: usize = 0;
        let best_start: usize = 0;
        let i: usize = 1;
        while (i < _.text_len) {
            const cur: usize = _.lcp[i] as usize;
            if (cur > best_len) {
                best_len = cur;
                best_start = _.sa[i] as usize;
            }
            i += 1;
        }
        if (best_len == 0) {
            return LrsResult { start: 0, len: 0, found: false };
        }
        return LrsResult { start: best_start, len: best_len, found: true };
    }
}
