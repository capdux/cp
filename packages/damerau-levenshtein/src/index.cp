// damerau-levenshtein — Zero-heap OSA & True Damerau-Levenshtein edit distance in pure CP
// Clean-room MIT implementation of npm `damerau-levenshtein` (#888, 64.6M/wk).

pub const DL_MAX_OSA_LEN: usize = 255;
pub const DL_MAX_TRUE_LEN: usize = 64;

pub struct DamLevResult {
    pub steps: usize,
    pub relative: f64,   // steps / max(len_a, len_b)
    pub similarity: f64, // 1.0 - relative
}

fn dlvMin3(a: usize, b: usize, c: usize): usize {
    let m = a;
    if (b < m) { m = b; }
    if (c < m) { m = c; }
    return m;
}

fn dlvMin4(a: usize, b: usize, c: usize, d: usize): usize {
    let m = dlvMin3(a, b, c);
    if (d < m) { m = d; }
    return m;
}

fn dlvAbsDiff(a: usize, b: usize): usize {
    return if (a >= b) a - b else b - a;
}

fn dlvBuildResult(steps: usize, len_a: usize, len_b: usize): DamLevResult {
    let max_len = len_a;
    if (len_b > max_len) { max_len = len_b; }
    if (max_len == 0) {
        return DamLevResult { steps: 0, relative: 0.0, similarity: 1.0 };
    }
    const rel = (steps as f64) / (max_len as f64);
    let sim = 1.0 - rel;
    if (sim < 0.0) { sim = 0.0; }
    return DamLevResult {
        steps: steps,
        relative: rel,
        similarity: sim,
    };
}

/// Computes Optimal String Alignment (restricted Damerau-Levenshtein) distance on byte slices
/// supporting insertion, deletion, substitution, and adjacent transposition (up to 255 bytes).
pub fn damerauLevenshteinStepsBytes(a: []const u8, b: []const u8): usize {
    if (a.len == 0) { return b.len; }
    if (b.len == 0) { return a.len; }

    let m = a.len;
    let n = b.len;
    if (m > DL_MAX_OSA_LEN) { m = DL_MAX_OSA_LEN; }
    if (n > DL_MAX_OSA_LEN) { n = DL_MAX_OSA_LEN; }

    // 3 rolling rows (prev2, prev1, cur) of width 256 — strictly written before read
    let row0: [256]usize = undefined;
    let row1: [256]usize = undefined;
    let row2: [256]usize = undefined;

    let j: usize = 0;
    while (j <= n) {
        row1[j] = j;
        j += 1;
    }

    let i: usize = 1;
    while (i <= m) {
        row2[0] = i;
        let k: usize = 1;
        while (k <= n) {
            const cost: usize = if (a[i - 1] == b[k - 1]) 0 else 1;
            let best = dlvMin3(
                row1[k] + 1,        // deletion
                row2[k - 1] + 1,    // insertion
                row1[k - 1] + cost, // substitution
            );
            // Adjacent transposition
            if (i > 1 && k > 1 && a[i - 1] == b[k - 2] && a[i - 2] == b[k - 1]) {
                const trans = row0[k - 2] + 1;
                if (trans < best) {
                    best = trans;
                }
            }
            row2[k] = best;
            k += 1;
        }
        // Shift rows: row0 <- row1, row1 <- row2
        let c: usize = 0;
        while (c <= n) {
            row0[c] = row1[c];
            row1[c] = row2[c];
            c += 1;
        }
        i += 1;
    }
    return row1[n];
}

/// Computes Optimal String Alignment (Damerau-Levenshtein) edit distance supporting
/// insertion, deletion, substitution, and adjacent character transposition (up to 255 bytes per string).
pub fn damerauLevenshteinSteps(a: string, b: string): usize {
    return damerauLevenshteinStepsBytes(a, b);
}

/// Alias for Optimal String Alignment (restricted Damerau-Levenshtein) distance on byte slices.
pub fn osaDistanceBytes(a: []const u8, b: []const u8): usize {
    return damerauLevenshteinStepsBytes(a, b);
}

/// Alias for Optimal String Alignment (restricted Damerau-Levenshtein) distance on strings.
pub fn osaDistance(a: string, b: string): usize {
    return damerauLevenshteinStepsBytes(a, b);
}

/// Computes True (unrestricted) Damerau-Levenshtein distance using the Lowrance-Wagner algorithm
/// on byte slices (up to 64 bytes; falls back to OSA above 64 bytes).
/// Unlike OSA, allows substrings to be edited more than once (e.g., `"ca"` -> `"abc"` = 2).
pub fn trueDamerauLevenshteinBytes(a: []const u8, b: []const u8): usize {
    if (a.len == 0) { return b.len; }
    if (b.len == 0) { return a.len; }
    if (a.len > DL_MAX_TRUE_LEN || b.len > DL_MAX_TRUE_LEN) {
        return damerauLevenshteinStepsBytes(a, b);
    }

    const m = a.len;
    const n = b.len;
    const stride: usize = n + 2;
    const maxdist: usize = m + n;

    // Flat stack matrix (m + 2) x (n + 2), max 66 x 66 = 4356 entries
    let d: [4356]usize = undefined;
    let da: [256]usize = [0; 256];

    d[0] = maxdist;
    let i: usize = 0;
    while (i <= m) {
        d[(i + 1) * stride + 0] = maxdist;
        d[(i + 1) * stride + 1] = i;
        i += 1;
    }
    let j: usize = 0;
    while (j <= n) {
        d[0 * stride + (j + 1)] = maxdist;
        d[1 * stride + (j + 1)] = j;
        j += 1;
    }

    i = 1;
    while (i <= m) {
        let db: usize = 0;
        j = 1;
        while (j <= n) {
            const k = da[b[j - 1] as usize];
            const l = db;
            let cost: usize = 1;
            if (a[i - 1] == b[j - 1]) {
                cost = 0;
                db = j;
            }
            const subst = d[i * stride + j] + cost;
            const ins = d[(i + 1) * stride + j] + 1;
            const del = d[i * stride + (j + 1)] + 1;
            const trans = d[k * stride + l] + (i - k - 1) + 1 + (j - l - 1);
            d[(i + 1) * stride + (j + 1)] = dlvMin4(subst, ins, del, trans);
            j += 1;
        }
        da[a[i - 1] as usize] = i;
        i += 1;
    }

    return d[(m + 1) * stride + (n + 1)];
}

