// @cap/strsim — Zero-heap string similarity & edit-distance metrics modeled on Rust's `strsim` crate.
// Clean-room MIT implementation (Hamming, Levenshtein, OSA Damerau-Levenshtein,
// Normalized Levenshtein, Jaro, Jaro-Winkler, and Sorensen-Dice bigram similarity).

pub const STRSIM_MAX_LEN: usize = 128;

fn ssmMin3(a: usize, b: usize, c: usize): usize {
    let m = if (a < b) { a } else { b };
    return if (m < c) { m } else { c };
}

/// Hamming distance between equal-length strings; returns `null` if lengths differ.
pub fn hamming(a: string, b: string): usize? {
    if (a.len != b.len) { return null; }
    let dist: usize = 0;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { dist = dist + 1; }
        i = i + 1;
    }
    return dist;
}

/// Levenshtein edit distance (O(min(n,m)) space, zero heap, supports strings up to 128 bytes).
pub fn levenshtein(a: string, b: string): usize {
    let n = if (a.len > STRSIM_MAX_LEN) { STRSIM_MAX_LEN } else { a.len };
    let m = if (b.len > STRSIM_MAX_LEN) { STRSIM_MAX_LEN } else { b.len };
    if (n == 0) { return m; }
    if (m == 0) { return n; }

    let prev: [129]usize = undefined;
    let curr: [129]usize = undefined;
    let j: usize = 0;
    while (j <= m) {
        prev[j] = j;
        j = j + 1;
    }

    let i: usize = 1;
    while (i <= n) {
        curr[0] = i;
        let k: usize = 1;
        while (k <= m) {
            let cost: usize = if (a[i - 1] == b[k - 1]) { 0 } else { 1 };
            curr[k] = ssmMin3(prev[k] + 1, curr[k - 1] + 1, prev[k - 1] + cost);
            k = k + 1;
        }
        let cpy: usize = 0;
        while (cpy <= m) {
            prev[cpy] = curr[cpy];
            cpy = cpy + 1;
        }
        i = i + 1;
    }
    return prev[m];
}

/// Optimal String Alignment (restricted Damerau-Levenshtein) distance with adjacent transposition cost 1.
pub fn osaDistance(a: string, b: string): usize {
    let n = if (a.len > STRSIM_MAX_LEN) { STRSIM_MAX_LEN } else { a.len };
    let m = if (b.len > STRSIM_MAX_LEN) { STRSIM_MAX_LEN } else { b.len };
    if (n == 0) { return m; }
    if (m == 0) { return n; }

    let prev2: [129]usize = undefined;
    let prev1: [129]usize = undefined;
    let curr: [129]usize = undefined;
    let j: usize = 0;
    while (j <= m) {
        prev1[j] = j;
        prev2[j] = 0;
        j = j + 1;
    }

    let i: usize = 1;
    while (i <= n) {
        curr[0] = i;
        let k: usize = 1;
        while (k <= m) {
            let cost: usize = if (a[i - 1] == b[k - 1]) { 0 } else { 1 };
            let best = ssmMin3(prev1[k] + 1, curr[k - 1] + 1, prev1[k - 1] + cost);
            if (i > 1 && k > 1 && a[i - 1] == b[k - 2] && a[i - 2] == b[k - 1]) {
                let trans = prev2[k - 2] + 1;
                if (trans < best) { best = trans; }
            }
            curr[k] = best;
            k = k + 1;
        }
        let cpy: usize = 0;
        while (cpy <= m) {
            prev2[cpy] = prev1[cpy];
            prev1[cpy] = curr[cpy];
            cpy = cpy + 1;
        }
        i = i + 1;
    }
    return prev1[m];
}

/// Normalized Levenshtein similarity in basis points `0..10000` (10000 = identical).
pub fn normalizedLevenshteinBps(a: string, b: string): u32 {
    if (a.len == 0 && b.len == 0) { return 10000; }
    let max_len: u64 = (if (a.len > b.len) { a.len } else { b.len }) as u64;
    let dist: u64 = levenshtein(a, b) as u64;
    if (dist >= max_len) { return 0; }
    return (((max_len - dist) * 10000) / max_len) as u32;
}

