// @cap/picomatch — Zero-allocation glob matcher & static base scanner for CP
// Clean-room MIT implementation with parity to npm `picomatch` v4.
// Reuses `@cap/normalize-path` and `@cap/is-extglob` with zero duplication.

import { normalizePath, hasBackslash } from "../../normalize-path/src/index.cp";
import { isExtglob } from "../../is-extglob/src/index.cp";
import { toLowerByte, rfindByte } from "cap:ascii";
import { findByteInBytes, bytesEq } from "cap:bin";

const PM_MAX_BUF: usize = 1024;
const PM_MAX_BRACE_DEPTH: usize = 16;
const PM_MAX_MATCH_DEPTH: usize = 256;

/// Matching options for `@cap/picomatch`.
pub struct PicoOptions {
    pub dot: bool,
    pub nocase: bool,
    pub basename: bool,
    pub noglobstar: bool,

    /// Construct default options (`dot: false`, `nocase: false`, `basename: false`, `noglobstar: false`).
    pub static fn defaults(): PicoOptions {
        return PicoOptions {
            dot: false,
            nocase: false,
            basename: false,
            noglobstar: false,
        };
    }
}

fn pmCharEq(a: u8, b: u8, nocase: bool): bool {
    if (nocase) {
        return toLowerByte(a) == toLowerByte(b);
    }
    return a == b;
}

fn pmBasenameSlice(path: string): string {
    if (lastSlash := rfindByte(path, 47)) {
        return path[(lastSlash + 1)..path.len];
    }
    return path;
}

fn pmIsLeadingDot(path: string, pIdx: usize): bool {
    if (pIdx >= path.len) {
        return false;
    }
    if (path[pIdx] != 46) {
        return false;
    }
    return pIdx == 0 || path[pIdx - 1] == 47;
}

fn pmCountLeadingNegations(pat: string): usize {
    let count: usize = 0;
    while (count < pat.len && pat[count] == 33) {
        // Do not treat extglob `!(...)` as leading negation
        if (count + 1 < pat.len && pat[count + 1] == 40 && isExtglob(pat[count..pat.len])) {
            break;
        }
        count = count + 1;
    }
    return count;
}

// Check if remaining path from `pIdx` contains any dotfile segment when `!opts.dot`.
fn pmHasHiddenSegment(path: string, pIdx: usize): bool {
    let i: usize = pIdx;
    while (i < path.len) {
        if (dotPos := findByteInBytes(path, 46, i)) {
            if (dotPos == 0 || path[dotPos - 1] == 47) {
                return true;
            }
            i = dotPos + 1;
        } else {
            return false;
        }
    }
    return false;
}

// Find closing `]` for a character class starting at `pat[gIdx] == '['`.
// Returns index of `]`, or `0` if unclosed.
fn pmFindClassEnd(pat: string, gIdx: usize): usize {
    let i: usize = gIdx + 1;
    if (i < pat.len && (pat[i] == 33 || pat[i] == 94)) {
        i = i + 1;
    }
    // A `]` immediately after `[` or `[!` / `[^` is treated as a literal member
    if (i < pat.len && pat[i] == 93) {
        i = i + 1;
    }
    const scanStart = i;
    while (i < pat.len) {
        if (closePos := findByteInBytes(pat, 93, i)) {
            // Count consecutive preceding backslashes down to scanStart
            let bs: usize = 0;
            let k: usize = closePos;
            while (k > scanStart && pat[k - 1] == 92) {
                bs = bs + 1;
                k = k - 1;
            }
            if ((bs & 1) == 0) {
                return closePos;
            }
            i = closePos + 1;
        } else {
            return 0;
        }
    }
    return 0;
}

// Match a single byte `ch` against character class `pat[gIdx..endIdx+1]`.
fn pmMatchCharClass(ch: u8, pat: string, gIdx: usize, endIdx: usize, nocase: bool): bool {
    if (ch == 47) {
        return false;
    }
    let i: usize = gIdx + 1;
    let negated = false;
    if (i < endIdx && (pat[i] == 33 || pat[i] == 94)) {
        negated = true;
        i = i + 1;
    }
    const target: u8 = if (nocase) toLowerByte(ch) else ch;
    let matched = false;
    while (i < endIdx) {
        let lo: u8 = pat[i];
        if (lo == 92 && i + 1 < endIdx) {
            lo = pat[i + 1];
            i = i + 2;
        } else {
            i = i + 1;
        }
        if (i + 1 < endIdx && pat[i] == 45) {
            // Range lo-hi
            i = i + 1;
            let hi: u8 = pat[i];
            if (hi == 92 && i + 1 < endIdx) {
                hi = pat[i + 1];
                i = i + 2;
            } else {
                i = i + 1;
            }
            const loC: u8 = if (nocase) toLowerByte(lo) else lo;
            const hiC: u8 = if (nocase) toLowerByte(hi) else hi;
            if (target >= loC && target <= hiC) {
                matched = true;
            }
        } else {
            const loC: u8 = if (nocase) toLowerByte(lo) else lo;
            if (target == loC) {
                matched = true;
            }
        }
    }
    if (negated) {
        return !matched;
    }
    return matched;
}

