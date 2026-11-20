// Source: cp/lib/suffixarray/suffixarray.cp (Suffix Array, Kasai LCP Array & Substring Index Engine)
// Pure CP zero-C-dependency Suffix Array using O(N log N) prefix doubling + radix sort,
// O(N) Kasai LCP array construction, O(|P| log N) binary search queries,
// longest repeated substring extraction, and distinct substring counting.

const std = @import("std");

/// Error set for SuffixArray construction and substring queries.
pub err SuffixArrayError {
    EmptyPattern,
    OutOfMemory,
}

fn quickSortUsize(items: []usize, lo: usize, hi: usize) {
    if (lo >= hi) {
        return;
    }
    const pivot = items[lo + (hi - lo) / 2];
    let i = lo;
    let j = hi;
    while (true) {
        while (items[i] < pivot) : (i += 1) {}
        while (items[j] > pivot) : (j -= 1) {}
        if (i >= j) {
            break;
        }
        const tmp = items[i];
        items[i] = items[j];
        items[j] = tmp;
        i += 1;
        j -= 1;
    }
    if (j > lo) {
        quickSortUsize(items, lo, j);
    }
    if (j + 1 < hi) {
        quickSortUsize(items, j + 1, hi);
    }
}

/// Suffix Array + Kasai LCP Array substring index engine.
pub class SuffixArray {
    alloc: std.mem.Allocator;
    text: []u8;
    sa: []usize;
    lcp: []usize;

