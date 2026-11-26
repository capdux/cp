// @cap/anymatch — Zero-heap file path & glob matcher in pure CP
// MIT License — Composes @cap/normalize-path and @cap/is-glob directly

import { normalizePathDefault } from "../../normalize-path/src/index.cp";
import { isGlobDefault } from "../../is-glob/src/index.cp";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn startsWithDirPrefix(path: []const u8, prefix: []const u8): bool {
    if (prefix.len == 0 || path.len <= prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (path[i] != prefix[i]) {
            return false;
        }
        i += 1;
    }
    if (prefix.len == 1 && prefix[0] == 47) {
        return true;
    }
    return path[prefix.len] == 47; // '/'
}

/// Zero-heap glob matcher supporting `*` (single segment), `**` / `**/` (multi-segment), and `?`.
fn globMatchAt(pat: []const u8, pi_in: usize, txt: []const u8, ti_in: usize): bool {
    let pi: usize = pi_in;
    let ti: usize = ti_in;

    while (pi < pat.len) {
        if (pi + 1 < pat.len && pat[pi] == 42 && pat[pi + 1] == 42) { // '**'
            let np: usize = pi + 2;
            while (np < pat.len && pat[np] == 42) {
                np += 1;
            }
            if (np == pat.len) {
                return true;
            }
            if (pat[np] == 47) { // '**/' matches zero or more directory segments
                if (globMatchAt(pat, np + 1, txt, ti)) {
                    return true;
                }
                let k: usize = ti;
                while (k < txt.len) {
                    if (txt[k] == 47 && globMatchAt(pat, np + 1, txt, k + 1)) {
                        return true;
                    }
                    k += 1;
                }
                return false;
            }
            // '**' not followed by '/'
            let k: usize = ti;
            while (k <= txt.len) {
                if (globMatchAt(pat, np, txt, k)) {
                    return true;
                }
                k += 1;
            }
            return false;
        } else if (pat[pi] == 42) { // '*' matches 0+ non-'/' bytes
            let k: usize = ti;
            while (true) {
                if (globMatchAt(pat, pi + 1, txt, k)) {
                    return true;
                }
                if (k >= txt.len || txt[k] == 47) {
                    return false;
                }
                k += 1;
            }
        } else if (pat[pi] == 63) { // '?' matches 1 non-'/' byte
            if (ti >= txt.len || txt[ti] == 47) {
                return false;
            }
            pi += 1;
            ti += 1;
        } else {
            if (ti >= txt.len || pat[pi] != txt[ti]) {
                return false;
            }
            pi += 1;
            ti += 1;
        }
    }
    return ti == txt.len;
}

/// Match a single pattern against `test_path`:
/// - Normalizes both Windows & POSIX paths via `@cap/normalize-path`
/// - Uses `@cap/is-glob` to dispatch between exact/directory-prefix match and `*`/`**`/`?` glob match
pub fn matchPattern(pattern: string, test_path: string): bool {
    if (pattern.len == 0 || test_path.len == 0) {
        return false;
    }
    let pat_buf: [512]u8 = undefined;
    let path_buf: [512]u8 = undefined;
    const pat_len: usize = normalizePathDefault(pattern, &pat_buf);
    const path_len: usize = normalizePathDefault(test_path, &path_buf);
    if (pat_len == 0 || path_len == 0) {
        return false;
    }
    const norm_pat: string = pat_buf[0..pat_len];
    const norm_path: string = path_buf[0..path_len];

    if (!isGlobDefault(norm_pat)) {
        if (bytesEq(norm_pat, norm_path)) {
            return true;
        }
        return startsWithDirPrefix(norm_path, norm_pat);
    }
    return globMatchAt(norm_pat, 0, norm_path, 0);
}

/// Return the index of the first matching positive pattern in `patterns`, or `-1` if
/// no positive pattern matches or if any `!pattern` negation rule excludes `test_path`.
pub fn anymatchIndex(patterns: []const string, test_path: string): i64 {
    if (patterns.len == 0 || test_path.len == 0) {
        return -1;
    }

    // Check negation rules first: any matching `!pattern` excludes the path
    let i: usize = 0;
    while (i < patterns.len) {
        const pat: string = patterns[i];
        if (pat.len > 1 && pat[0] == 33) { // '!'
            if (matchPattern(pat[1..pat.len], test_path)) {
                return -1;
            }
        }
        i += 1;
    }

    // Find first matching positive rule
    let j: usize = 0;
    while (j < patterns.len) {
        const pat: string = patterns[j];
        if (pat.len > 0 && pat[0] != 33) {
            if (matchPattern(pat, test_path)) {
                return j as i64;
            }
        }
        j += 1;
    }
    return -1;
}

/// Return true if `test_path` matches at least one positive pattern and no `!pattern` negation rule.
pub fn anymatch(patterns: []const string, test_path: string): bool {
    return anymatchIndex(patterns, test_path) >= 0;
}

/// Count how many patterns in `patterns` are glob patterns (delegating to `@cap/is-glob`).
pub fn countGlobPatterns(patterns: []const string): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < patterns.len) {
        const pat: string = patterns[i];
        const body: string = if (pat.len > 1 && pat[0] == 33) pat[1..pat.len] else pat;
        if (isGlobDefault(body)) {
            total += 1;
        }
        i += 1;
    }
    return total;
}
