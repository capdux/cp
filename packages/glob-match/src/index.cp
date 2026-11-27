// packages/@cap/glob-match/src/index.cp
// Zero-allocation, anti-backtracking path glob and brace alternation matching engine.
// Clean-room MIT pure CP implementation inspired by glob-match v0.2.
// Accelerated with 16-lane @Vector(16, u8) SIMD slash/brace/meta scanning, comptime 256-entry LUT, and @memcpy.

type GbmVec16 = @Vector(16, u8);

fn gbmBuildMetaLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    // bit0: glob meta (* ? [ {), bit1: brace scan stop (\ [ {)
    let lut: [256]u8 = [0; 256];
    lut['*' as usize] = 1;
    lut['?' as usize] = 1;
    lut['[' as usize] = 3;
    lut['{' as usize] = 3;
    lut['\\' as usize] = 2;
    return lut;
}

const GBM_META_LUT: [256]u8 = gbmBuildMetaLut();

fn gbmFindSlash(path: string, start: usize): usize {
    let i: usize = start;
    const v_sl: GbmVec16 = @splat('/' as u8);
    while (i + 16 <= path.len) {
        const v: GbmVec16 = path[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_sl)) {
            break;
        }
        i += 16;
    }
    while (i < path.len && path[i] != ('/' as u8)) {
        i += 1;
    }
    return i;
}

/// Byte range `[start, end)` captured by a `*`, `**`, or `?` wildcard in `globMatchCaptures`.
pub struct GlobSpan {
    pub start: usize,
    pub end: usize,
}

const GBM_MAX_PATTERN_BUF: usize = 2048;
const GBM_MAX_CAPTURES: usize = 64;
const GBM_MAX_BRACE_DEPTH: usize = 8;
const GBM_MAX_BRACE_STEPS: usize = 4096;

/// Find closing `]` for a character class starting at `pattern[open_idx] == '['`.
/// Returns the index of `]`, or `pattern.len` if unclosed.
fn gbmFindClassClose(pattern: string, open_idx: usize): usize {
    let i: usize = open_idx + 1;
    if (i < pattern.len && (pattern[i] == ('!' as u8) || pattern[i] == ('^' as u8))) {
        i += 1;
    }
    // A `]` immediately after `[`, `[!`, or `[^` is treated as a literal member.
    if (i < pattern.len && pattern[i] == (']' as u8)) {
        i += 1;
    }
    while (i < pattern.len) {
        if (pattern[i] == ('\\' as u8) && i + 1 < pattern.len) {
            i += 2;
            continue;
        }
        if (pattern[i] == (']' as u8)) {
            return i;
        }
        i += 1;
    }
    return pattern.len;
}

/// Match a single byte `ch` against character class `pattern[open_idx..=close_idx]`.
/// Character classes never match path separator `/`.
fn gbmMatchCharClass(pattern: string, open_idx: usize, close_idx: usize, ch: u8): bool {
    if (ch == ('/' as u8)) {
        return false;
    }
    let i: usize = open_idx + 1;
    let negated: bool = false;
    if (i < close_idx && (pattern[i] == ('!' as u8) || pattern[i] == ('^' as u8))) {
        negated = true;
        i += 1;
    }
    let matched: bool = false;
    while (i < close_idx) {
        let c1: u8 = pattern[i];
        if (c1 == ('\\' as u8) && i + 1 < close_idx) {
            i += 1;
            c1 = pattern[i];
        }
        i += 1;
        // Range `a-z` when `-` is followed by another char before `]`.
        if (i + 1 < close_idx && pattern[i] == ('-' as u8)) {
            i += 1;
            let c2: u8 = pattern[i];
            if (c2 == ('\\' as u8) && i + 1 < close_idx) {
                i += 1;
                c2 = pattern[i];
            }
            i += 1;
            if (c1 <= c2 && ch >= c1 && ch <= c2) {
                matched = true;
            }
        } else {
            if (ch == c1) {
                matched = true;
            }
        }
    }
    if (negated) {
        return !matched;
    }
    return matched;
}

