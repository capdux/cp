// Source: cp/lib/text/fuzzy.cp (Edit Distance, Jaro-Winkler & Fuzzy Subsequence Matcher)
// Pure CP Unicode-aware Levenshtein, True Damerau-Levenshtein, zero-alloc Jaro-Winkler,
// and interactive command-palette fuzzy subsequence scorer + top-K ranker.

const std = @import("std");
import { TextErr } from "./chunker.cp";

/// A ranked fuzzy match result returned by `fuzzyTopK`.
pub struct FuzzyMatch {
    index: usize = 0;
    score: i32 = 0;
    text: string = "";
}

fn decodeUtf8Next(s: string, pos: *usize): u32 {
    const i = pos.*;
    if (i >= s.len) return 0;
    const b0 = s[i] as u32;
    if (b0 < 0x80) {
        pos.* = i + 1;
        return b0;
    }
    if ((b0 & 0xE0) == 0xC0 && i + 1 < s.len && (s[i + 1] & 0xC0) == 0x80) {
        pos.* = i + 2;
        return ((b0 & 0x1F) << 6) | ((s[i + 1] & 0x3F) as u32);
    }
    if ((b0 & 0xF0) == 0xE0 && i + 2 < s.len && (s[i + 1] & 0xC0) == 0x80 && (s[i + 2] & 0xC0) == 0x80) {
        pos.* = i + 3;
        return ((b0 & 0x0F) << 12) | (((s[i + 1] & 0x3F) as u32) << 6) | ((s[i + 2] & 0x3F) as u32);
    }
    if ((b0 & 0xF8) == 0xF0 && i + 3 < s.len && (s[i + 1] & 0xC0) == 0x80 && (s[i + 2] & 0xC0) == 0x80 && (s[i + 3] & 0xC0) == 0x80) {
        pos.* = i + 4;
        return ((b0 & 0x07) << 18) | (((s[i + 1] & 0x3F) as u32) << 12) | (((s[i + 2] & 0x3F) as u32) << 6) | ((s[i + 3] & 0x3F) as u32);
    }
    pos.* = i + 1;
    return b0;
}

fn decodeCodepointsAlloc(alloc: std.mem.Allocator, s: string): ([]u32)! {
    let list: std.ArrayList(u32) = .empty;
    errdefer list.deinit(alloc);
    let pos: usize = 0;
    while (pos < s.len) {
        const cp = decodeUtf8Next(s, *pos);
        list.append(alloc, cp) catch return TextErr.OutOfMemory;
    }
    return list.toOwnedSlice(alloc) catch return TextErr.OutOfMemory;
}

fn countCodepoints(s: string): usize {
    let pos: usize = 0;
    let n: usize = 0;
    while (pos < s.len) {
        * = decodeUtf8Next(s, *pos);
        n += 1;
    }
    return n;
}

fn min3(a: usize, b: usize, c: usize): usize {
    let m = if (a < b) a else b;
    if (c < m) m = c;
    return m;
}

fn min4(a: usize, b: usize, c: usize, d: usize): usize {
    let m = min3(a, b, c);
    if (d < m) m = d;
    return m;
}

/// Computes the Unicode codepoint Levenshtein edit distance between `a` and `b`
/// using O(min(m, n)) heap space.
pub fn levenshtein(alloc: std.mem.Allocator, a: string, b: string): usize! {
    if (std.mem.eql(u8, a, b)) return 0;
    if (a.len == 0) return countCodepoints(b);
    if (b.len == 0) return countCodepoints(a);

    const cp_a = decodeCodepointsAlloc(alloc, a)?;
    defer alloc.free(cp_a);
    const cp_b = decodeCodepointsAlloc(alloc, b)?;
    defer alloc.free(cp_b);

    let s1 = cp_a;
    let s2 = cp_b;
    if (s1.len < s2.len) {
        s1 = cp_b;
        s2 = cp_a;
    }

    const m = s1.len;
    const n = s2.len;
    if (n == 0) return m;

    let prev = alloc.alloc(usize, n + 1) catch return TextErr.OutOfMemory;
    defer alloc.free(prev);
    let curr = alloc.alloc(usize, n + 1) catch return TextErr.OutOfMemory;
    defer alloc.free(curr);

    let j: usize = 0;
    while (j <= n) : (j += 1) {
        prev[j] = j;
    }

    let i: usize = 1;
    while (i <= m) : (i += 1) {
        curr[0] = i;
        let k: usize = 1;
        while (k <= n) : (k += 1) {
            const cost: usize = if (s1[i - 1] == s2[k - 1]) 0 else 1;
            curr[k] = min3(prev[k] + 1, curr[k - 1] + 1, prev[k - 1] + cost);
        }
        const tmp = prev;
        prev = curr;
        curr = tmp;
    }
    return prev[n];
}

