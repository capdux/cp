// @cap/levenshtein — Zero-heap Levenshtein & OSA Damerau-Levenshtein edit distance
// MIT License (clean-room CP implementation)

pub const MAX_LEN: usize = 256;

fn absDiff(a: usize, b: usize) -> usize {
    if (a >= b) {
        return a - b;
    }
    return b - a;
}

fn min3(a: usize, b: usize, c: usize) -> usize {
    let m: usize = a;
    if (b < m) {
        m = b;
    }
    if (c < m) {
        m = c;
    }
    return m;
}

pub fn distance(a: string, b: string) -> usize {
    // Strip common prefix
    let pfx: usize = 0;
    let min_l: usize = a.len;
    if (b.len < min_l) {
        min_l = b.len;
    }
    while (pfx < min_l && a[pfx] == b[pfx]) {
        pfx += 1;
    }

    let a_end: usize = a.len;
    let b_end: usize = b.len;
    while (a_end > pfx && b_end > pfx && a[a_end - 1] == b[b_end - 1]) {
        a_end -= 1;
        b_end -= 1;
    }

    let s1: string = a[pfx..a_end];
    let s2: string = b[pfx..b_end];
    if (s1.len == 0) {
        return s2.len;
    }
    if (s2.len == 0) {
        return s1.len;
    }

    // Ensure s2 is shorter to minimize rolling row size
    if (s2.len > s1.len) {
        const tmp: string = s1;
        s1 = s2;
        s2 = tmp;
    }

    let m: usize = s1.len;
    let n: usize = s2.len;
    if (m > MAX_LEN) {
        m = MAX_LEN;
    }
    if (n > MAX_LEN) {
        n = MAX_LEN;
    }

    let row: [257]usize = undefined;
    let j: usize = 0;
    while (j <= n) {
        row[j] = j;
        j += 1;
    }

    let i: usize = 1;
    while (i <= m) {
        let prev_diag: usize = row[0];
        row[0] = i;
        j = 1;
        while (j <= n) {
            const old_up: usize = row[j];
            let cost: usize = 1;
            if (s1[i - 1] == s2[j - 1]) {
                cost = 0;
            }
            row[j] = min3(old_up + 1, row[j - 1] + 1, prev_diag + cost);
            prev_diag = old_up;
            j += 1;
        }
        i += 1;
    }

    return row[n];
}

pub fn damerauDistance(a: string, b: string) -> usize {
    let pfx: usize = 0;
    let min_l: usize = a.len;
    if (b.len < min_l) {
        min_l = b.len;
    }
    while (pfx < min_l && a[pfx] == b[pfx]) {
        pfx += 1;
    }

    let a_end: usize = a.len;
    let b_end: usize = b.len;
    while (a_end > pfx && b_end > pfx && a[a_end - 1] == b[b_end - 1]) {
        a_end -= 1;
        b_end -= 1;
    }

    const s1: string = a[pfx..a_end];
    const s2: string = b[pfx..b_end];
    if (s1.len == 0) {
        return s2.len;
    }
    if (s2.len == 0) {
        return s1.len;
    }

    let m: usize = s1.len;
    let n: usize = s2.len;
    if (m > MAX_LEN) {
        m = MAX_LEN;
    }
    if (n > MAX_LEN) {
        n = MAX_LEN;
    }

    let prev2: [257]usize = undefined;
    let prev1: [257]usize = undefined;
    let curr: [257]usize = undefined;

    let j: usize = 0;
    while (j <= n) {
        prev1[j] = j;
        prev2[j] = 0;
        j += 1;
    }

    let i: usize = 1;
    while (i <= m) {
        curr[0] = i;
        j = 1;
        while (j <= n) {
            let cost: usize = 1;
            if (s1[i - 1] == s2[j - 1]) {
                cost = 0;
            }
            let best: usize = min3(prev1[j] + 1, curr[j - 1] + 1, prev1[j - 1] + cost);
            if (i > 1 && j > 1 && s1[i - 1] == s2[j - 2] && s1[i - 2] == s2[j - 1]) {
                const trans: usize = prev2[j - 2] + 1;
                if (trans < best) {
                    best = trans;
                }
            }
            curr[j] = best;
            j += 1;
        }
        j = 0;
        while (j <= n) {
            prev2[j] = prev1[j];
            prev1[j] = curr[j];
            j += 1;
        }
        i += 1;
    }

    return prev1[n];
}

pub fn similarity(a: string, b: string) -> f64 {
    let max_l: usize = a.len;
    if (b.len > max_l) {
        max_l = b.len;
    }
    if (max_l == 0) {
        return 1.0;
    }
    const d: usize = distance(a, b);
    if (d >= max_l) {
        return 0.0;
    }
    return 1.0 - (d as f64) / (max_l as f64);
}

pub fn withinDistance(a: string, b: string, max_dist: usize) -> bool {
    if (absDiff(a.len, b.len) > max_dist) {
        return false;
    }
    return distance(a, b) <= max_dist;
}

pub fn closestIndex(query: string, candidates: []const string, max_dist: usize) -> i64 {
    let best_idx: i64 = -1;
    let best_dist: usize = max_dist + 1;

    let i: usize = 0;
    while (i < candidates.len) {
        const cand: string = candidates[i];
        if (absDiff(query.len, cand.len) < best_dist) {
            const d: usize = distance(query, cand);
            if (d <= max_dist && d < best_dist) {
                best_dist = d;
                best_idx = i as i64;
                if (best_dist == 0) {
                    return best_idx;
                }
            }
        }
        i += 1;
    }
    return best_idx;
}