fn pmMatchCore(path: string, pIdx: usize, pat: string, gIdx: usize, opts: PicoOptions, depth: usize): bool {
    if (depth > PM_MAX_MATCH_DEPTH) {
        return false;
    }
    let pi: usize = pIdx;
    let gi: usize = gIdx;

    while (gi < pat.len) {
        const gch = pat[gi];

        // Globstar `**` when enabled and on segment boundary
        if (gch == 42 && !opts.noglobstar && gi + 1 < pat.len && pat[gi + 1] == 42) {
            const atSegStart = gi == 0 || pat[gi - 1] == 47;
            // Skip any consecutive '*'
            let afterStars: usize = gi + 2;
            while (afterStars < pat.len && pat[afterStars] == 42) {
                afterStars = afterStars + 1;
            }
            const atSegEnd = afterStars == pat.len || pat[afterStars] == 47;

            if (atSegStart && atSegEnd) {
                if (afterStars == pat.len) {
                    // Trailing `**`: matches everything remaining if no hidden segment violates !opts.dot
                    if (!opts.dot && pmHasHiddenSegment(path, pi)) {
                        return false;
                    }
                    return true;
                }
                // `**/` followed by more pattern
                const nextGi = afterStars + 1;
                // Case 1: `**/` matches 0 directory segments
                if (pmMatchCore(path, pi, pat, nextGi, opts, depth + 1)) {
                    return true;
                }
                // Case 2: `**/` consumes 1+ directory segments (16-lane SIMD '/' scan)
                let cur: usize = pi;
                while (cur < path.len) {
                    if (!opts.dot && pmIsLeadingDot(path, cur)) {
                        return false;
                    }
                    if (slashPos := findByteInBytes(path, 47, cur)) {
                        cur = slashPos + 1;
                        if (pmMatchCore(path, cur, pat, nextGi, opts, depth + 1)) {
                            return true;
                        }
                    } else {
                        return false;
                    }
                }
                return false;
            }
        }

        // Single-segment wildcard `*`
        if (gch == 42) {
            // Collapse adjacent `*`
            let nextGi: usize = gi + 1;
            while (nextGi < pat.len && pat[nextGi] == 42) {
                nextGi = nextGi + 1;
            }
            // Try matching 0 chars first
            if (pmMatchCore(path, pi, pat, nextGi, opts, depth + 1)) {
                return true;
            }
            // Dotfile protection: `*` cannot match a leading `.` of a segment unless opts.dot
            if (!opts.dot && pmIsLeadingDot(path, pi)) {
                return false;
            }
            // Determine segment boundary via 16-lane SIMD '/' scan
            const segEnd = findByteInBytes(path, 47, pi) ?? path.len;
            // Fast path: trailing `*` matches remainder iff no '/' remains
            if (nextGi == pat.len) {
                return segEnd == path.len;
            }
            // Consume non-'/' characters
            let cur: usize = pi;
            while (cur < segEnd) {
                cur = cur + 1;
                if (pmMatchCore(path, cur, pat, nextGi, opts, depth + 1)) {
                    return true;
                }
            }
            return false;
        }

        // Single-character wildcard `?`
        if (gch == 63) {
            if (pi >= path.len || path[pi] == 47) {
                return false;
            }
            if (!opts.dot && pmIsLeadingDot(path, pi)) {
                return false;
            }
            pi = pi + 1;
            gi = gi + 1;
            continue;
        }

        // Character class `[...]`
        if (gch == 91) {
            const endIdx = pmFindClassEnd(pat, gi);
            if (endIdx > 0) {
                if (pi >= path.len) {
                    return false;
                }
                if (!opts.dot && pmIsLeadingDot(path, pi)) {
                    return false;
                }
                if (!pmMatchCharClass(path[pi], pat, gi, endIdx, opts.nocase)) {
                    return false;
                }
                pi = pi + 1;
                gi = endIdx + 1;
                continue;
            }
        }

        // Backslash escape `\x`
        if (gch == 92 && gi + 1 < pat.len) {
            gi = gi + 1;
            if (pi >= path.len || !pmCharEq(path[pi], pat[gi], opts.nocase)) {
                return false;
            }
            pi = pi + 1;
            gi = gi + 1;
            continue;
        }

        // Literal character / run match (16-lane SIMD bytesEq when case-sensitive)
        if (!opts.nocase) {
            let litEnd: usize = gi + 1;
            while (litEnd < pat.len) {
                const c = pat[litEnd];
                if (c == 42 || c == 63 || c == 91 || c == 92) {
                    break;
                }
                litEnd = litEnd + 1;
            }
            const litLen = litEnd - gi;
            if (litLen > 1) {
                if (pi + litLen > path.len) {
                    return false;
                }
                if (!bytesEq(path[pi..(pi + litLen)], pat[gi..litEnd])) {
                    return false;
                }
                pi = pi + litLen;
                gi = litEnd;
                continue;
            }
        }
        if (pi >= path.len || !pmCharEq(path[pi], gch, opts.nocase)) {
            return false;
        }
        pi = pi + 1;
        gi = gi + 1;
    }

    return pi == path.len;
}