/// Computes the unrestricted (true) Damerau-Levenshtein edit distance between `a` and `b`
/// over Unicode codepoints (Lowrance-Wagner algorithm, supporting adjacent transpositions).
pub fn damerauLevenshtein(alloc: std.mem.Allocator, a: string, b: string): usize! {
    if (std.mem.eql(u8, a, b)) return 0;
    if (a.len == 0) return countCodepoints(b);
    if (b.len == 0) return countCodepoints(a);

    const cp_a = decodeCodepointsAlloc(alloc, a)?;
    defer alloc.free(cp_a);
    const cp_b = decodeCodepointsAlloc(alloc, b)?;
    defer alloc.free(cp_b);

    const m = cp_a.len;
    const n = cp_b.len;
    const cols = n + 2;
    const max_dist = m + n;

    let d = alloc.alloc(usize, (m + 2) * cols) catch return TextErr.OutOfMemory;
    defer alloc.free(d);

    let da: std.AutoHashMapUnmanaged(u32, usize) = .empty;
    defer da.deinit(alloc);

    d[0] = max_dist;
    let i: usize = 0;
    while (i <= m) : (i += 1) {
        d[(i + 1) * cols + 0] = max_dist;
        d[(i + 1) * cols + 1] = i;
    }
    let j: usize = 0;
    while (j <= n) : (j += 1) {
        d[0 * cols + (j + 1)] = max_dist;
        d[1 * cols + (j + 1)] = j;
    }

    i = 1;
    while (i <= m) : (i += 1) {
        let db: usize = 0;
        j = 1;
        while (j <= n) : (j += 1) {
            const k = da.get(cp_b[j - 1]) ?? 0;
            const l = db;
            let cost: usize = 1;
            if (cp_a[i - 1] == cp_b[j - 1]) {
                cost = 0;
                db = j;
            }
            const sub = d[i * cols + j] + cost;
            const ins = d[(i + 1) * cols + j] + 1;
            const del = d[i * cols + (j + 1)] + 1;
            const trans = d[k * cols + l] + (i - k - 1) + 1 + (j - l - 1);
            d[(i + 1) * cols + (j + 1)] = min4(sub, ins, del, trans);
        }
        da.put(alloc, cp_a[i - 1], i) catch return TextErr.OutOfMemory;
    }

    return d[(m + 1) * cols + (n + 1)];
}

const JW_MAX_STACK_CPS: usize = 1024;

