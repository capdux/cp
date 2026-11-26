// @cap/didyoumean — Zero-allocation spell-suggestion & edit-distance engine for CP
// Clean-room MIT implementation inspired by npm `didyoumean`, `fastest-levenshtein`, and Rust `strsim`.

const MAX_STR_LEN: usize = 128;

fn toLower(c: u8) -> u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn min3(a: usize, b: usize, c: usize) -> usize {
    let m = if (a < b) { a } else { b };
    return if (m < c) { m } else { c };
}

pub fn levenshtein(a: string, b: string) -> usize {
    const la = if (a.len > MAX_STR_LEN) { MAX_STR_LEN } else { a.len };
    const lb = if (b.len > MAX_STR_LEN) { MAX_STR_LEN } else { b.len };
    if (la == 0) {
        return lb;
    }
    if (lb == 0) {
        return la;
    }

    let prev: [MAX_STR_LEN + 1]usize = undefined;
    let curr: [MAX_STR_LEN + 1]usize = undefined;
    let j: usize = 0;
    while (j <= lb) {
        prev[j] = j;
        j += 1;
    }

    let i: usize = 1;
    while (i <= la) {
        curr[0] = i;
        let k: usize = 1;
        while (k <= lb) {
            const cost: usize = if (a[i - 1] == b[k - 1]) { 0 as usize } else { 1 as usize };
            curr[k] = min3(prev[k] + 1, curr[k - 1] + 1, prev[k - 1] + cost);
            k += 1;
        }
        let c: usize = 0;
        while (c <= lb) {
            prev[c] = curr[c];
            c += 1;
        }
        i += 1;
    }
    return prev[lb];
}

pub fn damerauLevenshtein(a: string, b: string, ignore_case: bool) -> usize {
    // Optimal String Alignment (adjacent transposition = cost 1), zero heap allocation
    const la = if (a.len > MAX_STR_LEN) { MAX_STR_LEN } else { a.len };
    const lb = if (b.len > MAX_STR_LEN) { MAX_STR_LEN } else { b.len };
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
        prev2[j] = j;
        j += 1;
    }

    let i: usize = 1;
    while (i <= la) {
        curr[0] = i;
        const ca = if (ignore_case) { toLower(a[i - 1]) } else { a[i - 1] };
        let pa: u8 = 0;
        if (i > 1) {
            if (ignore_case) {
                pa = toLower(a[i - 2]);
            } else {
                pa = a[i - 2];
            }
        }

        let k: usize = 1;
        while (k <= lb) {
            const cb = if (ignore_case) { toLower(b[k - 1]) } else { b[k - 1] };
            const cost: usize = if (ca == cb) { 0 as usize } else { 1 as usize };
            let best = min3(prev1[k] + 1, curr[k - 1] + 1, prev1[k - 1] + cost);
            if (i > 1 && k > 1) {
                const pb = if (ignore_case) { toLower(b[k - 2]) } else { b[k - 2] };
                if (ca == pb && pa == cb) {
                    const trans = prev2[k - 2] + 1;
                    if (trans < best) {
                        best = trans;
                    }
                }
            }
            curr[k] = best;
            k += 1;
        }
        let c: usize = 0;
        while (c <= lb) {
            prev2[c] = prev1[c];
            prev1[c] = curr[c];
            c += 1;
        }
        i += 1;
    }
    return prev1[lb];
}

pub fn similarity(a: string, b: string) -> f64 {
    if (a.len == 0 && b.len == 0) {
        return 1.0;
    }
    let max_len: usize = a.len;
    if (b.len > max_len) {
        max_len = b.len;
    }
    const dist = damerauLevenshtein(a, b, true);
    if (dist >= max_len) {
        return 0.0;
    }
    return 1.0 - ((dist as f64) / (max_len as f64));
}

pub struct Suggestion {
    pub found: bool,
    pub match_str: string,
    pub distance: usize,
    pub index: usize,
}

pub fn didYouMeanWith(input: string, candidates: []string, max_dist: usize) -> Suggestion {
    let best_idx: usize = 0;
    let best_dist: usize = max_dist + 1;
    let found = false;

    let i: usize = 0;
    while (i < candidates.len) {
        const d = damerauLevenshtein(input, candidates[i], true);
        if (d <= max_dist && d < best_dist) {
            best_dist = d;
            best_idx = i;
            found = true;
            if (d == 0) {
                break;
            }
        }
        i += 1;
    }

    if (!found) {
        return Suggestion{
            found: false,
            match_str: "",
            distance: best_dist,
            index: 0,
        };
    }
    return Suggestion{
        found: true,
        match_str: candidates[best_idx],
        distance: best_dist,
        index: best_idx,
    };
}

pub fn didYouMean(input: string, candidates: []string) -> string? {
    // Dynamic threshold: ~40% of input length (min 1, max 4)
    let threshold: usize = (input.len * 2) / 5;
    if (threshold < 1) {
        threshold = 1;
    }
    if (threshold > 4) {
        threshold = 4;
    }
    const s = didYouMeanWith(input, candidates, threshold);
    if (!s.found) {
        return null;
    }
    return s.match_str;
}

export {
    Suggestion,
    levenshtein,
    damerauLevenshtein,
    similarity,
    didYouMean,
    didYouMeanWith,
};
