// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Suffix Array, Kasai LCP, Binary-Search Pattern Index & Substring Analytics)

/// Caller-owned zero-alloc workspace for `buildSuffixArray`.
/// Required lengths for input length `n`:
/// - `rank.len >= n`, `tmp_rank.len >= n`, `order.len >= n`
/// - `cnt.len >= max(n + 1, 257)`
pub struct SuffixWorkspace {
    rank: []usize,
    tmp_rank: []usize,
    cnt: []usize,
    order: []usize,
}

/// Half-open match interval `[start, end)` inside the suffix array returned by `findPatternRange`.
pub struct OccurrenceRange {
    start: usize,
    end: usize,
    count: usize,
}

/// Starting position and length of a longest repeated substring.
pub struct LongestRepeat {
    pos: usize,
    len: usize,
}

/// Builds the suffix array `sa_out[0..text.len]` in O(n log n) time using prefix-doubling radix sort.
/// Supports arbitrary binary byte strings including `0x00` and `0xFF`.
/// Returns `false` if `sa_out` or any slice in `ws` is undersized.
pub fn buildSuffixArray(
    text: []const u8,
    sa_out: *[]usize,
    ws: *SuffixWorkspace,
): bool {
    const n = text.len;
    if (n == 0) {
        return true;
    }

    const min_cnt: usize = if (n + 1 > 257) n + 1 else 257;
    if (
        sa_out.*.len < n ||
        ws.rank.len < n ||
        ws.tmp_rank.len < n ||
        ws.order.len < n ||
        ws.cnt.len < min_cnt
    ) {
        return false;
    }

    if (n == 1) {
        sa_out.*[0] = 0;
        return true;
    }

    // Initial counting sort on single bytes mapped to 1..256 (0 reserved for out-of-bounds sentinel)
    let c: usize = 0;
    while (c < 257) {
        ws.cnt[c] = 0;
        c += 1;
    }

    let i: usize = 0;
    while (i < n) {
        const key = (text[i] as usize) + 1;
        ws.rank[i] = key;
        ws.cnt[key] += 1;
        i += 1;
    }

    c = 1;
    while (c < 257) {
        ws.cnt[c] += ws.cnt[c - 1];
        c += 1;
    }

    let rev: usize = n;
    while (rev > 0) {
        rev -= 1;
        const key = ws.rank[rev];
        ws.cnt[key] -= 1;
        sa_out.*[ws.cnt[key]] = rev;
    }

    // Compress initial equivalence classes into 1..classes
    ws.tmp_rank[sa_out.*[0]] = 1;
    let classes: usize = 1;
    i = 1;
    while (i < n) {
        const prev = sa_out.*[i - 1];
        const cur = sa_out.*[i];
        if (text[prev] != text[cur]) {
            classes += 1;
        }
        ws.tmp_rank[cur] = classes;
        i += 1;
    }

    i = 0;
    while (i < n) {
        ws.rank[i] = ws.tmp_rank[i];
        i += 1;
    }

    if (classes == n) {
        return true;
    }

    // Prefix doubling loop
    let step: usize = 1;
    while (step < n) {
        // Order suffixes by second key (rank[i + step], with 0 for i + step >= n)
        let p: usize = 0;
        let tail_idx: usize = n - step;
        while (tail_idx < n) {
            ws.order[p] = tail_idx;
            p += 1;
            tail_idx += 1;
        }

        let sa_i: usize = 0;
        while (sa_i < n) {
            if (sa_out.*[sa_i] >= step) {
                ws.order[p] = sa_out.*[sa_i] - step;
                p += 1;
            }
            sa_i += 1;
        }

        // Stable counting sort by first key (rank[order[i]])
        const alpha = classes + 1;
        let ci: usize = 0;
        while (ci < alpha) {
            ws.cnt[ci] = 0;
            ci += 1;
        }

        let oi: usize = 0;
        while (oi < n) {
            ws.cnt[ws.rank[ws.order[oi]]] += 1;
            oi += 1;
        }

        ci = 1;
        while (ci < alpha) {
            ws.cnt[ci] += ws.cnt[ci - 1];
            ci += 1;
        }

        let r_oi: usize = n;
        while (r_oi > 0) {
            r_oi -= 1;
            const suf = ws.order[r_oi];
            const rk = ws.rank[suf];
            ws.cnt[rk] -= 1;
            sa_out.*[ws.cnt[rk]] = suf;
        }

        // Recompute compressed equivalence classes
        ws.tmp_rank[sa_out.*[0]] = 1;
        classes = 1;
        let k: usize = 1;
        while (k < n) {
            const prev = sa_out.*[k - 1];
            const cur = sa_out.*[k];
            const prev_r2 = if (prev + step < n) ws.rank[prev + step] else 0;
            const cur_r2 = if (cur + step < n) ws.rank[cur + step] else 0;
            if (ws.rank[prev] != ws.rank[cur] || prev_r2 != cur_r2) {
                classes += 1;
            }
            ws.tmp_rank[cur] = classes;
            k += 1;
        }

        let cp: usize = 0;
        while (cp < n) {
            ws.rank[cp] = ws.tmp_rank[cp];
            cp += 1;
        }

        if (classes == n) {
            break;
        }
        step <<= 1;
    }

    return true;
}