    /// Build a SuffixArray and Kasai LCP array for `text` in O(N log N) time.
    /// Supports empty text (`text.len == 0`) and arbitrary binary bytes including `\x00`.
    pub static fn build(alloc: std.mem.Allocator, text: []const u8): SuffixArray ! SuffixArrayError {
        const n = text.len;
        const owned_text = alloc.alloc(u8, n) catch return SuffixArrayError.OutOfMemory;
        errdefer alloc.free(owned_text);
        if (n > 0) {
            @memcpy(owned_text, text);
        }

        let sa = alloc.alloc(usize, n) catch return SuffixArrayError.OutOfMemory;
        errdefer alloc.free(sa);

        let lcp = alloc.alloc(usize, n) catch return SuffixArrayError.OutOfMemory;
        errdefer alloc.free(lcp);

        if (n == 0) {
            return SuffixArray{
                alloc: alloc,
                text: owned_text,
                sa: sa,
                lcp: lcp,
            };
        }

        if (n == 1) {
            sa[0] = 0;
            lcp[0] = 0;
            return SuffixArray{
                alloc: alloc,
                text: owned_text,
                sa: sa,
                lcp: lcp,
            };
        }

        // Prefix doubling + counting (radix) sort: O(N log N)
        let rk = alloc.alloc(usize, n) catch return SuffixArrayError.OutOfMemory;
        defer alloc.free(rk);

        let tmp_rk = alloc.alloc(usize, n) catch return SuffixArrayError.OutOfMemory;
        defer alloc.free(tmp_rk);

        let sa2 = alloc.alloc(usize, n) catch return SuffixArrayError.OutOfMemory;
        defer alloc.free(sa2);

        const cnt_cap: usize = if (n > 256) n else 256;
        let cnt = alloc.alloc(usize, cnt_cap) catch return SuffixArrayError.OutOfMemory;
        defer alloc.free(cnt);

        let m: usize = 256;
        let i: usize = 0;
        while (i < m) : (i += 1) {
            cnt[i] = 0;
        }
        i = 0 as usize;
        while (i < n) : (i += 1) {
            const c = usize(owned_text[i]);
            rk[i] = c;
            cnt[c] += 1;
        }
        i = 1 as usize;
        while (i < m) : (i += 1) {
            cnt[i] += cnt[i - 1];
        }
        i = n;
        while (i > 0) {
            i -= 1;
            const c = rk[i];
            cnt[c] -= 1;
            sa[cnt[c]] = i;
        }

        let k: usize = 1;
        while (k < n) : (k *= 2) {
            // Sort by second key into sa2
            let p: usize = 0;
            i = n - k;
            while (i < n) : (i += 1) {
                sa2[p] = i;
                p += 1;
            }
            i = 0 as usize;
            while (i < n) : (i += 1) {
                if (sa[i] >= k) {
                    sa2[p] = sa[i] - k;
                    p += 1;
                }
            }

            // Counting sort by first key
            i = 0 as usize;
            while (i < m) : (i += 1) {
                cnt[i] = 0;
            }
            i = 0 as usize;
            while (i < n) : (i += 1) {
                cnt[rk[sa2[i]]] += 1;
            }
            i = 1 as usize;
            while (i < m) : (i += 1) {
                cnt[i] += cnt[i - 1];
            }
            i = n;
            while (i > 0) {
                i -= 1;
                const s = sa2[i];
                const r = rk[s];
                cnt[r] -= 1;
                sa[cnt[r]] = s;
            }

            // Recompute equivalence classes into tmp_rk
            tmp_rk[sa[0]] = 0;
            let num_classes: usize = 1;
            i = 1 as usize;
            while (i < n) : (i += 1) {
                const prev_s = sa[i - 1];
                const cur_s = sa[i];
                let same = false;
                if (rk[prev_s] == rk[cur_s]) {
                    const prev_next = prev_s + k;
                    const cur_next = cur_s + k;
                    if (prev_next < n && cur_next < n && rk[prev_next] == rk[cur_next]) {
                        same = true;
                    }
                }
                if (same) {
                    tmp_rk[cur_s] = num_classes - 1;
                } else {
                    tmp_rk[cur_s] = num_classes;
                    num_classes += 1;
                }
            }

            const swap_rk = rk;
            rk = tmp_rk;
            tmp_rk = swap_rk;
            m = num_classes;
            if (m == n) {
                break;
            }
        }

        // Kasai O(N) LCP array construction
        // Reuse tmp_rk as inverse suffix array (inv[sa[idx]] = idx)
        i = 0 as usize;
        while (i < n) : (i += 1) {
            tmp_rk[sa[i]] = i;
        }

        lcp[0] = 0;
        let h: usize = 0;
        i = 0 as usize;
        while (i < n) : (i += 1) {
            const r = tmp_rk[i];
            if (r == 0) {
                lcp[0] = 0;
                h = 0;
                continue;
            }
            const prev = sa[r - 1];
            while (i + h < n && prev + h < n && owned_text[i + h] == owned_text[prev + h]) {
                h += 1;
            }
            lcp[r] = h;
            if (h > 0) {
                h -= 1;
            }
        }

        return SuffixArray{
            alloc: alloc,
            text: owned_text,
            sa: sa,
            lcp: lcp,
        };
    }

    /// Release all owned buffers (`text`, `sa`, `lcp`).
    pub fn deinit(*_) {
        _.alloc.free(_.lcp);
        _.alloc.free(_.sa);
        _.alloc.free(_.text);
    }

    /// Return the length of the indexed text in bytes.
    pub fn len(): usize {
        return _.text.len;
    }

    /// Return a read-only slice of the suffix array `sa`.
    pub fn saSlice(): []const usize {
        return _.sa;
    }

    /// Return a read-only slice of the Kasai LCP array `lcp`.
    pub fn lcpSlice(): []const usize {
        return _.lcp;
    }

    fn compareSuffixPrefix(sa_pos: usize, pattern: []const u8): i32 {
        const n = _.text.len;
        const avail = n - sa_pos;
        const cmp_len = if (avail < pattern.len) avail else pattern.len;
        let i: usize = 0;
        while (i < cmp_len) : (i += 1) {
            const a = _.text[sa_pos + i];
            const b = pattern[i];
            if (a < b) {
                return -1;
            }
            if (a > b) {
                return 1;
            }
        }
        if (avail < pattern.len) {
            return -1;
        }
        return 0;
    }