/// Core iterative O(n*m) anti-backtracking matcher (no brace expansion).
/// When `record_caps` is true, populates `out_caps` and writes total captured count into `out_count.*`.
fn gbmMatchCore(
    pattern: string,
    path: string,
    record_caps: bool,
    out_caps: []GlobSpan,
    out_count: *usize
): bool {
    let pi: usize = 0;
    let si: usize = 0;

    // Single-segment `*` backtrack state.
    let has_star: bool = false;
    let star_pi: usize = 0;
    let star_si: usize = 0;
    let star_cap_idx: usize = 0;

    // Multi-segment `**` backtrack state.
    let has_dstar: bool = false;
    let dstar_pi: usize = 0;
    let dstar_si: usize = 0;
    let dstar_require_slash: bool = false;
    let dstar_cap_idx: usize = 0;

    let caps: [64]GlobSpan = undefined;
    let cap_count: usize = 0;

    while (pi < pattern.len || si < path.len) {
        let step_ok: bool = false;

        if (pi < pattern.len) {
            const pc: u8 = pattern[pi];

            if (pc == ('*' as u8)) {
                // Check if this is `**` (or `***...`).
                let is_double: bool = (pi + 1 < pattern.len && pattern[pi + 1] == ('*' as u8));
                let next_pi: usize = pi;
                while (next_pi < pattern.len && pattern[next_pi] == ('*' as u8)) {
                    next_pi += 1;
                }

                if (is_double) {
                    // Collapse consecutive `**/` segments such as `**/**/`.
                    let req_slash: bool = false;
                    if (next_pi < pattern.len && pattern[next_pi] == ('/' as u8)) {
                        req_slash = true;
                        next_pi += 1;
                        while (
                            next_pi + 2 < pattern.len &&
                            pattern[next_pi] == ('*' as u8) &&
                            pattern[next_pi + 1] == ('*' as u8) &&
                            pattern[next_pi + 2] == ('/' as u8)
                        ) {
                            next_pi += 3;
                        }
                    }

                    // Record capture slot for `**`.
                    if (record_caps) {
                        dstar_cap_idx = cap_count;
                        if (cap_count < GBM_MAX_CAPTURES) {
                            caps[cap_count] = GlobSpan { start: si, end: si };
                        }
                        cap_count += 1;
                    }

                    has_dstar = true;
                    dstar_pi = next_pi;
                    dstar_si = si;
                    dstar_require_slash = req_slash;
                    // `**` supersedes any earlier single-segment `*`.
                    has_star = false;
                    pi = next_pi;
                    step_ok = true;
                } else {
                    // Single `*` (does not cross `/`).
                    if (record_caps) {
                        star_cap_idx = cap_count;
                        if (cap_count < GBM_MAX_CAPTURES) {
                            caps[cap_count] = GlobSpan { start: si, end: si };
                        }
                        cap_count += 1;
                    }

                    has_star = true;
                    star_pi = next_pi;
                    star_si = si;
                    pi = next_pi;
                    step_ok = true;
                }
            } else if (pc == ('?' as u8)) {
                if (si < path.len && path[si] != ('/' as u8)) {
                    if (record_caps) {
                        if (cap_count < GBM_MAX_CAPTURES) {
                            caps[cap_count] = GlobSpan { start: si, end: si + 1 };
                        }
                        cap_count += 1;
                    }
                    pi += 1;
                    si += 1;
                    step_ok = true;
                }
            } else if (pc == ('[' as u8)) {
                const close_idx: usize = gbmFindClassClose(pattern, pi);
                if (close_idx < pattern.len) {
                    if (si < path.len && gbmMatchCharClass(pattern, pi, close_idx, path[si])) {
                        pi = close_idx + 1;
                        si += 1;
                        step_ok = true;
                    }
                } else {
                    // Unclosed `[` matches literal `[`.
                    if (si < path.len && path[si] == ('[' as u8)) {
                        pi += 1;
                        si += 1;
                        step_ok = true;
                    }
                }
            } else if (pc == ('\\' as u8)) {
                let lit: u8 = '\\' as u8;
                let adv: usize = 1;
                if (pi + 1 < pattern.len) {
                    lit = pattern[pi + 1];
                    adv = 2;
                }
                if (si < path.len && path[si] == lit) {
                    pi += adv;
                    si += 1;
                    step_ok = true;
                }
            } else {
                if (si < path.len && path[si] == pc) {
                    pi += 1;
                    si += 1;
                    step_ok = true;
                }
            }
        }

        if (step_ok) {
            continue;
        }

        // Backtrack to single-segment `*` if possible (cannot consume `/`).
        if (has_star && star_si < path.len && path[star_si] != ('/' as u8)) {
            star_si += 1;
            pi = star_pi;
            si = star_si;
            if (record_caps) {
                cap_count = star_cap_idx + 1;
                if (star_cap_idx < GBM_MAX_CAPTURES) {
                    caps[star_cap_idx].end = star_si;
                }
            }
            continue;
        }

        // Backtrack to multi-segment `**` if possible.
        if (has_dstar && dstar_si < path.len) {
            if (dstar_require_slash) {
                // Advance to right after the next '/' (16-lane SIMD fast-path).
                const slash_pos: usize = gbmFindSlash(path, dstar_si);
                if (slash_pos < path.len) {
                    dstar_si = slash_pos + 1;
                    pi = dstar_pi;
                    si = dstar_si;
                    has_star = false;
                    if (record_caps) {
                        cap_count = dstar_cap_idx + 1;
                        if (dstar_cap_idx < GBM_MAX_CAPTURES) {
                            caps[dstar_cap_idx].end = slash_pos;
                        }
                    }
                    continue;
                }
            } else {
                dstar_si += 1;
                pi = dstar_pi;
                si = dstar_si;
                has_star = false;
                if (record_caps) {
                    cap_count = dstar_cap_idx + 1;
                    if (dstar_cap_idx < GBM_MAX_CAPTURES) {
                        caps[dstar_cap_idx].end = dstar_si;
                    }
                }
                continue;
            }
        }

        return false;
    }

    if (record_caps) {
        let n: usize = cap_count;
        if (n > GBM_MAX_CAPTURES) {
            n = GBM_MAX_CAPTURES;
        }
        if (n > out_caps.len) {
            n = out_caps.len;
        }
        let k: usize = 0;
        while (k < n) {
            out_caps[k] = caps[k];
            k += 1;
        }
        out_count.* = n;
    }
    return true;
}

