// Reference Only: Functional overlap with lib/text/suffixarray.cp | Kept for comparison/analysis
// Source: native-cp-stdlib (Suffix Array & Kasai LCP)
import std from "std";

pub class SuffixArray {
    alloc: std.mem.Allocator,
    text: []const u8,
    sa: []usize,
    lcp: []usize,

    pub fn init(alloc: std.mem.Allocator, text: []const u8) -> !SuffixArray {
        const n = text.len;
        let sa = try alloc.alloc(usize, n);
        errdefer alloc.free(sa);

        let lcp = try alloc.alloc(usize, n);
        errdefer alloc.free(lcp);

        if (n == 0) {
            return SuffixArray{
                .alloc = alloc,
                .text = text,
                .sa = sa,
                .lcp = lcp,
            };
        }

        if (n == 1) {
            sa[0] = 0;
            lcp[0] = 0;
            return SuffixArray{
                .alloc = alloc,
                .text = text,
                .sa = sa,
                .lcp = lcp,
            };
        }

        let m: usize = if (n > 256) n else 256;
        let rank = try alloc.alloc(usize, n);
        defer alloc.free(rank);
        let tmp_rank = try alloc.alloc(usize, n);
        defer alloc.free(tmp_rank);
        let id = try alloc.alloc(usize, n);
        defer alloc.free(id);
        let cnt = try alloc.alloc(usize, m);
        defer alloc.free(cnt);

        // Initial counting sort by first byte
        let ci: usize = 0;
        while (ci < m) {
            cnt[ci] = 0;
            ci += 1;
        }
        let ti: usize = 0;
        while (ti < n) {
            const c: usize = @as(usize, text[ti]);
            rank[ti] = c;
            cnt[c] += 1;
            ti += 1;
        }
        let pi: usize = 1;
        while (pi < m) {
            cnt[pi] += cnt[pi - 1];
            pi += 1;
        }
        let ri = n;
        while (ri > 0) {
            ri -= 1;
            const r = rank[ri];
            cnt[r] -= 1;
            const pos = cnt[r];
            sa[pos] = ri;
        }

        // Prefix-doubling O(N log N) construction
        let k: usize = 1;
        while (k < n) {
            let p: usize = 0;
            let tail_i = n - k;
            while (tail_i < n) {
                id[p] = tail_i;
                p += 1;
                tail_i += 1;
            }
            let si: usize = 0;
            while (si < n) {
                if (sa[si] >= k) {
                    id[p] = sa[si] - k;
                    p += 1;
                }
                si += 1;
            }

            let zj: usize = 0;
            while (zj < m) {
                cnt[zj] = 0;
                zj += 1;
            }
            let cj: usize = 0;
            while (cj < n) {
                const r = rank[id[cj]];
                cnt[r] += 1;
                cj += 1;
            }
            let pj: usize = 1;
            while (pj < m) {
                cnt[pj] += cnt[pj - 1];
                pj += 1;
            }
            let rj = n;
            while (rj > 0) {
                rj -= 1;
                const pos = id[rj];
                const r = rank[pos];
                cnt[r] -= 1;
                const out_idx = cnt[r];
                sa[out_idx] = pos;
            }

            tmp_rank[sa[0]] = 0;
            let classes: usize = 1;
            let gj: usize = 1;
            while (gj < n) {
                const a = sa[gj - 1];
                const b = sa[gj];
                let same = (rank[a] == rank[b]);
                if (same) {
                    const a_has = (a + k < n);
                    const b_has = (b + k < n);
                    if (a_has != b_has) {
                        same = false;
                    } else if (a_has and b_has) {
                        same = (rank[a + k] == rank[b + k]);
                    }
                }
                if (!same) {
                    classes += 1;
                }
                tmp_rank[b] = classes - 1;
                gj += 1;
            }

            let cp_j: usize = 0;
            while (cp_j < n) {
                rank[cp_j] = tmp_rank[cp_j];
                cp_j += 1;
            }

            if (classes == n) {
                break;
            }
            m = classes;
            k *= 2;
        }

        // Kasai O(N) LCP array construction
        let ki: usize = 0;
        while (ki < n) {
            rank[sa[ki]] = ki;
            lcp[ki] = 0;
            ki += 1;
        }

        let h: usize = 0;
        let li: usize = 0;
        while (li < n) {
            const r = rank[li];
            if (r == 0) {
                h = 0;
                li += 1;
                continue;
            }
            const prev = sa[r - 1];
            while (li + h < n and prev + h < n and text[li + h] == text[prev + h]) {
                h += 1;
            }
            lcp[r] = h;
            if (h > 0) {
                h -= 1;
            }
            li += 1;
        }

        return SuffixArray{
            .alloc = alloc,
            .text = text,
            .sa = sa,
            .lcp = lcp,
        };
    }