// Find the first top-level `{a,b,...}` brace group containing at least one top-level comma.
// Returns `true` and fills `outBounds[0] = openIdx`, `outBounds[1] = closeIdx`.
fn pmFindBraceGroup(pat: string, outBounds: []usize): bool {
    let i: usize = 0;
    while (i < pat.len) {
        const ch = pat[i];
        if (ch == 92 && i + 1 < pat.len) {
            i = i + 2;
            continue;
        }
        if (ch == 91) {
            const endClass = pmFindClassEnd(pat, i);
            if (endClass > 0) {
                i = endClass + 1;
                continue;
            }
        }
        if (ch == 123) {
            // Found '{' — scan for matching '}' and check for a depth-1 comma
            let depth: usize = 1;
            let hasComma = false;
            let j: usize = i + 1;
            while (j < pat.len && depth > 0) {
                const c2 = pat[j];
                if (c2 == 92 && j + 1 < pat.len) {
                    j = j + 2;
                    continue;
                }
                if (c2 == 123) {
                    depth = depth + 1;
                } else if (c2 == 125) {
                    depth = depth - 1;
                    if (depth == 0) {
                        if (hasComma) {
                            outBounds[0] = i;
                            outBounds[1] = j;
                            return true;
                        }
                        break;
                    }
                } else if (c2 == 44 && depth == 1) {
                    hasComma = true;
                }
                j = j + 1;
            }
        }
        i = i + 1;
    }
    return false;
}

fn pmMatchWithBraces(path: string, pat: string, opts: PicoOptions, depth: usize): bool {
    if (depth > PM_MAX_BRACE_DEPTH) {
        return pmMatchCore(path, 0, pat, 0, opts, 0);
    }
    let bounds: [2]usize = undefined;
    if (!pmFindBraceGroup(pat, bounds[0..2])) {
        return pmMatchCore(path, 0, pat, 0, opts, 0);
    }

    const openIdx = bounds[0];
    const closeIdx = bounds[1];
    const suffixStart = closeIdx + 1;
    const suffixLen = pat.len - suffixStart;

    let altStart: usize = openIdx + 1;
    let cur: usize = altStart;
    let nested: usize = 0;

    while (cur <= closeIdx) {
        let isSplit = false;
        if (cur == closeIdx) {
            isSplit = true;
        } else if (pat[cur] == 92 && cur + 1 < closeIdx) {
            cur = cur + 2;
            continue;
        } else if (pat[cur] == 123) {
            nested = nested + 1;
        } else if (pat[cur] == 125 && nested > 0) {
            nested = nested - 1;
        } else if (pat[cur] == 44 && nested == 0) {
            isSplit = true;
        }

        if (isSplit) {
            const altLen = cur - altStart;
            const totalLen = openIdx + altLen + suffixLen;
            if (totalLen <= PM_MAX_BUF) {
                let buf: [1024]u8 = undefined;
                if (openIdx > 0) {
                    @memcpy(buf[0..openIdx], pat[0..openIdx]);
                }
                if (altLen > 0) {
                    @memcpy(buf[openIdx..(openIdx + altLen)], pat[altStart..cur]);
                }
                if (suffixLen > 0) {
                    @memcpy(buf[(openIdx + altLen)..totalLen], pat[suffixStart..pat.len]);
                }
                if (pmMatchWithBraces(path, buf[0..totalLen], opts, depth + 1)) {
                    return true;
                }
            }
            altStart = cur + 1;
        }
        cur = cur + 1;
    }

    return false;
}

fn pmMatchNormalized(normPath: string, pat: string, opts: PicoOptions): bool {
    if (pat.len == 0) {
        return false;
    }
    const negCount = pmCountLeadingNegations(pat);
    const negated = (negCount % 2) == 1;
    const corePat = pat[negCount..pat.len];
    if (corePat.len == 0) {
        return false;
    }

    let targetPath = normPath;
    if (opts.basename && findByteInBytes(corePat, 47, 0) == null) {
        targetPath = pmBasenameSlice(normPath);
    }

    const hit = pmMatchWithBraces(targetPath, corePat, opts, 0);
    if (negated) {
        return !hit;
    }
    return hit;
}

