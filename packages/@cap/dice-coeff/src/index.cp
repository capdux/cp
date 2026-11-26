// packages/@cap/dice-coeff/src/index.cp
// Zero-allocation Sorensen-Dice bigram string similarity & best-match engine.
// MIT clean-room pure CP implementation.

fn dcToLower(c: u8): u8 {
    if (c >= ('A' as u8) && c <= ('Z' as u8)) {
        return c + (32 as u8);
    }
    return c;
}

fn dcIsSpace(c: u8): bool {
    return c == (' ' as u8) || c == ('\t' as u8) || c == ('\r' as u8) || c == ('\n' as u8);
}

fn dcNormalize(src: []const u8, strip_ws_lower: bool, dst: []u8): usize {
    let n: usize = 0;
    let i: usize = 0;
    while (i < src.len && n < dst.len) {
        const c = src[i];
        if (strip_ws_lower) {
            if (!dcIsSpace(c)) {
                dst[n] = dcToLower(c);
                n += 1;
            }
        } else {
            dst[n] = c;
            n += 1;
        }
        i += 1;
    }
    return n;
}

fn dcEval(a: string, b: string, strip_ws_lower: bool): f64 {
    let bufA: [256]u8 = undefined;
    let bufB: [256]u8 = undefined;
    const lenA = dcNormalize(a, strip_ws_lower, bufA[0..256]);
    const lenB = dcNormalize(b, strip_ws_lower, bufB[0..256]);

    if (lenA == 0 && lenB == 0) {
        return 1.0 as f64;
    }
    if (lenA == lenB) {
        let same = true;
        let k: usize = 0;
        while (k < lenA) {
            if (bufA[k] != bufB[k]) {
                same = false;
                break;
            }
            k += 1;
        }
        if (same) {
            return 1.0 as f64;
        }
    }
    if (lenA < 2 || lenB < 2) {
        return 0.0 as f64;
    }

    const nA = lenA - 1;
    const nB = lenB - 1;

    let used: [256]bool = undefined;
    let ui: usize = 0;
    while (ui < nB) {
        used[ui] = false;
        ui += 1;
    }

    let hits: usize = 0;
    let i: usize = 0;
    while (i < nA) {
        const a0 = bufA[i];
        const a1 = bufA[i + 1];
        let j: usize = 0;
        while (j < nB) {
            if (!used[j] && bufB[j] == a0 && bufB[j + 1] == a1) {
                used[j] = true;
                hits += 1;
                break;
            }
            j += 1;
        }
        i += 1;
    }

    return ((2.0 as f64) * (hits as f64)) / ((nA + nB) as f64);
}

/// Compute the Sorensen-Dice 2-gram multiset similarity coefficient in `[0.0, 1.0]`.
/// Case-insensitive and ignores ASCII whitespace.
/// Returns `1.0` for identical strings (including two empty strings) and `0.0` for unequal strings of length `< 2`.
pub fn diceCoefficient(a: string, b: string): f64 {
    return dcEval(a, b, true);
}

/// Compute the exact case-sensitive, whitespace-preserving Sorensen-Dice bigram similarity in `[0.0, 1.0]`.
pub fn diceCaseSensitive(a: string, b: string): f64 {
    return dcEval(a, b, false);
}

/// Find the index of the candidate in `candidates` with the highest `diceCoefficient` against `target`
/// that is at least `min_rating`. Returns `-1` if no candidate meets `min_rating`.
pub fn bestMatchIndex(target: string, candidates: []const string, min_rating: f64): i64 {
    let bestIdx: i64 = -1 as i64;
    let bestScore: f64 = -1.0;

    let i: usize = 0;
    while (i < candidates.len) {
        const score: f64 = diceCoefficient(target, candidates[i]);
        if (score >= min_rating && score > bestScore) {
            bestScore = score;
            bestIdx = i as i64;
        }
        i += 1;
    }
    return bestIdx;
}

export {
    diceCoefficient,
    diceCaseSensitive,
    bestMatchIndex,
};