    pub fn deinit() {
        _.alloc.free(_.sa);
        _.alloc.free(_.lcp);
    }

    pub fn len() -> usize {
        return _.text.len;
    }

    pub fn saSlice() -> []const usize {
        return _.sa;
    }

    pub fn lcpSlice() -> []const usize {
        return _.lcp;
    }

    fn comparePrefix(suffix_start: usize, pattern: []const u8) -> i32 {
        const suf = _.text[suffix_start..];
        const min_n = if (suf.len < pattern.len) suf.len else pattern.len;
        let i: usize = 0;
        while (i < min_n) {
            if (suf[i] < pattern[i]) {
                return -1;
            }
            if (suf[i] > pattern[i]) {
                return 1;
            }
            i += 1;
        }
        if (suf.len < pattern.len) {
            return -1;
        }
        return 0;
    }

    fn lowerBound(pattern: []const u8) -> usize {
        let lo: usize = 0;
        let hi: usize = _.sa.len;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            if (_.comparePrefix(_.sa[mid], pattern) < 0) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    }

    fn upperBound(pattern: []const u8) -> usize {
        let lo: usize = 0;
        let hi: usize = _.sa.len;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            if (_.comparePrefix(_.sa[mid], pattern) <= 0) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    }

    pub fn contains(pattern: []const u8) -> bool {
        if (pattern.len == 0 or _.sa.len == 0) {
            return false;
        }
        const lo = _.lowerBound(pattern);
        if (lo >= _.sa.len) {
            return false;
        }
        return _.comparePrefix(_.sa[lo], pattern) == 0;
    }

    pub fn countOccurrences(pattern: []const u8) -> usize {
        if (pattern.len == 0 or _.sa.len == 0) {
            return 0;
        }
        const lo = _.lowerBound(pattern);
        const hi = _.upperBound(pattern);
        if (hi <= lo) {
            return 0;
        }
        return hi - lo;
    }

    pub fn searchAll(alloc: std.mem.Allocator, pattern: []const u8) -> ![]usize {
        if (pattern.len == 0 or _.sa.len == 0) {
            return alloc.alloc(usize, 0);
        }
        const lo = _.lowerBound(pattern);
        const hi = _.upperBound(pattern);
        if (hi <= lo) {
            return alloc.alloc(usize, 0);
        }
        const count = hi - lo;
        let results = try alloc.alloc(usize, count);
        let i: usize = 0;
        while (i < count) {
            results[i] = _.sa[lo + i];
            i += 1;
        }
        return results;
    }

    pub fn longestRepeatedSubstring() -> []const u8 {
        if (_.lcp.len < 2) {
            return _.text[0..0];
        }
        let max_lcp: usize = 0;
        let max_idx: usize = 0;
        let i: usize = 1;
        while (i < _.lcp.len) {
            if (_.lcp[i] > max_lcp) {
                max_lcp = _.lcp[i];
                max_idx = i;
            }
            i += 1;
        }
        if (max_lcp == 0) {
            return _.text[0..0];
        }
        const start = _.sa[max_idx];
        return _.text[start .. start + max_lcp];
    }
}
