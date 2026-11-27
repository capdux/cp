// @cap/damerau-levenshtein — Zero-heap Damerau-Levenshtein edit distance in pure CP
// Clean-room MIT implementation of npm `damerau-levenshtein` (#888, 64.6M/wk).

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

/// Computes Optimal String Alignment (Damerau-Levenshtein) edit distance supporting
/// insertion, deletion, substitution, and adjacent character transposition (up to 127 bytes per string).
pub fn damerauLevenshteinSteps(a: string, b: string): usize {
    if (a.len == 0) { return b.len; }
    if (b.len == 0) { return a.len; }

    let m = a.len;
    let n = b.len;
    if (m > 127) { m = 127; }
    if (n > 127) { n = 127; }

    // 3 rolling rows (prev2, prev1, cur) of width 128 — strictly written before read
    let row0: [128]usize = undefined;
    let row1: [128]usize = undefined;
    let row2: [128]usize = undefined;

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

/// Computes edit distance, relative distance, and similarity ratio `[0.0, 1.0]`.
pub fn damerauLevenshtein(a: string, b: string): DamLevResult {
    const steps = damerauLevenshteinSteps(a, b);
    let max_len = a.len;
    if (b.len > max_len) { max_len = b.len; }
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

export {
    DamLevResult,
    damerauLevenshteinSteps,
    damerauLevenshtein,
};
