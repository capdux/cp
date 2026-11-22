// Reference Only: Functional overlap with lib/text/fuzzy.cp | Kept for comparison/analysis
// Source: cp/lib/fuzzy/fuzzy.cp (Fuzzy String Matching, Edit Distance, Jaro-Winkler & Ranked Search)
// Pure CP zero-C-dependency fuzzy string matching library providing:
// - O(min(M, N)) space-optimized Unicode Levenshtein distance
// - True Unicode Damerau-Levenshtein distance (Lowrance-Wagner with adjacent transpositions)
// - Jaro & Jaro-Winkler similarity with prefix_scale validation
// - fzf-style subsequence fuzzyScore with exact match positions, boundary/camelCase/consecutive bonuses
// - rankCandidates & freeMatches for ranked search

const std = @import("std");

/// Error set for `cap:fuzzy` operations.
pub err FuzzyError {
    InvalidPrefixScale,
    OutOfMemory,
}

/// Ranked fuzzy subsequence match result with exact byte positions in `candidate`.
pub struct FuzzyMatch {
    candidate: []const u8;
    index: usize;
    score: i64;
    positions: []usize;

    /// Releases the heap-allocated `positions` slice.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.positions.len > 0) {
            allocator.free(_.positions);
        }
        _.positions = _.positions[0..0];
    }
}

/// Frees a slice of `FuzzyMatch` returned by `rankCandidates` along with each match's `positions`.
pub fn freeMatches(allocator: std.mem.Allocator, matches: []FuzzyMatch) {
    let i: usize = 0;
    while (i < matches.len) : (i += 1) {
        matches[i].deinit(allocator);
    }
    if (matches.len > 0) {
        allocator.free(matches);
    }
}