fn pmSegmentHasGlob(seg: string): bool {
    if (isExtglob(seg)) {
        return true;
    }
    let i: usize = 0;
    while (i < seg.len) {
        const ch = seg[i];
        if (ch == 92 && i + 1 < seg.len) {
            i = i + 2;
            continue;
        }
        if (ch == 42 || ch == 63 || ch == 91 || ch == 93 || ch == 123 || ch == 125) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

/// Test whether `path` matches `pattern` with explicit `PicoOptions`.
/// Automatically normalizes Windows backslashes via `@cap/normalize-path`.
pub fn isMatchWithOptions(path: string, pattern: string, opts: PicoOptions): bool {
    if (pattern.len == 0) {
        return false;
    }
    let pathBuf: [1024]u8 = undefined;
    let normPath = path;
    if (path.len > 0 && path.len <= PM_MAX_BUF) {
        const nLen = normalizePath(path, false, pathBuf[0..1024]);
        normPath = pathBuf[0..nLen];
    }
    return pmMatchNormalized(normPath, pattern, opts);
}

/// Test whether `path` matches `pattern` using default `PicoOptions`.
pub fn isMatch(path: string, pattern: string): bool {
    return isMatchWithOptions(path, pattern, PicoOptions.defaults());
}

/// Test whether `path` matches a list of positive and/or negative (`!pattern`) glob rules.
/// Positive rules include matching paths; negative rules exclude previously matched paths.
/// If all rules are negative, returns `true` unless excluded by at least one rule.
pub fn isMatchAny(path: string, patterns: []const string, opts: PicoOptions): bool {
    if (patterns.len == 0) {
        return false;
    }
    let pathBuf: [1024]u8 = undefined;
    let normPath = path;
    if (path.len > 0 && path.len <= PM_MAX_BUF) {
        const nLen = normalizePath(path, false, pathBuf[0..1024]);
        normPath = pathBuf[0..nLen];
    }

    let hasPositive = false;
    let matched = false;
    let excluded = false;
    let i: usize = 0;
    while (i < patterns.len) {
        const pat = patterns[i];
        if (pat.len > 0) {
            const negCount = pmCountLeadingNegations(pat);
            const isNeg = (negCount % 2) == 1;
            const inner = pat[negCount..pat.len];
            if (inner.len > 0) {
                let targetPath = normPath;
                if (opts.basename && findByteInBytes(inner, 47, 0) == null) {
                    targetPath = pmBasenameSlice(normPath);
                }
                const hit = pmMatchWithBraces(targetPath, inner, opts, 0);
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

/// Extract the static (non-glob) base directory prefix of `pattern` into `out`
/// (equivalent to `picomatch.scan(pattern).base`).
/// Example: `"src/lib/**/*.cp"` -> `"src/lib"`, `"**/*.cp"` -> `""`.
/// Returns bytes written (`0` if no static prefix or `out` is too small).
pub fn scanBase(pattern: string, out: []u8): usize {
    if (pattern.len == 0 || out.len == 0) {
        return 0;
    }

    // Strip leading negation `!` before normalizing
    const negCount = pmCountLeadingNegations(pattern);
    const raw = pattern[negCount..pattern.len];
    if (raw.len == 0) {
        return 0;
    }

    let normBuf: [1024]u8 = undefined;
    let pat = raw;
    if (hasBackslash(raw) && raw.len <= PM_MAX_BUF) {
        const nLen = normalizePath(raw, true, normBuf[0..1024]);
        pat = normBuf[0..nLen];
    }

    let baseEnd: usize = 0;
    let hasBase = false;
    let i: usize = 0;

    // Preserve leading root '/'
    if (pat.len > 0 && pat[0] == 47) {
        i = 1;
    }

    while (i < pat.len) {
        const segStart = i;
        i = findByteInBytes(pat, 47, i) ?? pat.len;
        const seg = pat[segStart..i];
        if (pmSegmentHasGlob(seg)) {
            break;
        }
        baseEnd = i;
        hasBase = true;
        if (i < pat.len && pat[i] == 47) {
            i = i + 1;
        }
    }

    if (!hasBase || baseEnd == 0 || out.len < baseEnd) {
        return 0;
    }
    @memcpy(out[0..baseEnd], pat[0..baseEnd]);
    return baseEnd;
}

export {
    PicoOptions,
    isMatch,
    isMatchWithOptions,
    isMatchAny,
    scanBase,
};