/// Computes True (unrestricted) Damerau-Levenshtein distance on strings.
pub fn trueDamerauLevenshteinSteps(a: string, b: string): usize {
    return trueDamerauLevenshteinBytes(a, b);
}

/// Computes OSA edit distance on byte slices with early-exit pruning when distance exceeds `maxDistance`.
/// Returns `null` if the edit distance is strictly greater than `maxDistance`.
pub fn damerauLevenshteinBoundedBytes(a: []const u8, b: []const u8, maxDistance: usize): usize? {
    if (dlvAbsDiff(a.len, b.len) > maxDistance) {
        return null;
    }
    if (a.len == 0) {
        return if (b.len <= maxDistance) b.len else null;
    }
    if (b.len == 0) {
        return if (a.len <= maxDistance) a.len else null;
    }

    let m = a.len;
    let n = b.len;
    if (m > DL_MAX_OSA_LEN) { m = DL_MAX_OSA_LEN; }
    if (n > DL_MAX_OSA_LEN) { n = DL_MAX_OSA_LEN; }

    let row0: [256]usize = undefined;
    let row1: [256]usize = undefined;
    let row2: [256]usize = undefined;

    let j: usize = 0;
    while (j <= n) {
        row1[j] = j;
        j += 1;
    }

    let i: usize = 1;
    while (i <= m) {
        row2[0] = i;
        let row_min: usize = row2[0];
        let k: usize = 1;
        while (k <= n) {
            const cost: usize = if (a[i - 1] == b[k - 1]) 0 else 1;
            let best = dlvMin3(
                row1[k] + 1,
                row2[k - 1] + 1,
                row1[k - 1] + cost,
            );
            if (i > 1 && k > 1 && a[i - 1] == b[k - 2] && a[i - 2] == b[k - 1]) {
                const trans = row0[k - 2] + 1;
                if (trans < best) {
                    best = trans;
                }
            }
            row2[k] = best;
            if (best < row_min) {
                row_min = best;
            }
            k += 1;
        }
        // Safe early pruning: adjacent transpositions can reduce a subsequent row by at most 1
        // relative to prev row, so if both row_min and prev_min exceed maxDistance + 1, prune.
        if (row_min > maxDistance + 1) {
            return null;
        }
        let c: usize = 0;
        while (c <= n) {
            row0[c] = row1[c];
            row1[c] = row2[c];
            c += 1;
        }
        i += 1;
    }
    if (row1[n] > maxDistance) {
        return null;
    }
    return row1[n];
}

/// Computes OSA edit distance with `maxDistance` early-exit pruning on strings.
/// Returns `null` if the edit distance exceeds `maxDistance`.
pub fn damerauLevenshteinBounded(a: string, b: string, maxDistance: usize): usize? {
    return damerauLevenshteinBoundedBytes(a, b, maxDistance);
}

/// Computes OSA edit distance, relative distance, and similarity ratio `[0.0, 1.0]`.
pub fn damerauLevenshtein(a: string, b: string): DamLevResult {
    const steps = damerauLevenshteinStepsBytes(a, b);
    return dlvBuildResult(steps, a.len, b.len);
}

/// Computes True Damerau-Levenshtein edit distance, relative distance, and similarity ratio `[0.0, 1.0]`.
pub fn trueDamerauLevenshtein(a: string, b: string): DamLevResult {
    const steps = trueDamerauLevenshteinBytes(a, b);
    return dlvBuildResult(steps, a.len, b.len);
}

/// Returns normalized OSA similarity in `[0.0, 1.0]`.
pub fn similarity(a: string, b: string): f64 {
    return damerauLevenshtein(a, b).similarity;
}

/// Returns normalized True Damerau-Levenshtein similarity in `[0.0, 1.0]`.
pub fn trueSimilarity(a: string, b: string): f64 {
    return trueDamerauLevenshtein(a, b).similarity;
}

export {
    DL_MAX_OSA_LEN,
    DL_MAX_TRUE_LEN,
    DamLevResult,
    damerauLevenshteinStepsBytes,
    damerauLevenshteinSteps,
    osaDistanceBytes,
    osaDistance,
    trueDamerauLevenshteinBytes,
    trueDamerauLevenshteinSteps,
    damerauLevenshteinBoundedBytes,
    damerauLevenshteinBounded,
    damerauLevenshtein,
    trueDamerauLevenshtein,
    similarity,
    trueSimilarity,
};