    fn lowerBound(pattern: []const u8): usize {
        let lo: usize = 0;
        let hi: usize = _.sa.len;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            if (_.compareSuffixPrefix(_.sa[mid], pattern) < 0) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    }

    fn upperBound(pattern: []const u8): usize {
        let lo: usize = 0;
        let hi: usize = _.sa.len;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            if (_.compareSuffixPrefix(_.sa[mid], pattern) <= 0) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    }

    /// Check whether `pattern` occurs at least once in O(|P| log N) time.
    /// Returns `SuffixArrayError.EmptyPattern` if `pattern.len == 0`.
    pub fn contains(pattern: []const u8): bool ! SuffixArrayError {
        if (pattern.len == 0) {
            return SuffixArrayError.EmptyPattern;
        }
        if (_.sa.len == 0) {
            return false;
        }
        const lo = _.lowerBound(pattern);
        if (lo >= _.sa.len) {
            return false;
        }
        return _.compareSuffixPrefix(_.sa[lo], pattern) == 0;
    }

    /// Count total occurrences (including overlapping) of `pattern` in O(|P| log N) time.
    /// Returns `SuffixArrayError.EmptyPattern` if `pattern.len == 0`.
    pub fn count(pattern: []const u8): usize ! SuffixArrayError {
        if (pattern.len == 0) {
            return SuffixArrayError.EmptyPattern;
        }
        if (_.sa.len == 0) {
            return 0;
        }
        const lo = _.lowerBound(pattern);
        if (lo >= _.sa.len || _.compareSuffixPrefix(_.sa[lo], pattern) != 0) {
            return 0;
        }
        const hi = _.upperBound(pattern);
        return hi - lo;
    }

    /// Find all starting byte offsets of `pattern` in ascending order.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    pub fn findAll(alloc: std.mem.Allocator, pattern: []const u8): ([]usize) ! SuffixArrayError {
        if (pattern.len == 0) {
            return SuffixArrayError.EmptyPattern;
        }
        if (_.sa.len == 0) {
            const empty_res = alloc.alloc(usize, 0) catch return SuffixArrayError.OutOfMemory;
            return empty_res;
        }
        const lo = _.lowerBound(pattern);
        if (lo >= _.sa.len || _.compareSuffixPrefix(_.sa[lo], pattern) != 0) {
            const empty_res = alloc.alloc(usize, 0) catch return SuffixArrayError.OutOfMemory;
            return empty_res;
        }
        const hi = _.upperBound(pattern);
        const total = hi - lo;
        let result = alloc.alloc(usize, total) catch return SuffixArrayError.OutOfMemory;
        let i: usize = 0;
        while (i < total) : (i += 1) {
            result[i] = _.sa[lo + i];
        }
        if (total > 1) {
            quickSortUsize(result, 0, total - 1);
        }
        return result;
    }

    /// Extract the longest repeated substring in O(N) time using the LCP array.
    /// Returns an empty slice if the text has length <= 1 or all characters are distinct.
    pub fn longestRepeatedSubstring(): []const u8 {
        const n = _.text.len;
        if (n <= 1) {
            return _.text[0..0];
        }
        let best_len: usize = 0;
        let best_idx: usize = 0;
        let i: usize = 1;
        while (i < n) : (i += 1) {
            if (_.lcp[i] > best_len) {
                best_len = _.lcp[i];
                best_idx = i;
            }
        }
        if (best_len == 0) {
            return _.text[0..0];
        }
        const start = _.sa[best_idx];
        return _.text[start .. start + best_len];
    }

    /// Compute the total number of distinct non-empty substrings in O(N) time:
    /// N*(N+1)/2 - sum(lcp[i]).
    pub fn distinctSubstringCount(): u64 {
        const n = _.text.len;
        if (n == 0) {
            return 0;
        }
        const n64 = n as u64;
        const total = (n64 * (n64 + 1)) / 2;
        let lcp_sum: u64 = 0;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            lcp_sum += _.lcp[i] as u64;
        }
        return total - lcp_sum;
    }
}
