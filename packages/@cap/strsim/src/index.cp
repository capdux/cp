// @cap/strsim — Zero-allocation 7-in-1 string distance & similarity metrics for CP
// Clean-room MIT implementation inspired by Rust `strsim` (crates.io #46, 1.325B+ downloads).
//
// Included algorithms (all zero heap allocation with fixed stack buffers, max 256 bytes):
// 1. `hamming(a, b) -> usize?`
// 2. `levenshtein(a, b) -> usize`
// 3. `normalizedLevenshtein(a, b) -> f64`
// 4. `osaDistance(a, b) -> usize` (Optimal String Alignment / restricted Damerau-Levenshtein)
// 5. `jaro(a, b) -> f64`
// 6. `jaroWinkler(a, b) -> f64`
// 7. `sorensenDice(a, b) -> f64`

const MAX_STR_LEN: usize = 256;

fn clampLen(n: usize) -> usize {
    if (n > MAX_STR_LEN) {
        return MAX_STR_LEN;
    }
    return n;
}

fn strEq(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Computes Hamming distance between equal-length strings.
/// Returns `null` if `a.len != b.len`.
pub fn hamming(a: string, b: string) -> usize? {
    if (a.len != b.len) {
        return null;
    }
    let dist: usize = 0;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            dist += 1;
        }
        i += 1;
    }
    return dist;
}

/// Computes Levenshtein edit distance (insertions, deletions, substitutions) with O(min(m, n)) stack memory.
pub fn levenshtein(a: string, b: string) -> usize {
    const la = clampLen(a.len);
    const lb = clampLen(b.len);
    if (la == 0) {
        return lb;
    }
    if (lb == 0) {
        return la;
    }

    let row: [MAX_STR_LEN + 1]usize = undefined;
    let j: usize = 0;
    while (j <= lb) {
        row[j] = j;
        j += 1;
    }

    let i: usize = 1;
    while (i <= la) {
        let prev_diag = row[0];
        row[0] = i;
        j = 1;
        while (j <= lb) {
            const old_val = row[j];
            const cost: usize = if (a[i - 1] == b[j - 1]) { 0 as usize } else { 1 as usize };
            let best = old_val + 1; // deletion
            const ins = row[j - 1] + 1; // insertion
            if (ins < best) {
                best = ins;
            }
            const sub = prev_diag + cost; // substitution
            if (sub < best) {
                best = sub;
            }
            row[j] = best;
            prev_diag = old_val;
            j += 1;
        }
        i += 1;
    }

    return row[lb];
}

/// Computes normalized Levenshtein similarity in `[0.0, 1.0]`. Returns `1.0` for two empty strings.
pub fn normalizedLevenshtein(a: string, b: string) -> f64 {
    const la = clampLen(a.len);
    const lb = clampLen(b.len);
    if (la == 0 && lb == 0) {
        return 1.0;
    }
    const max_len: usize = if (la > lb) { la } else { lb };
    const dist: usize = levenshtein(a, b);
    return 1.0 - ((dist as f64) / (max_len as f64));
}

/// Computes Optimal String Alignment (OSA / restricted Damerau-Levenshtein) distance,
/// allowing adjacent character transpositions at cost 1.
pub fn osaDistance(a: string, b: string) -> usize {
    const la = clampLen(a.len);
    const lb = clampLen(b.len);
    if (la == 0) {
        return lb;
    }
    if (lb == 0) {
        return la;
    }

    let prev2: [MAX_STR_LEN + 1]usize = undefined;
    let prev1: [MAX_STR_LEN + 1]usize = undefined;
    let curr: [MAX_STR_LEN + 1]usize = undefined;

    let j: usize = 0;
    while (j <= lb) {
        prev1[j] = j;
        prev2[j] = 0;
        j += 1;
    }

    let i: usize = 1;
    while (i <= la) {
        curr[0] = i;
        j = 1;
        while (j <= lb) {
            const cost: usize = if (a[i - 1] == b[j - 1]) { 0 as usize } else { 1 as usize };
            let best = prev1[j] + 1; // deletion
            const ins = curr[j - 1] + 1; // insertion
            if (ins < best) {
                best = ins;
            }
            const sub = prev1[j - 1] + cost; // substitution
            if (sub < best) {
                best = sub;
            }
            // Adjacent transposition
            if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1]) {
                const trans = prev2[j - 2] + 1;
                if (trans < best) {
                    best = trans;
                }
            }
            curr[j] = best;
            j += 1;
        }
        // Rotate rows: prev2 <- prev1, prev1 <- curr
        j = 0;
        while (j <= lb) {
            prev2[j] = prev1[j];
            prev1[j] = curr[j];
            j += 1;
        }
        i += 1;
    }

    return prev1[lb];
}