/// Computes Jaro-Winkler similarity in `[0.0, 1.0]` over UTF-8 codepoints with zero heap allocation.
/// Uses Winkler prefix scale `p = 0.1` up to prefix length `4`.
pub fn jaroWinkler(a: string, b: string): f64 {
    if (a.len == 0 && b.len == 0) return 1.0;
    if (a.len == 0 || b.len == 0) return 0.0;
    if (std.mem.eql(u8, a, b)) return 1.0;

    let cp_a: [JW_MAX_STACK_CPS]u32 = [0; JW_MAX_STACK_CPS];
    let cp_b: [JW_MAX_STACK_CPS]u32 = [0; JW_MAX_STACK_CPS];
    let len_a: usize = 0;
    let len_b: usize = 0;

    let pa: usize = 0;
    while (pa < a.len && len_a < JW_MAX_STACK_CPS) {
        cp_a[len_a] = decodeUtf8Next(a, *pa);
        len_a += 1;
    }
    let pb: usize = 0;
    while (pb < b.len && len_b < JW_MAX_STACK_CPS) {
        cp_b[len_b] = decodeUtf8Next(b, *pb);
        len_b += 1;
    }

    if (len_a == 0 || len_b == 0) return 0.0;

    const max_len = if (len_a > len_b) len_a else len_b;
    const match_dist: usize = if (max_len >= 2) (max_len / 2) - 1 else 0;

    let a_matched: [JW_MAX_STACK_CPS]bool = [false; JW_MAX_STACK_CPS];
    let b_matched: [JW_MAX_STACK_CPS]bool = [false; JW_MAX_STACK_CPS];

    let matches: usize = 0;
    let i: usize = 0;
    while (i < len_a) : (i += 1) {
        const lo: usize = if (i > match_dist) i - match_dist else 0;
        let hi: usize = i + match_dist + 1;
        if (hi > len_b) hi = len_b;

        let j = lo;
        while (j < hi) : (j += 1) {
            if (!b_matched[j] && cp_a[i] == cp_b[j]) {
                a_matched[i] = true;
                b_matched[j] = true;
                matches += 1;
                break;
            }
        }
    }

    if (matches == 0) return 0.0;

    let half_transpositions: usize = 0;
    let k: usize = 0;
    i = 0;
    while (i < len_a) : (i += 1) {
        if (!a_matched[i]) continue;
        while (!b_matched[k]) : (k += 1) {}
        if (cp_a[i] != cp_b[k]) {
            half_transpositions += 1;
        }
        k += 1;
    }

    const m = matches as f64;
    const trans = (half_transpositions as f64) / 2.0;
    const jaro = (m / (len_a as f64) + m / (len_b as f64) + (m - trans) / m) / 3.0;

    let prefix_len: usize = 0;
    const max_prefix = if (len_a < len_b) (if (len_a < 4) len_a else 4) else (if (len_b < 4) len_b else 4);
    while (prefix_len < max_prefix && cp_a[prefix_len] == cp_b[prefix_len]) : (prefix_len += 1) {}

    return jaro + (prefix_len as f64) * 0.1 * (1.0 - jaro);
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn isSeparatorChar(c: u8): bool {
    return c == ' ' || c == '_' || c == '-' || c == '/' || c == '\\' || c == '.' || c == ':';
}

fn charMatchBonus(cand: string, j: usize, q_byte: u8): i32 {
    let bonus: i32 = 10;
    if (cand[j] == q_byte) {
        bonus += 2;
    }
    if (j == 0) {
        bonus += 15;
    } else {
        const prev = cand[j - 1];
        if (isSeparatorChar(prev)) {
            bonus += 15;
        } else if (prev >= 'a' && prev <= 'z' && cand[j] >= 'A' && cand[j] <= 'Z') {
            bonus += 12;
        }
    }
    return bonus;
}

const FUZZY_DP_MAX_LEN: usize = 512;
const FUZZY_NEG_INF: i32 = -1000000000;

fn fuzzyScoreGreedy(query: string, candidate: string): i32? {
    let qi: usize = 0;
    let cj: usize = 0;
    let score: i32 = 0;
    let prev_match: usize = 0;
    let has_prev = false;

    while (qi < query.len && cj < candidate.len) : (cj += 1) {
        if (toLowerAscii(query[qi]) == toLowerAscii(candidate[cj])) {
            score += charMatchBonus(candidate, cj, query[qi]);
            if (has_prev) {
                if (cj == prev_match + 1) {
                    score += 15;
                } else {
                    score -= ((cj - prev_match - 1) as i32);
                }
            } else {
                score -= (cj as i32);
            }
            prev_match = cj;
            has_prev = true;
            qi += 1;
        }
    }

    if (qi < query.len) return null;
    score -= (candidate.len as i32);
    return score;
}

/// Case-insensitive subsequence fuzzy matcher with bonuses for consecutive hits,
/// word-boundary starts (`_`, `-`, `/`, `.`, ` `, `:`), and camelCase transitions.
/// Returns `null` if `query` is not a subsequence of `candidate`.
pub fn fuzzyScore(query: string, candidate: string): i32? {
    if (query.len == 0) return 0;
    if (candidate.len == 0 || query.len > candidate.len) return null;

    if (candidate.len > FUZZY_DP_MAX_LEN) {
        return fuzzyScoreGreedy(query, candidate);
    }

    // Fast subsequence pre-check so non-matches return in O(n)
    let q_chk: usize = 0;
    let c_chk: usize = 0;
    while (q_chk < query.len && c_chk < candidate.len) : (c_chk += 1) {
        if (toLowerAscii(query[q_chk]) == toLowerAscii(candidate[c_chk])) {
            q_chk += 1;
        }
    }
    if (q_chk < query.len) return null;

    let prev_row: [FUZZY_DP_MAX_LEN]i32 = [FUZZY_NEG_INF; FUZZY_DP_MAX_LEN];
    let curr_row: [FUZZY_DP_MAX_LEN]i32 = [FUZZY_NEG_INF; FUZZY_DP_MAX_LEN];

    const n = candidate.len;
    const m = query.len;

    // Row 0: match query[0] at candidate[j]
    let j: usize = 0;
    while (j < n) : (j += 1) {
        if (toLowerAscii(query[0]) == toLowerAscii(candidate[j])) {
            prev_row[j] = charMatchBonus(candidate, j, query[0]) - (j as i32);
        } else {
            prev_row[j] = FUZZY_NEG_INF;
        }
    }

    // Rows 1..m-1: optimal alignment maximizing consecutive & boundary bonuses
    let i: usize = 1;
    while (i < m) : (i += 1) {
        let best_gap = FUZZY_NEG_INF;
        const qi_lower = toLowerAscii(query[i]);

        j = 0;
        while (j < n) : (j += 1) {
            let best = FUZZY_NEG_INF;
            if (j >= i && qi_lower == toLowerAscii(candidate[j])) {
                const pos_bonus = charMatchBonus(candidate, j, query[i]);
                // Consecutive match from j - 1
                if (j > 0 && prev_row[j - 1] != FUZZY_NEG_INF) {
                    const consec_score = prev_row[j - 1] + pos_bonus + 15;
                    if (consec_score > best) best = consec_score;
                }
                // Non-consecutive match from k <= j - 2 with -1 per skipped byte
                if (best_gap != FUZZY_NEG_INF) {
                    const gap_score = best_gap + pos_bonus;
                    if (gap_score > best) best = gap_score;
                }
            }
            curr_row[j] = best;

            // Advance `best_gap` for j + 1: tracks max_{k <= j - 1} (prev_row[k] - ((j + 1) - k - 1))
            if (best_gap != FUZZY_NEG_INF) {
                best_gap -= 1;
            }
            if (j > 0 && prev_row[j - 1] != FUZZY_NEG_INF) {
                const cand_gap = prev_row[j - 1] - 1;
                if (cand_gap > best_gap) best_gap = cand_gap;
            }
        }

        j = 0;
        while (j < n) : (j += 1) {
            prev_row[j] = curr_row[j];
        }
    }

    let best_final = FUZZY_NEG_INF;
    j = m - 1;
    while (j < n) : (j += 1) {
        if (prev_row[j] > best_final) {
            best_final = prev_row[j];
        }
    }
    if (best_final == FUZZY_NEG_INF) return null;

    // Slight preference for shorter candidates on equal match quality
    return best_final - (n as i32);
}

fn fuzzyMatchBetter(a: FuzzyMatch, b: FuzzyMatch): bool {
    if (a.score != b.score) return a.score > b.score;
    if (a.text.len != b.text.len) return a.text.len < b.text.len;
    return a.index < b.index;
}

fn sortFuzzyMatchesDesc(items: []FuzzyMatch) {
    let i: usize = 1;
    while (i < items.len) : (i += 1) {
        const key = items[i];
        let j = i;
        while (j > 0 && fuzzyMatchBetter(key, items[j - 1])) {
            items[j] = items[j - 1];
            j -= 1;
        }
        items[j] = key;
    }
}

/// Scores all `candidates` against `query` using `fuzzyScore` and returns up to `top_k`
/// matches sorted descending by score (tie-broken by shorter length, then original index).
/// Caller owns the returned slice and must free it with `alloc.free(matches)`.
pub fn fuzzyTopK(
    alloc: std.mem.Allocator,
    query: string,
    candidates: []const string,
    top_k: usize,
): ([]FuzzyMatch)! {
    if (top_k == 0 || candidates.len == 0) {
        return alloc.alloc(FuzzyMatch, 0) catch return TextErr.OutOfMemory;
    }

    let matches: std.ArrayList(FuzzyMatch) = .empty;
    defer matches.deinit(alloc);

    let i: usize = 0;
    while (i < candidates.len) : (i += 1) {
        if (s := fuzzyScore(query, candidates[i])) {
            matches.append(alloc, FuzzyMatch{
                index: i,
                score: s,
                text: candidates[i],
            }) catch return TextErr.OutOfMemory;
        }
    }

    if (matches.items.len == 0) {
        return alloc.alloc(FuzzyMatch, 0) catch return TextErr.OutOfMemory;
    }

    sortFuzzyMatchesDesc(matches.items);
    const out_len = if (matches.items.len < top_k) matches.items.len else top_k;
    let out = alloc.alloc(FuzzyMatch, out_len) catch return TextErr.OutOfMemory;
    let k: usize = 0;
    while (k < out_len) : (k += 1) {
        out[k] = matches.items[k];
    }
    return out;
}
