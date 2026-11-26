// @cap/lcs-diff — Zero-heap LCS length, diff stats, similarity & Longest Common Substring
// MIT License (clean-room CP implementation)

pub const MAX_LEN: usize = 256;

pub struct SubstringMatch {
    pub a_start: usize,
    pub b_start: usize,
    pub len: usize,
}

pub struct DiffStats {
    pub common: usize,
    pub insertions: usize,
    pub deletions: usize,
}

pub fn lcsLength(a: string, b: string) -> usize {
    if (a.len == 0 || b.len == 0) {
        return 0;
    }
    let m: usize = a.len;
    let n: usize = b.len;
    if (m > MAX_LEN) {
        m = MAX_LEN;
    }
    if (n > MAX_LEN) {
        n = MAX_LEN;
    }

    let prev: [257]usize = undefined;
    let curr: [257]usize = undefined;
    let j: usize = 0;
    while (j <= n) {
        prev[j] = 0;
        curr[j] = 0;
        j += 1;
    }

    let i: usize = 1;
    while (i <= m) {
        curr[0] = 0;
        j = 1;
        while (j <= n) {
            if (a[i - 1] == b[j - 1]) {
                curr[j] = prev[j - 1] + 1;
            } else {
                let best: usize = prev[j];
                if (curr[j - 1] > best) {
                    best = curr[j - 1];
                }
                curr[j] = best;
            }
            j += 1;
        }
        j = 0;
        while (j <= n) {
            prev[j] = curr[j];
            j += 1;
        }
        i += 1;
    }

    return prev[n];
}

pub fn diffStats(a: string, b: string) -> DiffStats {
    let m: usize = a.len;
    let n: usize = b.len;
    if (m > MAX_LEN) {
        m = MAX_LEN;
    }
    if (n > MAX_LEN) {
        n = MAX_LEN;
    }
    const common: usize = lcsLength(a, b);
    return DiffStats {
        common: common,
        insertions: n - common,
        deletions: m - common,
    };
}

pub fn lcsSimilarity(a: string, b: string) -> f64 {
    if (a.len == 0 && b.len == 0) {
        return 1.0;
    }
    let m: usize = a.len;
    let n: usize = b.len;
    if (m > MAX_LEN) {
        m = MAX_LEN;
    }
    if (n > MAX_LEN) {
        n = MAX_LEN;
    }
    const total: usize = m + n;
    if (total == 0) {
        return 1.0;
    }
    const common: usize = lcsLength(a, b);
    return (2.0 * (common as f64)) / (total as f64);
}

pub fn longestCommonSubstring(a: string, b: string) -> SubstringMatch {
    let best_len: usize = 0;
    let best_a: usize = 0;
    let best_b: usize = 0;

    if (a.len == 0 || b.len == 0) {
        return SubstringMatch { a_start: 0, b_start: 0, len: 0 };
    }

    let m: usize = a.len;
    let n: usize = b.len;
    if (m > MAX_LEN) {
        m = MAX_LEN;
    }
    if (n > MAX_LEN) {
        n = MAX_LEN;
    }

    let prev: [257]usize = undefined;
    let curr: [257]usize = undefined;
    let j: usize = 0;
    while (j <= n) {
        prev[j] = 0;
        curr[j] = 0;
        j += 1;
    }

    let i: usize = 1;
    while (i <= m) {
        curr[0] = 0;
        j = 1;
        while (j <= n) {
            if (a[i - 1] == b[j - 1]) {
                const run: usize = prev[j - 1] + 1;
                curr[j] = run;
                if (run > best_len) {
                    best_len = run;
                    best_a = i - run;
                    best_b = j - run;
                }
            } else {
                curr[j] = 0;
            }
            j += 1;
        }
        j = 0;
        while (j <= n) {
            prev[j] = curr[j];
            j += 1;
        }
        i += 1;
    }

    return SubstringMatch {
        a_start: best_a,
        b_start: best_b,
        len: best_len,
    };
}