/// Find the first unescaped single-level brace pair `{...}` outside `[...]` character classes.
/// Writes `open_out.*` and `close_out.*` and returns true if found.
fn gbmFindBracePair(pattern: string, open_out: *usize, close_out: *usize): bool {
    let i: usize = 0;
    const v_bs: GbmVec16 = @splat('\\' as u8);
    const v_lb: GbmVec16 = @splat('[' as u8);
    const v_lc: GbmVec16 = @splat('{' as u8);
    while (i < pattern.len) {
        while (i + 16 <= pattern.len) {
            const v: GbmVec16 = pattern[i .. i + 16][0..16].*;
            if (@reduce(.Or, (v == v_bs) | (v == v_lb) | (v == v_lc))) {
                break;
            }
            i += 16;
        }
        if (i >= pattern.len) {
            break;
        }
        const c: u8 = pattern[i];
        if ((GBM_META_LUT[c as usize] & 2) == 0) {
            i += 1;
            continue;
        }
        if (c == ('\\' as u8) && i + 1 < pattern.len) {
            i += 2;
            continue;
        }
        if (c == ('[' as u8)) {
            const close_cls: usize = gbmFindClassClose(pattern, i);
            if (close_cls < pattern.len) {
                i = close_cls + 1;
                continue;
            }
            i += 1;
            continue;
        }
        if (c == ('{' as u8)) {
            // Find matching unescaped `}` for single-level brace alternation.
            let j: usize = i + 1;
            let open_pos: usize = i;
            while (j < pattern.len) {
                const cj: u8 = pattern[j];
                if (cj == ('\\' as u8) && j + 1 < pattern.len) {
                    j += 2;
                    continue;
                }
                if (cj == ('[' as u8)) {
                    const close_cls: usize = gbmFindClassClose(pattern, j);
                    if (close_cls < pattern.len) {
                        j = close_cls + 1;
                        continue;
                    }
                    j += 1;
                    continue;
                }
                if (cj == ('{' as u8)) {
                    // Inner brace: anchor on the innermost pair.
                    open_pos = j;
                    j += 1;
                    continue;
                }
                if (cj == ('}' as u8)) {
                    open_out.* = open_pos;
                    close_out.* = j;
                    return true;
                }
                j += 1;
            }
            return false;
        }
        i += 1;
    }
    return false;
}

/// Copy slice `src[s..e]` into `dst` starting at `pos`, returning new position.
fn gbmCopySlice(dst: []u8, pos: usize, src: string, s: usize, e: usize): usize {
    if (e <= s || pos >= dst.len) {
        return pos;
    }
    const len: usize = e - s;
    const avail: usize = dst.len - pos;
    const n: usize = if (len <= avail) len else avail;
    @memcpy(dst[pos .. pos + n], src[s .. s + n]);
    return pos + n;
}

