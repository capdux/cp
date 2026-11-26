// packages/@cap/glob-match/src/index.cp
// Zero-allocation POSIX & .gitignore glob path matching engine (ReDoS-safe).
// MIT clean-room pure CP implementation.

fn gmIsSpace(c: u8): bool {
    return c == (' ' as u8) || c == ('\t' as u8) || c == ('\r' as u8) || c == ('\n' as u8);
}

fn gmTrim(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && gmIsSpace(s[start])) {
        start += 1;
    }
    while (end > start && gmIsSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn gmContainsByte(s: []const u8, target: u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == target) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Match a POSIX character class `[...]` starting at `pattern[pi] == '['` against byte `ch`.
/// If a closing `']'` exists, writes the index right after `']'` into `next_pi.*` and returns
/// `1` (matched) or `0` (not matched). If no closing `']'` exists, returns `-1` (treat `'['` literally).
fn gmMatchCharClass(pattern: []const u8, pi: usize, ch: u8, next_pi: *usize): i32 {
    if (ch == ('/' as u8)) {
        // Still advance past class if well-formed, but POSIX character class never matches '/'.
        let scan = pi + 1;
        if (scan < pattern.len && (pattern[scan] == ('!' as u8) || pattern[scan] == ('^' as u8))) {
            scan += 1;
        }
        if (scan < pattern.len && pattern[scan] == (']' as u8)) {
            scan += 1;
        }
        while (scan < pattern.len) {
            if (pattern[scan] == ('\\' as u8) && scan + 1 < pattern.len) {
                scan += 2;
            } else if (pattern[scan] == (']' as u8)) {
                next_pi.* = scan + 1;
                return 0 as i32;
            } else {
                scan += 1;
            }
        }
        return -1 as i32;
    }

    let i = pi + 1;
    if (i >= pattern.len) {
        return -1 as i32;
    }

    let negated = false;
    if (pattern[i] == ('!' as u8) || pattern[i] == ('^' as u8)) {
        negated = true;
        i += 1;
    }

    let matched = false;
    let first = true;
    while (i < pattern.len) {
        if (!first && pattern[i] == (']' as u8)) {
            next_pi.* = i + 1;
            if (negated) {
                if (!matched) { return 1 as i32; }
                return 0 as i32;
            }
            if (matched) { return 1 as i32; }
            return 0 as i32;
        }
        first = false;

        let lo = pattern[i];
        if (lo == ('\\' as u8) && i + 1 < pattern.len) {
            lo = pattern[i + 1];
            i += 2;
        } else {
            i += 1;
        }

        // Check for range `lo-hi`.
        if (i + 1 < pattern.len && pattern[i] == ('-' as u8) && pattern[i + 1] != (']' as u8)) {
            i += 1; // skip '-'
            let hi = pattern[i];
            if (hi == ('\\' as u8) && i + 1 < pattern.len) {
                hi = pattern[i + 1];
                i += 2;
            } else {
                i += 1;
            }
            if (ch >= lo && ch <= hi) {
                matched = true;
            }
        } else {
            if (ch == lo) {
                matched = true;
            }
        }
    }

    // Unclosed '[' -> treat as literal '['.
    return -1 as i32;
}

/// Non-recursive, ReDoS-safe POSIX glob path matcher.
/// Supports:
/// - `*`: matches 0+ non-`/` characters within a single path segment.
/// - `**`: matches 0+ path segments across `/` boundaries (`src/**/*.cp`, `**/*.md`, `a/**/b`).
/// - `?`: matches any single non-`/` character.
/// - `[abc]`, `[a-z]`, `[!0-9]`, `[^0-9]`: character classes (never match `/`).
/// - `\x`: escaped literal character.
pub fn isMatch(pattern: string, path: string): bool {
    let pi: usize = 0;
    let si: usize = 0;

    let hasStar = false;
    let starPi: usize = 0;
    let starSi: usize = 0;

    let hasGlobstar = false;
    let gsPi: usize = 0;
    let gsSi: usize = 0;
    let gsSlashOnly = false;

    while (true) {
        if (pi < pattern.len) {
            const pc = pattern[pi];

            // Wildcard '*' or '**'.
            if (pc == ('*' as u8)) {
                let isDouble = false;
                if (pi + 1 < pattern.len && pattern[pi + 1] == ('*' as u8)) {
                    isDouble = true;
                }
                while (pi < pattern.len && pattern[pi] == ('*' as u8)) {
                    pi += 1;
                }

                if (isDouble) {
                    if (pi < pattern.len && pattern[pi] == ('/' as u8)) {
                        // '**/': matches 0 or more full directory segments.
                        pi += 1;
                        hasGlobstar = true;
                        gsPi = pi;
                        gsSi = si;
                        gsSlashOnly = true;
                        hasStar = false;
                    } else {
                        // Trailing '**' or '**suffix': matches any chars including '/'.
                        hasGlobstar = true;
                        gsPi = pi;
                        gsSi = si;
                        gsSlashOnly = false;
                        hasStar = false;
                    }
                } else {
                    // Single '*': matches within a single segment (cannot cross '/').
                    hasStar = true;
                    starPi = pi;
                    starSi = si;
                }
                continue;
            }

            // Single-char '?' (cannot match '/').
            if (pc == ('?' as u8)) {
                if (si < path.len && path[si] != ('/' as u8)) {
                    pi += 1;
                    si += 1;
                    continue;
                }
            } else if (pc == ('[' as u8)) {
                if (si < path.len) {
                    let nextPi: usize = 0;
                    const clsRes = gmMatchCharClass(pattern, pi, path[si], &nextPi);
                    if (clsRes == (1 as i32)) {
                        pi = nextPi;
                        si += 1;
                        continue;
                    } else if (clsRes == (-1 as i32) && path[si] == ('[' as u8)) {
                        pi += 1;
                        si += 1;
                        continue;
                    }
                }
            } else if (pc == ('\\' as u8)) {
                let lit = pc;
                let step: usize = 1;
                if (pi + 1 < pattern.len) {
                    lit = pattern[pi + 1];
                    step = 2;
                }
                if (si < path.len && path[si] == lit) {
                    pi += step;
                    si += 1;
                    continue;
                }
            } else {
                if (si < path.len && path[si] == pc) {
                    pi += 1;
                    si += 1;
                    continue;
                }
            }
        } else if (si == path.len) {
            return true;
        }

        // Backtrack: prefer single-segment '*' first (cannot cross '/').
        if (hasStar && starSi < path.len && path[starSi] != ('/' as u8)) {
            starSi += 1;
            pi = starPi;
            si = starSi;
            continue;
        }

        // Backtrack: cross-directory '**'.
        if (hasGlobstar && gsSi < path.len) {
            if (gsSlashOnly) {
                // Advance gsSi past the next '/' to maintain segment boundary alignment.
                let nextSlash = gsSi;
                while (nextSlash < path.len && path[nextSlash] != ('/' as u8)) {
                    nextSlash += 1;
                }
                if (nextSlash < path.len) {
                    gsSi = nextSlash + 1;
                    pi = gsPi;
                    si = gsSi;
                    hasStar = false;
                    continue;
                }
            } else {
                gsSi += 1;
                pi = gsPi;
                si = gsSi;
                hasStar = false;
                continue;
            }
        }

        return false;
    }

    return false;
}

/// Returns true if `path` matches any glob pattern in `patterns`.
pub fn isMatchAny(patterns: []const string, path: string): bool {
    let i: usize = 0;
    while (i < patterns.len) {
        if (isMatch(patterns[i], path)) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn gmBasename(path: []const u8): []const u8 {
    let lastSlash = path.len;
    let i: usize = 0;
    while (i < path.len) {
        if (path[i] == ('/' as u8)) {
            lastSlash = i;
        }
        i += 1;
    }
    if (lastSlash < path.len) {
        return path[(lastSlash + 1)..path.len];
    }
    return path;
}

fn gmRuleMatchesPath(rule: []const u8, path: []const u8): bool {
    let pat = rule;
    let anchored = false;
    if (pat.len > 0 && pat[0] == ('/' as u8)) {
        anchored = true;
        pat = pat[1..pat.len];
    }
    if (pat.len == 0) {
        return false;
    }

    // Trailing '/' matches a directory and everything inside it.
    if (pat[pat.len - 1] == ('/' as u8)) {
        const dirPat = pat[0..(pat.len - 1)];
        if (dirPat.len == 0) {
            return false;
        }
        if (isMatch(dirPat, path)) {
            return true;
        }
        // Check if path has a prefix segment matching dirPat followed by '/'.
        let i: usize = 0;
        while (i < path.len) {
            if (path[i] == ('/' as u8)) {
                if (isMatch(dirPat, path[0..i])) {
                    return true;
                }
                if (!anchored && !gmContainsByte(dirPat, '/' as u8)) {
                    if (isMatch(dirPat, gmBasename(path[0..i]))) {
                        return true;
                    }
                }
            }
            i += 1;
        }
        return false;
    }

    if (isMatch(pat, path)) {
        return true;
    }

    // Unanchored rule without '/' matches against the basename at any directory depth (.gitignore semantics).
    if (!anchored && !gmContainsByte(pat, '/' as u8)) {
        if (isMatch(pat, gmBasename(path))) {
            return true;
        }
    }
    return false;
}

/// Evaluate `.gitignore` rules sequentially against `path`:
/// - Blank lines and `#` comment lines are ignored.
/// - Rules prefixed with `!` negate previous ignore matches.
/// - Unanchored single-segment patterns (e.g. `*.log`, `important.log`) match at any depth.
pub fn isIgnored(rules: []const string, path: string): bool {
    let ignored = false;
    let i: usize = 0;
    while (i < rules.len) {
        let line = gmTrim(rules[i]);
        if (line.len == 0 || line[0] == ('#' as u8)) {
            i += 1;
            continue;
        }
        let negated = false;
        if (line[0] == ('!' as u8)) {
            negated = true;
            line = line[1..line.len];
        }
        if (line.len > 0 && gmRuleMatchesPath(line, path)) {
            ignored = !negated;
        }
        i += 1;
    }
    return ignored;
}

/// Returns true if `pattern` contains any unescaped glob metacharacter (`*`, `?`, `[`, `{`).
pub fn hasGlobMagic(pattern: string): bool {
    let i: usize = 0;
    while (i < pattern.len) {
        const c = pattern[i];
        if (c == ('\\' as u8) && i + 1 < pattern.len) {
            i += 2;
            continue;
        }
        if (c == ('*' as u8) || c == ('?' as u8) || c == ('[' as u8) || c == ('{' as u8)) {
            return true;
        }
        i += 1;
    }
    return false;
}

export {
    isMatch,
    isMatchAny,
    isIgnored,
    hasGlobMagic,
};