/// Jaro similarity in basis points `0..10000` (10000 = identical).
pub fn jaroBps(a: string, b: string): u32 {
    let n = if (a.len > STRSIM_MAX_LEN) { STRSIM_MAX_LEN } else { a.len };
    let m = if (b.len > STRSIM_MAX_LEN) { STRSIM_MAX_LEN } else { b.len };
    if (n == 0 && m == 0) { return 10000; }
    if (n == 0 || m == 0) { return 0; }

    let max_nm = if (n > m) { n } else { m };
    let match_dist: usize = if (max_nm >= 2) { (max_nm / 2) - 1 } else { 0 };

    let a_matched: [128]bool = undefined;
    let b_matched: [128]bool = undefined;
    let init_i: usize = 0;
    while (init_i < n) { a_matched[init_i] = false; init_i = init_i + 1; }
    let init_j: usize = 0;
    while (init_j < m) { b_matched[init_j] = false; init_j = init_j + 1; }

    let matches: u64 = 0;
    let i: usize = 0;
    while (i < n) {
        let start = if (i > match_dist) { i - match_dist } else { 0 };
        let end_excl = if (i + match_dist + 1 < m) { i + match_dist + 1 } else { m };
        let j = start;
        while (j < end_excl) {
            if (!b_matched[j] && a[i] == b[j]) {
                a_matched[i] = true;
                b_matched[j] = true;
                matches = matches + 1;
                break;
            }
            j = j + 1;
        }
        i = i + 1;
    }
    if (matches == 0) { return 0; }

    let transpositions: u64 = 0;
    let k: usize = 0;
    let ii: usize = 0;
    while (ii < n) {
        if (a_matched[ii]) {
            while (k < m && !b_matched[k]) { k = k + 1; }
            if (k < m && a[ii] != b[k]) {
                transpositions = transpositions + 1;
            }
            k = k + 1;
        }
        ii = ii + 1;
    }

    let t = transpositions / 2;
    let term1 = (matches * 10000) / (n as u64);
    let term2 = (matches * 10000) / (m as u64);
    let term3 = ((matches - t) * 10000) / matches;
    return ((term1 + term2 + term3) / 3) as u32;
}

/// Jaro-Winkler similarity in basis points `0..10000` (prefix boost p=0.1, up to 4 chars).
pub fn jaroWinklerBps(a: string, b: string): u32 {
    let jaro: u64 = jaroBps(a, b) as u64;
    if (jaro <= 7000) { return jaro as u32; }
    let max_pref = if (a.len < b.len) { a.len } else { b.len };
    let limit = if (max_pref > 4) { 4 } else { max_pref };
    let pref: u64 = 0;
    let i: usize = 0;
    while (i < limit && a[i] == b[i]) {
        pref = pref + 1;
        i = i + 1;
    }
    let boost = (pref * (10000 - jaro)) / 10;
    let res = jaro + boost;
    return if (res > 10000) { 10000 } else { res as u32 };
}

/// Sorensen-Dice bigram similarity in basis points `0..10000`.
pub fn sorensenDiceBps(a: string, b: string): u32 {
    let n = if (a.len > STRSIM_MAX_LEN) { STRSIM_MAX_LEN } else { a.len };
    let m = if (b.len > STRSIM_MAX_LEN) { STRSIM_MAX_LEN } else { b.len };
    if (n == 0 && m == 0) { return 10000; }
    if (n < 2 || m < 2) {
        if (n == 1 && m == 1 && a[0] == b[0]) { return 10000; }
        return 0;
    }
    let used: [128]bool = undefined;
    let init_j: usize = 0;
    while (init_j < m - 1) { used[init_j] = false; init_j = init_j + 1; }

    let inter: u64 = 0;
    let i: usize = 0;
    while (i + 1 < n) {
        let j: usize = 0;
        while (j + 1 < m) {
            if (!used[j] && a[i] == b[j] && a[i + 1] == b[j + 1]) {
                used[j] = true;
                inter = inter + 1;
                break;
            }
            j = j + 1;
        }
        i = i + 1;
    }
    let total: u64 = ((n - 1) + (m - 1)) as u64;
    return ((2 * inter * 10000) / total) as u32;
}