/// Recursive single-level brace alternation driver with strict depth & step budgets.
fn gbmMatchWithBraces(
    pattern: string,
    path: string,
    depth: usize,
    steps: *usize,
    record_caps: bool,
    out_caps: []GlobSpan,
    out_count: *usize
): bool {
    if (steps.* >= GBM_MAX_BRACE_STEPS) {
        return false;
    }
    steps.* = steps.* + 1;

    let open_idx: usize = 0;
    let close_idx: usize = 0;
    if (depth >= GBM_MAX_BRACE_DEPTH || !gbmFindBracePair(pattern, &open_idx, &close_idx)) {
        return gbmMatchCore(pattern, path, record_caps, out_caps, out_count);
    }

    const prefix_len: usize = open_idx;
    const suffix_start: usize = close_idx + 1;
    const suffix_len: usize = pattern.len - suffix_start;

    let buf: [2048]u8 = undefined;
    let alt_start: usize = open_idx + 1;
    let i: usize = open_idx + 1;

    while (i <= close_idx) {
        let is_sep: bool = (i == close_idx);
        if (!is_sep) {
            if (pattern[i] == ('\\' as u8) && i + 1 < close_idx) {
                i += 2;
                continue;
            }
            if (pattern[i] == ('[' as u8)) {
                const close_cls: usize = gbmFindClassClose(pattern, i);
                if (close_cls < close_idx) {
                    i = close_cls + 1;
                    continue;
                }
            }
            if (pattern[i] == (',' as u8)) {
                is_sep = true;
            }
        }

        if (is_sep) {
            const alt_len: usize = i - alt_start;
            const total_len: usize = prefix_len + alt_len + suffix_len;
            if (total_len <= GBM_MAX_PATTERN_BUF) {
                let p: usize = 0;
                p = gbmCopySlice(buf[0..GBM_MAX_PATTERN_BUF], p, pattern, 0, open_idx);
                p = gbmCopySlice(buf[0..GBM_MAX_PATTERN_BUF], p, pattern, alt_start, i);
                p = gbmCopySlice(buf[0..GBM_MAX_PATTERN_BUF], p, pattern, suffix_start, pattern.len);
                const expanded: string = buf[0..p];
                if (gbmMatchWithBraces(expanded, path, depth + 1, steps, record_caps, out_caps, out_count)) {
                    return true;
                }
            }
            alt_start = i + 1;
        }
        i += 1;
    }

    return false;
}

/// Match `path` against glob `pattern` with zero heap allocations and anti-backtracking guarantees.
///
/// Supported syntax:
/// - `*`: matches zero or more non-`/` characters
/// - `**`: matches zero or more path segments across `/`
/// - `?`: matches any single non-`/` character
/// - `[abc]`, `[a-z]`, `[!0-9]`, `[^0-9]`: character classes (never match `/`)
/// - `{a,b,c}`: single-level brace alternation (multiple brace groups supported)
/// - `\x`: escape literal character `x`
pub fn globMatch(pattern: string, path: string): bool {
    let steps: usize = 0;
    let dummy_caps: [1]GlobSpan = undefined;
    let dummy_count: usize = 0;
    return gbmMatchWithBraces(pattern, path, 0, &steps, false, dummy_caps[0..0], &dummy_count);
}

/// Returns true if any pattern in `patterns` matches `path`.
pub fn globMatchAny(patterns: []const string, path: string): bool {
    let i: usize = 0;
    while (i < patterns.len) {
        if (globMatch(patterns[i], path)) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Match `path` against `pattern` and extract wildcard (`*`, `**`, `?`) capture byte ranges into `out_caps`.
/// Returns the number of captured spans written into `out_caps`, or `0` if the pattern does not match
/// (or contains no wildcard captures).
pub fn globMatchCaptures(pattern: string, path: string, out_caps: []GlobSpan): usize {
    let steps: usize = 0;
    let count: usize = 0;
    const ok: bool = gbmMatchWithBraces(pattern, path, 0, &steps, true, out_caps, &count);
    if (!ok) {
        return 0;
    }
    return count;
}

/// Returns true if `pattern` contains no unescaped glob wildcard metacharacters (`*`, `?`, `[`, `{`).
pub fn isStaticGlob(pattern: string): bool {
    let i: usize = 0;
    const v_star: GbmVec16 = @splat('*' as u8);
    const v_q: GbmVec16 = @splat('?' as u8);
    const v_lb: GbmVec16 = @splat('[' as u8);
    const v_lc: GbmVec16 = @splat('{' as u8);
    const v_bs: GbmVec16 = @splat('\\' as u8);
    while (i < pattern.len) {
        while (i + 16 <= pattern.len) {
            const v: GbmVec16 = pattern[i .. i + 16][0..16].*;
            if (@reduce(.Or, (v == v_star) | (v == v_q) | (v == v_lb) | (v == v_lc) | (v == v_bs))) {
                break;
            }
            i += 16;
        }
        if (i >= pattern.len) {
            break;
        }
        const c: u8 = pattern[i];
        if (c == ('\\' as u8) && i + 1 < pattern.len) {
            i += 2;
            continue;
        }
        if ((GBM_META_LUT[c as usize] & 1) != 0) {
            return false;
        }
        i += 1;
    }
    return true;
}