fn decodeUtf8Next(s: []const u8, pos: *usize): u32 {
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

fn countCodepoints(s: []const u8): usize {
    let pos: usize = 0;
    let count: usize = 0;
    while (pos < s.len) {
        * = decodeUtf8Next(s, *pos);
        count += 1;
    }
    return count;
}

fn decodeCodepointsAlloc(allocator: std.mem.Allocator, s: []const u8): ([]u32) ! FuzzyError {
    let list: std.ArrayList(u32) = .empty;
    errdefer list.deinit(allocator);
    let pos: usize = 0;
    while (pos < s.len) {
        const cp = decodeUtf8Next(s, *pos);
        list.append(allocator, cp) catch return FuzzyError.OutOfMemory;
    }
    return list.toOwnedSlice(allocator) catch return FuzzyError.OutOfMemory;
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

/// Computes the Unicode Levenshtein edit distance between `a` and `b` using O(min(M, N)) space.
pub fn levenshtein(allocator: std.mem.Allocator, a: []const u8, b: []const u8): usize ! FuzzyError {
    if (std.mem.eql(u8, a, b)) return 0;
    if (a.len == 0) return countCodepoints(b);
    if (b.len == 0) return countCodepoints(a);

    const cp_a = decodeCodepointsAlloc(allocator, a)?;
    defer allocator.free(cp_a);
    const cp_b = decodeCodepointsAlloc(allocator, b)?;
    defer allocator.free(cp_b);

    let s1 = cp_a;
    let s2 = cp_b;
    if (s1.len < s2.len) {
        s1 = cp_b;
        s2 = cp_a;
    }

    const m = s1.len;
    const n = s2.len;
    if (n == 0) return m;

    let prev = allocator.alloc(usize, n + 1) catch return FuzzyError.OutOfMemory;
    defer allocator.free(prev);
    let curr = allocator.alloc(usize, n + 1) catch return FuzzyError.OutOfMemory;
    defer allocator.free(curr);

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

/// Computes the true (unrestricted) Damerau-Levenshtein edit distance between `a` and `b`
/// over Unicode codepoints, supporting adjacent transpositions at cost 1.
pub fn damerauLevenshtein(allocator: std.mem.Allocator, a: []const u8, b: []const u8): usize ! FuzzyError {
    if (std.mem.eql(u8, a, b)) return 0;
    if (a.len == 0) return countCodepoints(b);
    if (b.len == 0) return countCodepoints(a);

    const cp_a = decodeCodepointsAlloc(allocator, a)?;
    defer allocator.free(cp_a);
    const cp_b = decodeCodepointsAlloc(allocator, b)?;
    defer allocator.free(cp_b);

    const m = cp_a.len;
    const n = cp_b.len;
    const cols = n + 2;
    const max_dist = m + n;

    let d = allocator.alloc(usize, (m + 2) * cols) catch return FuzzyError.OutOfMemory;
    defer allocator.free(d);

    let da: std.AutoHashMapUnmanaged(u32, usize) = .empty;
    defer da.deinit(allocator);

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
        da.put(allocator, cp_a[i - 1], i) catch return FuzzyError.OutOfMemory;
    }

    return d[(m + 1) * cols + (n + 1)];
}

fn jaroFromCodepoints(
    allocator: std.mem.Allocator,
    cp_a: []const u32,
    cp_b: []const u32,
): f64 ! FuzzyError {
    const len_a = cp_a.len;
    const len_b = cp_b.len;
    if (len_a == 0 && len_b == 0) return 1.0;
    if (len_a == 0 || len_b == 0) return 0.0;

    const max_len = if (len_a > len_b) len_a else len_b;
    const match_dist: usize = if (max_len >= 2) (max_len / 2) - 1 else 0;

    let a_matched = allocator.alloc(bool, len_a) catch return FuzzyError.OutOfMemory;
    defer allocator.free(a_matched);
    let b_matched = allocator.alloc(bool, len_b) catch return FuzzyError.OutOfMemory;
    defer allocator.free(b_matched);

    @memset(a_matched, false);
    @memset(b_matched, false);

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
    return (m / (len_a as f64) + m / (len_b as f64) + (m - trans) / m) / 3.0;
}

/// Computes the Jaro similarity in `[0.0, 1.0]` over Unicode codepoints.
pub fn jaro(allocator: std.mem.Allocator, a: []const u8, b: []const u8): f64 ! FuzzyError {
    if (a.len == 0 && b.len == 0) return 1.0;
    if (a.len == 0 || b.len == 0) return 0.0;
    if (std.mem.eql(u8, a, b)) return 1.0;

    const cp_a = decodeCodepointsAlloc(allocator, a)?;
    defer allocator.free(cp_a);
    const cp_b = decodeCodepointsAlloc(allocator, b)?;
    defer allocator.free(cp_b);

    return jaroFromCodepoints(allocator, cp_a, cp_b);
}

/// Computes the Jaro-Winkler similarity in `[0.0, 1.0]` over Unicode codepoints.
/// Validates `0.0 <= prefix_scale <= 0.25`, returning `FuzzyError.InvalidPrefixScale` otherwise.
pub fn jaroWinkler(
    allocator: std.mem.Allocator,
    a: []const u8,
    b: []const u8,
    prefix_scale: f64,
): f64 ! FuzzyError {
    if (std.math.isNan(prefix_scale) || prefix_scale < 0.0 || prefix_scale > 0.25) {
        return FuzzyError.InvalidPrefixScale;
    }
    if (a.len == 0 && b.len == 0) return 1.0;
    if (a.len == 0 || b.len == 0) return 0.0;
    if (std.mem.eql(u8, a, b)) return 1.0;

    const cp_a = decodeCodepointsAlloc(allocator, a)?;
    defer allocator.free(cp_a);
    const cp_b = decodeCodepointsAlloc(allocator, b)?;
    defer allocator.free(cp_b);

    const j = jaroFromCodepoints(allocator, cp_a, cp_b)?;
    const min_len = if (cp_a.len < cp_b.len) cp_a.len else cp_b.len;
    const max_prefix = if (min_len < 4) min_len else 4;
    let prefix_len: usize = 0;
    while (prefix_len < max_prefix && cp_a[prefix_len] == cp_b[prefix_len]) : (prefix_len += 1) {}

    return j + (prefix_len as f64) * prefix_scale * (1.0 - j);
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn bytesMatch(q: u8, t: u8, case_sensitive: bool): bool {
    if (case_sensitive) return q == t;
    return toLowerAscii(q) == toLowerAscii(t);
}

fn isSeparatorChar(c: u8): bool {
    return c == '/' || c == '-' || c == '_' || c == '.' || c == ' ' || c == '\\' || c == ':';
}

fn charMatchBonus(target: []const u8, j: usize, q_byte: u8): i64 {
    let bonus: i64 = 10;
    if (target[j] == q_byte) {
        bonus += 2;
    }
    if (j == 0) {
        bonus += 24;
    } else {
        const prev = target[j - 1];
        if (isSeparatorChar(prev)) {
            bonus += 20;
        } else if (prev >= 'a' && prev <= 'z' && target[j] >= 'A' && target[j] <= 'Z') {
            bonus += 16;
        }
    }
    return bonus;
}

const FUZZY_NEG_INF: i64 = -1000000000000;
const FUZZY_MAX_DP_CELLS: usize = 262144;
const BONUS_CONSECUTIVE: i64 = 20;
const PENALTY_GAP: i64 = 3;
const PENALTY_LEADING: i64 = 2;

fn fuzzyScoreGreedy(
    allocator: std.mem.Allocator,
    query: []const u8,
    target: []const u8,
    case_sensitive: bool,
): (FuzzyMatch?) ! FuzzyError {
    let positions = allocator.alloc(usize, query.len) catch return FuzzyError.OutOfMemory;
    errdefer allocator.free(positions);

    let qi: usize = 0;
    let cj: usize = 0;
    let score: i64 = 0;
    let prev_match: usize = 0;
    let has_prev = false;

    while (qi < query.len && cj < target.len) : (cj += 1) {
        if (bytesMatch(query[qi], target[cj], case_sensitive)) {
            score += charMatchBonus(target, cj, query[qi]);
            if (has_prev) {
                if (cj == prev_match + 1) {
                    score += BONUS_CONSECUTIVE;
                } else {
                    score -= ((cj - prev_match - 1) as i64) * PENALTY_GAP;
                }
            } else {
                const lead = (cj as i64) * PENALTY_LEADING;
                score -= if (lead > 24) 24 else lead;
            }
            positions[qi] = cj;
            prev_match = cj;
            has_prev = true;
            qi += 1;
        }
    }

    if (qi < query.len) {
        allocator.free(positions);
        return null;
    }

    score -= (target.len as i64);
    return FuzzyMatch{
        candidate: target,
        index: 0,
        score: score,
        positions: positions,
    };
}

/// Performs fzf-style subsequence matching of `query` against `target` and computes
/// an alignment score with bonuses for consecutive matches, word boundaries (`/`, `-`, `_`, `.`, space),
/// camelCase transitions, and first-character matches.
/// Returns `null` if `query` is not a subsequence of `target`.
/// Caller owns the returned `FuzzyMatch.positions` and must call `deinit(allocator)`.
pub fn fuzzyScore(
    allocator: std.mem.Allocator,
    query: []const u8,
    target: []const u8,
    case_sensitive: bool,
): (FuzzyMatch?) ! FuzzyError {
    if (query.len == 0) {
        const empty_pos = allocator.alloc(usize, 0) catch return FuzzyError.OutOfMemory;
        return FuzzyMatch{
            candidate: target,
            index: 0,
            score: 0,
            positions: empty_pos,
        };
    }
    if (target.len == 0 || query.len > target.len) return null;

    // Fast O(N) subsequence pre-check
    let q_chk: usize = 0;
    let c_chk: usize = 0;
    while (q_chk < query.len && c_chk < target.len) : (c_chk += 1) {
        if (bytesMatch(query[q_chk], target[c_chk], case_sensitive)) {
            q_chk += 1;
        }
    }
    if (q_chk < query.len) return null;

    const m = query.len;
    const n = target.len;
    if (m * n > FUZZY_MAX_DP_CELLS) {
        return fuzzyScoreGreedy(allocator, query, target, case_sensitive);
    }

    let dp = allocator.alloc(i64, m * n) catch return FuzzyError.OutOfMemory;
    defer allocator.free(dp);
    let prev_idx = allocator.alloc(usize, m * n) catch return FuzzyError.OutOfMemory;
    defer allocator.free(prev_idx);

    // Row 0: match query[0] at target[j]
    let j: usize = 0;
    while (j < n) : (j += 1) {
        if (bytesMatch(query[0], target[j], case_sensitive)) {
            const lead = (j as i64) * PENALTY_LEADING;
            const lead_pen = if (lead > 24) 24 else lead;
            dp[j] = charMatchBonus(target, j, query[0]) - lead_pen;
        } else {
            dp[j] = FUZZY_NEG_INF;
        }
        prev_idx[j] = 0;
    }

    // Rows 1..m-1: optimal alignment with O(M * N) tracking for exact backtrace
    let i: usize = 1;
    while (i < m) : (i += 1) {
        let best_gap = FUZZY_NEG_INF;
        let best_gap_pos: usize = 0;
        const row_off = i * n;
        const prev_row_off = (i - 1) * n;

        j = 0;
        while (j < n) : (j += 1) {
            let best = FUZZY_NEG_INF;
            let best_k: usize = 0;

            if (j >= i && bytesMatch(query[i], target[j], case_sensitive)) {
                const pos_bonus = charMatchBonus(target, j, query[i]);
                if (j > 0 && dp[prev_row_off + j - 1] != FUZZY_NEG_INF) {
                    const consec = dp[prev_row_off + j - 1] + pos_bonus + BONUS_CONSECUTIVE;
                    if (consec > best) {
                        best = consec;
                        best_k = j - 1;
                    }
                }
                if (best_gap != FUZZY_NEG_INF) {
                    const gap_score = best_gap + pos_bonus;
                    if (gap_score > best) {
                        best = gap_score;
                        best_k = best_gap_pos;
                    }
                }
            }

            dp[row_off + j] = best;
            prev_idx[row_off + j] = best_k;

            // Advance best_gap for j + 1: max_{k <= j - 1} (dp[i-1, k] - ((j + 1) - k - 1) * PENALTY_GAP)
            if (best_gap != FUZZY_NEG_INF) {
                best_gap -= PENALTY_GAP;
            }
            if (j > 0 && dp[prev_row_off + j - 1] != FUZZY_NEG_INF) {
                const cand_gap = dp[prev_row_off + j - 1] - PENALTY_GAP;
                if (cand_gap > best_gap) {
                    best_gap = cand_gap;
                    best_gap_pos = j - 1;
                }
            }
        }
    }

    let best_final = FUZZY_NEG_INF;
    let best_end_pos: usize = 0;
    const last_row = (m - 1) * n;
    j = m - 1;
    while (j < n) : (j += 1) {
        if (dp[last_row + j] > best_final) {
            best_final = dp[last_row + j];
            best_end_pos = j;
        }
    }
    if (best_final == FUZZY_NEG_INF) return null;

    let positions = allocator.alloc(usize, m) catch return FuzzyError.OutOfMemory;
    let cur = best_end_pos;
    let step = m;
    while (step > 0) {
        step -= 1;
        positions[step] = cur;
        if (step > 0) {
            cur = prev_idx[step * n + cur];
        }
    }

    return FuzzyMatch{
        candidate: target,
        index: 0,
        score: best_final - (n as i64),
        positions: positions,
    };
}

fn fuzzyMatchBetter(a: FuzzyMatch, b: FuzzyMatch): bool {
    if (a.score != b.score) return a.score > b.score;
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

/// Filters `candidates` by `fuzzyScore` and returns matches sorted by `score` descending,
/// tie-breaking by original `index` ascending.
/// Caller must free the returned slice with `freeMatches(allocator, matches)`.
pub fn rankCandidates(
    allocator: std.mem.Allocator,
    query: []const u8,
    candidates: []const []const u8,
    case_sensitive: bool,
): ([]FuzzyMatch) ! FuzzyError {
    let matches: std.ArrayList(FuzzyMatch) = .empty;
    errdefer {
        let k: usize = 0;
        while (k < matches.items.len) : (k += 1) {
            matches.items[k].deinit(allocator);
        }
        matches.deinit(allocator);
    }

    let i: usize = 0;
    while (i < candidates.len) : (i += 1) {
        if (m := fuzzyScore(allocator, query, candidates[i], case_sensitive)?) {
            let entry = m;
            entry.index = i;
            matches.append(allocator, entry) catch {
                entry.deinit(allocator);
                return FuzzyError.OutOfMemory;
            };
        }
    }

    sortFuzzyMatchesDesc(matches.items);
    return matches.toOwnedSlice(allocator) catch return FuzzyError.OutOfMemory;
}