/// Computes Jaro similarity in `[0.0, 1.0]` with match window `max(len_a, len_b)/2 - 1`.
pub fn jaro(a: string, b: string) -> f64 {
    const la = clampLen(a.len);
    const lb = clampLen(b.len);
    if (la == 0 && lb == 0) {
        return 1.0;
    }
    if (la == 0 || lb == 0) {
        return 0.0;
    }

    const max_len = if (la > lb) { la } else { lb };
    const match_window: usize = if (max_len >= 2) { (max_len / 2) - 1 } else { 0 };

    let a_matched: [MAX_STR_LEN]bool = undefined;
    let b_matched: [MAX_STR_LEN]bool = undefined;
    let idx: usize = 0;
    while (idx < la) {
        a_matched[idx] = false;
        idx += 1;
    }
    idx = 0;
    while (idx < lb) {
        b_matched[idx] = false;
        idx += 1;
    }

    let matches: usize = 0;
    let i: usize = 0;
    while (i < la) {
        const lo: usize = if (i > match_window) { i - match_window } else { 0 };
        const hi: usize = if (i + match_window + 1 < lb) { i + match_window + 1 } else { lb };
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

    let transpositions: usize = 0;
    let k: usize = 0;
    i = 0;
    while (i < la) {
        if (a_matched[i]) {
            while (k < lb && !b_matched[k]) {
                k += 1;
            }
            if (k < lb) {
                if (a[i] != b[k]) {
                    transpositions += 1;
                }
                k += 1;
            }
        }
        i += 1;
    }

    const m = matches as f64;
    const t = (transpositions as f64) / 2.0;
    return ((m / (la as f64)) + (m / (lb as f64)) + ((m - t) / m)) / 3.0;
}

/// Computes Jaro-Winkler similarity in `[0.0, 1.0]` with prefix scaling `p = 0.1`
/// (up to 4 matching prefix characters, applied when `jaro(a, b) > 0.7`).
pub fn jaroWinkler(a: string, b: string) -> f64 {
    const j = jaro(a, b);
    if (j <= 0.7) {
        return j;
    }
    const min_len = if (a.len < b.len) { a.len } else { b.len };
    const max_prefix = if (min_len < 4) { min_len } else { 4 };
    let prefix: usize = 0;
    while (prefix < max_prefix && a[prefix] == b[prefix]) {
        prefix += 1;
    }
    const res = j + (0.1 * (prefix as f64) * (1.0 - j));
    if (res > 1.0) {
        return 1.0;
    }
    return res;
}

/// Computes Sørensen-Dice coefficient in `[0.0, 1.0]` over character bigrams (2-grams).
pub fn sorensenDice(a: string, b: string) -> f64 {
    if (strEq(a, b)) {
        return 1.0;
    }
    const la = clampLen(a.len);
    const lb = clampLen(b.len);
    if (la < 2 || lb < 2) {
        return 0.0;
    }

    const count_a = la - 1;
    const count_b = lb - 1;

    let b_used: [MAX_STR_LEN]bool = undefined;
    let j: usize = 0;
    while (j < count_b) {
        b_used[j] = false;
        j += 1;
    }

    let intersection: usize = 0;
    let i: usize = 0;
    while (i < count_a) {
        const a0 = a[i];
        const a1 = a[i + 1];
        j = 0;
        while (j < count_b) {
            if (!b_used[j] && b[j] == a0 && b[j + 1] == a1) {
                b_used[j] = true;
                intersection += 1;
                break;
            }
            j += 1;
        }
        i += 1;
    }

    const total_bigrams: usize = count_a + count_b;
    return (2.0 * (intersection as f64)) / (total_bigrams as f64);
}
