// @cap/micromatch — Zero-allocation glob matching suite for CP
// Clean-room MIT implementation with full parity to npm `micromatch` v4.
// Reuses `@cap/picomatch` for core glob matching and `@cap/braces` for
// numeric/alphabetic range & set expansion with zero duplication.

import {
    PicoOptions,
    isMatchWithOptions,
    isMatchAny,
    scanBase,
} from "../../picomatch/src/index.cp";
import {
    BraceResult,
    expandBraces,
} from "../../braces/src/index.cp";

const MM_BRACE_BUF_SIZE: usize = 4096;
const MM_PAT_BUF_SIZE: usize = 1024;

fn mmHasBrace(s: string): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 123) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

fn mmCountLeadingNeg(pat: string): usize {
    let c: usize = 0;
    while (c < pat.len && pat[c] == 33) {
        if (c + 1 < pat.len && pat[c + 1] == 40) {
            break;
        }
        c = c + 1;
    }
    return c;
}

fn mmMatchPositiveExpanded(path: string, inner: string, opts: PicoOptions): bool {
    if (inner.len == 0) {
        return false;
    }
    if (!mmHasBrace(inner)) {
        return isMatchWithOptions(path, inner, opts);
    }
    let buf: [4096]u8 = undefined;
    const expanded: BraceResult = expandBraces(inner, buf[0..MM_BRACE_BUF_SIZE]);
    if (expanded.count == 0) {
        return isMatchWithOptions(path, inner, opts);
    }
    let i: usize = 0;
    while (i < expanded.count) {
        if (isMatchWithOptions(path, expanded.itemAt(i), opts)) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

fn mmWriteWrapped(dst: []u8, prefix: string, mid: string, suffix: string): usize {
    const total = prefix.len + mid.len + suffix.len;
    if (total > dst.len) {
        return 0;
    }
    let w: usize = 0;
    let i: usize = 0;
    while (i < prefix.len) {
        dst[w] = prefix[i];
        w = w + 1;
        i = i + 1;
    }
    i = 0;
    while (i < mid.len) {
        dst[w] = mid[i];
        w = w + 1;
        i = i + 1;
    }
    i = 0;
    while (i < suffix.len) {
        dst[w] = suffix[i];
        w = w + 1;
        i = i + 1;
    }
    return w;
}

fn mmContainsSinglePositive(path: string, pat: string, opts: PicoOptions): bool {
    if (path.len == 0 || pat.len == 0) {
        return false;
    }
    if (isMatchWithOptions(path, pat, opts)) {
        return true;
    }
    let tmp: [1024]u8 = undefined;
    let n: usize = 0;

    // Match at end of path: `**/pat`
    n = mmWriteWrapped(tmp[0..MM_PAT_BUF_SIZE], "**/", pat, "");
    if (n > 0 && isMatchWithOptions(path, tmp[0..n], opts)) {
        return true;
    }
    // Match at start of path: `pat/**`
    n = mmWriteWrapped(tmp[0..MM_PAT_BUF_SIZE], "", pat, "/**");
    if (n > 0 && isMatchWithOptions(path, tmp[0..n], opts)) {
        return true;
    }
    // Match as middle segment(s): `**/pat/**`
    n = mmWriteWrapped(tmp[0..MM_PAT_BUF_SIZE], "**/", pat, "/**");
    if (n > 0 && isMatchWithOptions(path, tmp[0..n], opts)) {
        return true;
    }
    // Match as partial fragment in final segment: `**/*pat*`
    n = mmWriteWrapped(tmp[0..MM_PAT_BUF_SIZE], "**/*", pat, "*");
    if (n > 0 && isMatchWithOptions(path, tmp[0..n], opts)) {
        return true;
    }
    // Match as partial fragment in intermediate segment: `**/*pat*/**`
    n = mmWriteWrapped(tmp[0..MM_PAT_BUF_SIZE], "**/*", pat, "*/**");
    if (n > 0 && isMatchWithOptions(path, tmp[0..n], opts)) {
        return true;
    }
    return false;
}

/// Test whether `path` matches `pattern` with `PicoOptions`.
/// Automatically expands numeric/alphabetic brace ranges (e.g. `file-{1..5}.cp`) via `@cap/braces`
/// and delegates glob evaluation to `@cap/picomatch`.
pub fn mmIsMatch(path: string, pattern: string, opts: PicoOptions): bool {
    if (pattern.len == 0) {
        return false;
    }
    const negCount = mmCountLeadingNeg(pattern);
    const negated = (negCount % 2) == 1;
    const inner = pattern[negCount..pattern.len];
    if (inner.len == 0) {
        return false;
    }
    const hit = mmMatchPositiveExpanded(path, inner, opts);
    if (negated) {
        return !hit;
    }
    return hit;
}

/// Return `true` iff `path` matches EVERY pattern in `patterns`.
/// Returns `false` if `patterns` is empty.
pub fn mmAll(path: string, patterns: []const string, opts: PicoOptions): bool {
    if (patterns.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < patterns.len) {
        if (!mmIsMatch(path, patterns[i], opts)) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// Return `true` iff `path` matches at least one positive pattern and is not excluded
/// by a subsequent negative (`!`) pattern. Supports `@cap/braces` range expansion.
pub fn mmAny(path: string, patterns: []const string, opts: PicoOptions): bool {
    if (patterns.len == 0) {
        return false;
    }
    // Fast path: no braces in any pattern -> delegate directly to `@cap/picomatch.isMatchAny`
    let anyBrace = false;
    let checkIdx: usize = 0;
    while (checkIdx < patterns.len) {
        if (mmHasBrace(patterns[checkIdx])) {
            anyBrace = true;
            break;
        }
        checkIdx = checkIdx + 1;
    }
    if (!anyBrace) {
        return isMatchAny(path, patterns, opts);
    }

    let hasPositive = false;
    let matched = false;
    let excluded = false;
    let i: usize = 0;
    while (i < patterns.len) {
        const pat = patterns[i];
        if (pat.len > 0) {
            const negCount = mmCountLeadingNeg(pat);
            const isNeg = (negCount % 2) == 1;
            const inner = pat[negCount..pat.len];
            if (inner.len > 0) {
                const hit = mmMatchPositiveExpanded(path, inner, opts);
                if (isNeg) {
                    if (hit) {
                        matched = false;
                        excluded = true;
                    }
                } else {
                    hasPositive = true;
                    if (hit) {
                        matched = true;
                    }
                }
            }
        }
        i = i + 1;
    }

    if (hasPositive) {
        return matched;
    }
    return !excluded;
}

/// Return `true` iff at least one path in `paths` matches `patterns`.
pub fn mmSome(paths: []const string, patterns: []const string, opts: PicoOptions): bool {
    if (paths.len == 0 || patterns.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < paths.len) {
        if (mmAny(paths[i], patterns, opts)) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

/// Return `true` iff EVERY path in `paths` matches `patterns`.
/// Returns `false` if `paths` or `patterns` is empty.
pub fn mmEvery(paths: []const string, patterns: []const string, opts: PicoOptions): bool {
    if (paths.len == 0 || patterns.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < paths.len) {
        if (!mmAny(paths[i], patterns, opts)) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// Filter `paths` against `patterns`, writing matching indices into `out_indices`.
/// Returns the number of indices written.
pub fn mmFilter(paths: []const string, patterns: []const string, opts: PicoOptions, out_indices: []usize): usize {
    if (paths.len == 0 || patterns.len == 0) {
        return 0;
    }
    let written: usize = 0;
    let i: usize = 0;
    while (i < paths.len) {
        if (mmAny(paths[i], patterns, opts)) {
            if (written < out_indices.len) {
                out_indices[written] = i;
                written = written + 1;
            }
        }
        i = i + 1;
    }
    return written;
}

/// Inverse filter: write indices of paths in `paths` that do NOT match `patterns` into `out_indices`.
/// Returns the number of indices written.
pub fn mmNot(paths: []const string, patterns: []const string, opts: PicoOptions, out_indices: []usize): usize {
    if (paths.len == 0) {
        return 0;
    }
    let written: usize = 0;
    let i: usize = 0;
    while (i < paths.len) {
        if (!mmAny(paths[i], patterns, opts)) {
            if (written < out_indices.len) {
                out_indices[written] = i;
                written = written + 1;
            }
        }
        i = i + 1;
    }
    return written;
}

/// Return `true` if `path` contains a sub-path or segment fragment matching `pattern`.
pub fn mmContains(path: string, pattern: string, opts: PicoOptions): bool {
    if (path.len == 0 || pattern.len == 0) {
        return false;
    }
    const negCount = mmCountLeadingNeg(pattern);
    const negated = (negCount % 2) == 1;
    const inner = pattern[negCount..pattern.len];
    if (inner.len == 0) {
        return false;
    }

    let hit = false;
    if (!mmHasBrace(inner)) {
        hit = mmContainsSinglePositive(path, inner, opts);
    } else {
        let buf: [4096]u8 = undefined;
        const expanded: BraceResult = expandBraces(inner, buf[0..MM_BRACE_BUF_SIZE]);
        if (expanded.count == 0) {
            hit = mmContainsSinglePositive(path, inner, opts);
        } else {
            let i: usize = 0;
            while (i < expanded.count) {
                if (mmContainsSinglePositive(path, expanded.itemAt(i), opts)) {
                    hit = true;
                    break;
                }
                i = i + 1;
            }
        }
    }

    if (negated) {
        return !hit;
    }
    return hit;
}

export {
    PicoOptions,
    scanBase,
    mmIsMatch,
    mmAll,
    mmAny,
    mmSome,
    mmEvery,
    mmFilter,
    mmNot,
    mmContains,
};
