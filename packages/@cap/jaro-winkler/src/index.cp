// @cap/jaro-winkler — Zero-allocation Jaro & Jaro-Winkler String Similarity Metrics
// Clean-room MIT implementation in pure CP.

const MAX_LEN: usize = 256;

/// Computes the Jaro similarity in `[0.0, 1.0]` between `a` and `b` (zero heap allocation,
/// truncates inputs longer than 256 bytes). Returns `1.0` for equal/both-empty strings and `0.0` if one is empty.
pub fn jaro(a: string, b: string) -> f64 {
    if (a.len == 0 && b.len == 0) {
        return 1.0;
    }
    if (a.len == 0 || b.len == 0) {
        return 0.0;
    }

    let la = a.len;
    if (la > MAX_LEN) { la = MAX_LEN; }
    let lb = b.len;
    if (lb > MAX_LEN) { lb = MAX_LEN; }

    // Fast path for identical strings
    if (la == lb) {
        let same = true;
        let i: usize = 0;
        while (i < la) {
            if (a[i] != b[i]) {
                same = false;
                break;
            }
            i += 1;
        }
        if (same) {
            return 1.0;
        }
    }

    let max_len = la;
    if (lb > max_len) {
        max_len = lb;
    }
    let match_dist: usize = 0;
    if (max_len >= 2) {
        match_dist = (max_len / 2) - 1;
    }

    let a_matched: [256]bool = undefined;
    let b_matched: [256]bool = undefined;
    let init_i: usize = 0;
    while (init_i < la) {
        a_matched[init_i] = false;
        init_i += 1;
    }
    init_i = 0;
    while (init_i < lb) {
        b_matched[init_i] = false;
        init_i += 1;
    }

    let matches: usize = 0;
    let i: usize = 0;
    while (i < la) {
        let lo: usize = 0;
        if (i > match_dist) {
            lo = i - match_dist;
        }
        let hi = i + match_dist + 1;
        if (hi > lb) {
            hi = lb;
        }

        let j = lo;
        while (j < hi) {
            if (!b_matched[j] && a[i] == b[j]) {
                a_matched[i] = true;
                b_matched[j] = true;
                matches += 1;
                break;
            }
            j += 1;
        }
        i += 1;
    }

    if (matches == 0) {
        return 0.0;
    }

    let transpositions_double: usize = 0;
    let bj: usize = 0;
    i = 0;
    while (i < la) {
        if (a_matched[i]) {
            while (bj < lb && !b_matched[bj]) {
                bj += 1;
            }
            if (bj < lb && a[i] != b[bj]) {
                transpositions_double += 1;
            }
            bj += 1;
        }
        i += 1;
    }

    const m = matches as f64;
    const t = (transpositions_double as f64) * 0.5;
    return (m / (la as f64) + m / (lb as f64) + (m - t) / m) / 3.0;
}

/// Computes the Jaro-Winkler similarity with a custom `prefix_scale` (clamped to `[0.0, 0.25]`),
/// applying Winkler prefix boost (up to 4 leading matching bytes) when `jaro(a, b) >= 0.7`.
pub fn jaroWinklerWithScale(a: string, b: string, prefix_scale: f64) -> f64 {
    const j = jaro(a, b);
    if (j < 0.7) {
        return j;
    }
    let p = prefix_scale;
    if (p < 0.0) {
        p = 0.0;
    }
    if (p > 0.25) {
        p = 0.25;
    }

    let max_pref: usize = 4;
    if (a.len < max_pref) { max_pref = a.len; }
    if (b.len < max_pref) { max_pref = b.len; }

    let pref: usize = 0;
    while (pref < max_pref && a[pref] == b[pref]) {
        pref += 1;
    }

    let score = j + (pref as f64) * p * (1.0 - j);
    if (score > 1.0) {
        score = 1.0;
    }
    return score;
}

/// Computes the standard Jaro-Winkler similarity (`prefix_scale = 0.1`, threshold `0.7`, max prefix `4`).
pub fn jaroWinkler(a: string, b: string) -> f64 {
    return jaroWinklerWithScale(a, b, 0.1);
}

/// Finds the index of the candidate in `candidates` with the highest `jaroWinkler` similarity
/// to `query` that is `>= min_score`. Returns `-1` if no candidate qualifies.
pub fn bestMatchIndex(query: string, candidates: []const string, min_score: f64) -> i64 {
    let best_idx: i64 = -1;
    let best_score: f64 = -1.0;

    let i: usize = 0;
    while (i < candidates.len) {
        const sc = jaroWinkler(query, candidates[i]);
        if (sc >= min_score && sc > best_score) {
            best_score = sc;
            best_idx = i as i64;
        }
        i += 1;
    }

    return best_idx;
}