/// Builds the LCP array `lcp_out[0..text.len]` in O(n) time via Kasai's algorithm,
/// where `lcp_out[0] = 0` and `lcp_out[i] = lcp(text[sa[i-1]..], text[sa[i]..])`.
/// Returns `false` if buffers are undersized or `sa` contains an out-of-range suffix index.
pub fn buildLcpArray(
    text: []const u8,
    sa: []const usize,
    lcp_out: *[]usize,
    rank_scratch: *[]usize,
): bool {
    const n = text.len;
    if (n == 0) {
        return true;
    }
    if (sa.len < n || lcp_out.*.len < n || rank_scratch.*.len < n) {
        return false;
    }

    let i: usize = 0;
    while (i < n) {
        if (sa[i] >= n) {
            return false;
        }
        rank_scratch.*[sa[i]] = i;
        i += 1;
    }

    lcp_out.*[0] = 0;
    let h: usize = 0;
    let pos: usize = 0;
    while (pos < n) {
        const r = rank_scratch.*[pos];
        if (r == 0) {
            h = 0;
            pos += 1;
            continue;
        }
        const prev_pos = sa[r - 1];
        while (pos + h < n && prev_pos + h < n && text[pos + h] == text[prev_pos + h]) {
            h += 1;
        }
        lcp_out.*[r] = h;
        if (h > 0) {
            h -= 1;
        }
        pos += 1;
    }

    return true;
}

fn compareSuffixPrefix(text: []const u8, suffix_pos: usize, pattern: []const u8): i32 {
    const n = text.len;
    const m = pattern.len;
    let i: usize = 0;
    while (i < m && suffix_pos + i < n) {
        const a = text[suffix_pos + i];
        const b = pattern[i];
        if (a < b) {
            return -1;
        }
        if (a > b) {
            return 1;
        }
        i += 1;
    }
    if (i == m) {
        return 0;
    }
    return -1;
}

/// Locates all occurrences of `pattern` in `text` via two O(|P| log n) binary searches over `sa`.
/// Returns half-open range `[start, end)` in `sa` with `count == end - start`.
/// For an empty pattern, returns `[0, text.len)`.
pub fn findPatternRange(
    text: []const u8,
    sa: []const usize,
    pattern: []const u8,
): OccurrenceRange {
    const n = text.len;
    if (n == 0 || sa.len < n) {
        return OccurrenceRange { start: 0, end: 0, count: 0 };
    }
    if (pattern.len == 0) {
        return OccurrenceRange { start: 0, end: n, count: n };
    }

    // Lower bound: first index where suffix >= pattern
    let lo: usize = 0;
    let hi: usize = n;
    while (lo < hi) {
        const mid = lo + ((hi - lo) >> 1);
        if (compareSuffixPrefix(text, sa[mid], pattern) < 0) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    const start = lo;

    // Upper bound: first index where suffix > pattern
    lo = start;
    hi = n;
    while (lo < hi) {
        const mid = lo + ((hi - lo) >> 1);
        if (compareSuffixPrefix(text, sa[mid], pattern) <= 0) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    const end = lo;

    return OccurrenceRange {
        start: start,
        end: end,
        count: end - start,
    };
}

/// Computes the exact number of distinct non-empty substrings: `n*(n+1)/2 - sum(lcp[1..n])`.
pub fn countDistinctSubstrings(n: usize, lcp: []const usize): u64 {
    if (n == 0 || lcp.len < n) {
        return 0;
    }
    const n64 = n as u64;
    const total = (n64 * (n64 + 1)) / 2;
    let lcp_sum: u64 = 0;
    let i: usize = 1;
    while (i < n) {
        lcp_sum += lcp[i] as u64;
        i += 1;
    }
    if (lcp_sum >= total) {
        return 0;
    }
    return total - lcp_sum;
}

/// Finds the longest repeated substring from the suffix and LCP arrays.
/// Returns `{ pos: 0, len: 0 }` if no repeated substring exists.
pub fn longestRepeatedSubstring(sa: []const usize, lcp: []const usize): LongestRepeat {
    const n = if (sa.len < lcp.len) sa.len else lcp.len;
    if (n < 2) {
        return LongestRepeat { pos: 0, len: 0 };
    }

    let best_len: usize = 0;
    let best_pos: usize = 0;
    let i: usize = 1;
    while (i < n) {
        if (lcp[i] > best_len) {
            best_len = lcp[i];
            best_pos = sa[i];
        }
        i += 1;
    }

    return LongestRepeat {
        pos: best_pos,
        len: best_len,
    };
}
