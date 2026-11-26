// @cap/abbrev — Zero-heap unique unambiguous prefix abbreviation generator
// Clean-room ISC implementation inspired by npm:abbrev (npm/abbrev-js).

pub struct AbbrevEntry {
    abbrev: string,
    full: string,
}

fn abvStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn abvStrLt(a: string, b: string): bool {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) : (i += 1) {
        if (a[i] != b[i]) {
            return a[i] < b[i];
        }
    }
    return a.len < b.len;
}

fn abvHasPrefix(word: string, prefix: string): bool {
    if (prefix.len > word.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (word[i] != prefix[i]) {
            return false;
        }
    }
    return true;
}

fn abvLcp(a: string, b: string): usize {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len && a[i] == b[i]) : (i += 1) {}
    return i;
}

/// Generate all unique unambiguous (abbrev -> full) mappings in lexicographical order
/// into `out` and return the number of entries written.
pub fn abbrev(words: []const string, out: []AbbrevEntry): usize {
    if (words.len == 0 || out.len == 0) {
        return 0;
    }

    // Copy into local buffer (up to 256 words) and insertion-sort lexicographically
    let sorted: [256]string = undefined;
    let count: usize = 0;
    let wi: usize = 0;
    while (wi < words.len && count < 256) : (wi += 1) {
        const w = words[wi];
        // Insertion sort
        let pos = count;
        while (pos > 0 && abvStrLt(w, sorted[pos - 1])) : (pos -= 1) {
            sorted[pos] = sorted[pos - 1];
        }
        sorted[pos] = w;
        count += 1;
    }

    // Deduplicate adjacent equal words in-place
    let uniq: usize = 0;
    let si: usize = 0;
    while (si < count) : (si += 1) {
        if (uniq == 0 || !abvStrEq(sorted[si], sorted[uniq - 1])) {
            sorted[uniq] = sorted[si];
            uniq += 1;
        }
    }

    let out_pos: usize = 0;
    let i: usize = 0;
    while (i < uniq) : (i += 1) {
        const cur = sorted[i];
        if (cur.len == 0) {
            continue;
        }

        let max_lcp: usize = 0;
        if (i > 0) {
            const lp = abvLcp(cur, sorted[i - 1]);
            if (lp > max_lcp) {
                max_lcp = lp;
            }
        }
        if (i + 1 < uniq) {
            const rp = abvLcp(cur, sorted[i + 1]);
            if (rp > max_lcp) {
                max_lcp = rp;
            }
        }

        // If `cur` is a prefix of the next word, only the exact full word is unambiguous.
        let min_len: usize = if (max_lcp >= cur.len) cur.len else max_lcp + 1;
        let plen = min_len;
        while (plen <= cur.len) : (plen += 1) {
            if (out_pos >= out.len) {
                return out_pos;
            }
            out[out_pos] = AbbrevEntry{
                abbrev: cur[0..plen],
                full: cur,
            };
            out_pos += 1;
        }
    }

    return out_pos;
}

/// Return the shortest unambiguous prefix of `target` within `words`,
/// or `null` if `target` is not present in `words`.
pub fn shortestAbbrev(words: []const string, target: string): string? {
    let found = false;
    let max_lcp: usize = 0;
    let i: usize = 0;
    while (i < words.len) : (i += 1) {
        const w = words[i];
        if (abvStrEq(w, target)) {
            found = true;
        } else {
            const lp = abvLcp(target, w);
            if (lp > max_lcp) {
                max_lcp = lp;
            }
        }
    }

    if (!found) {
        return null;
    }
    if (target.len == 0) {
        return target;
    }
    if (max_lcp >= target.len) {
        return target;
    }
    return target[0..(max_lcp + 1)];
}

/// Resolve `prefix` against `words`:
/// - Exact match always takes precedence.
/// - Returns the unique matching word if unambiguous.
/// - Returns `null` if no match or ambiguous (2+ distinct words share `prefix`).
pub fn resolveAbbrev(words: []const string, prefix: string): string? {
    if (words.len == 0) {
        return null;
    }

    // Pass 1: exact match takes priority
    let i: usize = 0;
    while (i < words.len) : (i += 1) {
        if (abvStrEq(words[i], prefix)) {
            return words[i];
        }
    }

    if (prefix.len == 0) {
        return null;
    }

    // Pass 2: check unique prefix match (ignoring duplicate entries of the same word)
    let matched: string = "";
    let match_count: usize = 0;
    i = 0;
    while (i < words.len) : (i += 1) {
        const w = words[i];
        if (abvHasPrefix(w, prefix)) {
            if (match_count == 0) {
                matched = w;
                match_count = 1;
            } else if (!abvStrEq(matched, w)) {
                return null;
            }
        }
    }

    if (match_count == 1) {
        return matched;
    }
    return null;
}
