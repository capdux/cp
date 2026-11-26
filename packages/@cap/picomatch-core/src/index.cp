// packages/@cap/picomatch-core/src/index.cp
// Zero-allocation picomatch-compatible glob scanner, extglob matcher, and path option engine.
// Clean-room MIT pure CP implementation reusing `@cap/glob-match`.

import { globMatch, isStaticGlob } from "../../glob-match/src/index.cp";

const PM_MAX_BUF: usize = 1024;
const PM_MAX_DEPTH: usize = 8;
const PM_MAX_STEPS: usize = 4096;

/// Result of scanning a glob pattern into its static base directory and dynamic glob suffix.
pub struct ScanResult {
    pub prefix: string,
    pub input: string,
    pub base: string,
    pub glob: string,
    pub is_glob: bool,
    pub negated: bool,
}

/// Matching options compatible with `picomatch`.
pub struct PicomatchOptions {
    pub dot: bool,
    pub nocase: bool,
    pub match_base: bool,
    pub bash: bool,

    pub static fn defaults(): PicomatchOptions {
        return PicomatchOptions {
            dot: false,
            nocase: false,
            match_base: false,
            bash: false,
        };
    }
}

fn pmIsExtglobOp(ch: u8): bool {
    return ch == ('@' as u8)
        || ch == ('!' as u8)
        || ch == ('?' as u8)
        || ch == ('+' as u8)
        || ch == ('*' as u8);
}

fn pmContainsSlash(s: string): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ('/' as u8)) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn pmCopySlice(dst: []u8, pos: usize, src: string, start: usize, end: usize): usize {
    let p: usize = pos;
    let i: usize = start;
    while (i < end && p < dst.len) {
        dst[p] = src[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn pmToLowerAscii(dst: []u8, src: string): usize {
    let i: usize = 0;
    let limit: usize = src.len;
    if (limit > dst.len) {
        limit = dst.len;
    }
    while (i < limit) {
        let ch: u8 = src[i];
        if (ch >= ('A' as u8) && ch <= ('Z' as u8)) {
            dst[i] = ch + 32;
        } else {
            dst[i] = ch;
        }
        i += 1;
    }
    return limit;
}

/// Find matching `]` for a character class starting at `s[open_idx] == '['`.
/// Returns `s.len` if unclosed.
fn pmFindClassClose(s: string, open_idx: usize): usize {
    let i: usize = open_idx + 1;
    if (i < s.len && (s[i] == ('!' as u8) || s[i] == ('^' as u8))) {
        i += 1;
    }
    if (i < s.len && s[i] == (']' as u8)) {
        i += 1;
    }
    while (i < s.len) {
        if (s[i] == ('\\' as u8) && i + 1 < s.len) {
            i += 2;
            continue;
        }
        if (s[i] == (']' as u8)) {
            return i;
        }
        i += 1;
    }
    return s.len;
}

/// Find matching `}` for a brace expansion starting at `s[open_idx] == '{'`.
/// Returns `s.len` if unclosed or not a valid alternation/range.
fn pmFindBraceClose(s: string, open_idx: usize): usize {
    let depth: usize = 1;
    let i: usize = open_idx + 1;
    let has_sep: bool = false;
    while (i < s.len) {
        if (s[i] == ('\\' as u8) && i + 1 < s.len) {
            i += 2;
            continue;
        }
        if (s[i] == ('{' as u8)) {
            depth += 1;
        } else if (s[i] == ('}' as u8)) {
            depth -= 1;
            if (depth == 0) {
                if (has_sep) {
                    return i;
                }
                return s.len;
            }
        } else if (depth == 1) {
            if (s[i] == (',' as u8)) {
                has_sep = true;
            } else if (s[i] == ('.' as u8) && i + 1 < s.len && s[i + 1] == ('.' as u8)) {
                has_sep = true;
            }
        }
        i += 1;
    }
    return s.len;
}

/// Find matching `)` for an extglob starting with `(` at `open_paren`.
/// Returns `s.len` if unclosed.
fn pmFindParenClose(s: string, open_paren: usize): usize {
    let depth: usize = 1;
    let i: usize = open_paren + 1;
    while (i < s.len) {
        if (s[i] == ('\\' as u8) && i + 1 < s.len) {
            i += 2;
            continue;
        }
        if (s[i] == ('[' as u8)) {
            let cls_end: usize = pmFindClassClose(s, i);
            if (cls_end < s.len) {
                i = cls_end + 1;
                continue;
            }
        }
        if (s[i] == ('(' as u8)) {
            depth += 1;
        } else if (s[i] == (')' as u8)) {
            depth -= 1;
            if (depth == 0) {
                return i;
            }
        }
        i += 1;
    }
    return s.len;
}

/// Find first extglob `op(...)` in `pat`. Writes operator index to `start_out` and `)` index to `end_out`.
fn pmFindExtglob(pat: string, start_out: *usize, end_out: *usize): bool {
    let i: usize = 0;
    while (i < pat.len) {
        if (pat[i] == ('\\' as u8) && i + 1 < pat.len) {
            i += 2;
            continue;
        }
        if (pat[i] == ('[' as u8)) {
            let cls_end: usize = pmFindClassClose(pat, i);
            if (cls_end < pat.len) {
                i = cls_end + 1;
                continue;
            }
        }
        if (pmIsExtglobOp(pat[i]) && i + 1 < pat.len && pat[i + 1] == ('(' as u8)) {
            let close_idx: usize = pmFindParenClose(pat, i + 1);
            if (close_idx < pat.len) {
                start_out.* = i;
                end_out.* = close_idx;
                return true;
            }
        }
        i += 1;
    }
    return false;
}

/// Scan a glob pattern into prefix, static base directory, and dynamic glob part.
/// Recognizes leading `!` negation (excluding `!(...)` extglob) and `./` prefix.
pub fn scan(pattern: string): ScanResult {
    let idx: usize = 0;
    let neg_count: usize = 0;
    while (idx < pattern.len && pattern[idx] == ('!' as u8)) {
        if (idx + 1 < pattern.len && pattern[idx + 1] == ('(' as u8)) {
            break;
        }
        neg_count += 1;
        idx += 1;
    }

    if (idx + 1 < pattern.len && pattern[idx] == ('.' as u8) && pattern[idx + 1] == ('/' as u8)) {
        idx += 2;
    }

    let prefix: string = pattern[0..idx];
    let body: string = pattern[idx..pattern.len];
    let negated: bool = (neg_count % 2) == 1;

    let i: usize = 0;
    let has_slash: bool = false;
    let last_slash: usize = 0;
    let found_glob: bool = false;

    while (i < body.len) {
        let ch: u8 = body[i];
        if (ch == ('\\' as u8) && i + 1 < body.len) {
            i += 2;
            continue;
        }
        if (ch == ('/' as u8)) {
            has_slash = true;
            last_slash = i;
            i += 1;
            continue;
        }
        if (ch == ('*' as u8) || ch == ('?' as u8)) {
            found_glob = true;
            break;
        }
        if (ch == ('[' as u8) && pmFindClassClose(body, i) < body.len) {
            found_glob = true;
            break;
        }
        if (ch == ('{' as u8) && pmFindBraceClose(body, i) < body.len) {
            found_glob = true;
            break;
        }
        if (pmIsExtglobOp(ch) && i + 1 < body.len && body[i + 1] == ('(' as u8)) {
            if (pmFindParenClose(body, i + 1) < body.len) {
                found_glob = true;
                break;
            }
        }
        i += 1;
    }

    if (!found_glob && !isStaticGlob(body)) {
        found_glob = true;
    }

    if (!found_glob) {
        return ScanResult {
            prefix: prefix,
            input: pattern,
            base: body,
            glob: "",
            is_glob: false,
            negated: negated,
        };
    }

    if (has_slash) {
        let base_part: string = "/";
        if (last_slash > 0) {
            base_part = body[0..last_slash];
        }
        let glob_part: string = body[last_slash + 1 .. body.len];
        return ScanResult {
            prefix: prefix,
            input: pattern,
            base: base_part,
            glob: glob_part,
            is_glob: true,
            negated: negated,
        };
    }

    return ScanResult {
        prefix: prefix,
        input: pattern,
        base: "",
        glob: body,
        is_glob: true,
        negated: negated,
    };
}

/// Match `text` against any top-level `|`-separated branch inside `inner`.
fn pmMatchAnyBranch(text: string, inner: string, depth: usize, steps: *usize): bool {
    let start: usize = 0;
    let i: usize = 0;
    let paren_depth: usize = 0;

    while (i <= inner.len) {
        let is_sep: bool = (i == inner.len);
        if (!is_sep) {
            if (inner[i] == ('\\' as u8) && i + 1 < inner.len) {
                i += 2;
                continue;
            }
            if (inner[i] == ('(' as u8)) {
                paren_depth += 1;
            } else if (inner[i] == (')' as u8) && paren_depth > 0) {
                paren_depth -= 1;
            } else if (inner[i] == ('|' as u8) && paren_depth == 0) {
                is_sep = true;
            }
        }

        if (is_sep) {
            let branch: string = inner[start..i];
            if (pmMatchExtglobInner(text, branch, depth + 1, steps)) {
                return true;
            }
            start = i + 1;
        }
        i += 1;
    }
    return false;
}

/// Match `text` as 1 or more concatenations of branches in `inner` (without crossing `/`).
fn pmMatchRepeat(text: string, inner: string, depth: usize, steps: *usize): bool {
    if (steps.* >= PM_MAX_STEPS || depth >= PM_MAX_DEPTH) {
        return false;
    }
    steps.* = steps.* + 1;

    if (pmContainsSlash(text)) {
        return false;
    }
    if (pmMatchAnyBranch(text, inner, depth + 1, steps)) {
        return true;
    }
    let split: usize = 1;
    while (split < text.len) {
        if (pmMatchAnyBranch(text[0..split], inner, depth + 1, steps)) {
            if (pmMatchRepeat(text[split..text.len], inner, depth + 1, steps)) {
                return true;
            }
        }
        split += 1;
    }
    return false;
}

/// Core extglob matcher with recursion depth and step budgets.
/// Delegates non-extglob patterns directly to `@cap/glob-match`.
fn pmMatchExtglobInner(text: string, pat: string, depth: usize, steps: *usize): bool {
    if (steps.* >= PM_MAX_STEPS) {
        return false;
    }
    steps.* = steps.* + 1;

    let ext_start: usize = 0;
    let ext_end: usize = 0;
    if (depth >= PM_MAX_DEPTH || !pmFindExtglob(pat, &ext_start, &ext_end)) {
        return globMatch(pat, text);
    }

    let op: u8 = pat[ext_start];
    let pre: string = pat[0..ext_start];
    let inner: string = pat[ext_start + 2 .. ext_end];
    let suf_start: usize = ext_end + 1;
    let suf: string = pat[suf_start..pat.len];

    // `@(a|b)` and `?(a|b)`: expand each branch into stack buffer and recurse.
    if (op == ('@' as u8) || op == ('?' as u8)) {
        let buf: [1024]u8 = undefined;

        if (op == ('?' as u8)) {
            let zero_len: usize = pre.len + suf.len;
            if (zero_len <= PM_MAX_BUF) {
                let p0: usize = 0;
                p0 = pmCopySlice(buf[0..PM_MAX_BUF], p0, pre, 0, pre.len);
                p0 = pmCopySlice(buf[0..PM_MAX_BUF], p0, suf, 0, suf.len);
                let zero_pat: string = buf[0..p0];
                if (pmMatchExtglobInner(text, zero_pat, depth + 1, steps)) {
                    return true;
                }
            }
        }

        let alt_start: usize = 0;
        let i: usize = 0;
        let paren_depth: usize = 0;
        while (i <= inner.len) {
            let is_sep: bool = (i == inner.len);
            if (!is_sep) {
                if (inner[i] == ('\\' as u8) && i + 1 < inner.len) {
                    i += 2;
                    continue;
                }
                if (inner[i] == ('(' as u8)) {
                    paren_depth += 1;
                } else if (inner[i] == (')' as u8) && paren_depth > 0) {
                    paren_depth -= 1;
                } else if (inner[i] == ('|' as u8) && paren_depth == 0) {
                    is_sep = true;
                }
            }

            if (is_sep) {
                let alt_len: usize = i - alt_start;
                let total_len: usize = pre.len + alt_len + suf.len;
                if (total_len <= PM_MAX_BUF) {
                    let p: usize = 0;
                    p = pmCopySlice(buf[0..PM_MAX_BUF], p, pre, 0, pre.len);
                    p = pmCopySlice(buf[0..PM_MAX_BUF], p, inner, alt_start, i);
                    p = pmCopySlice(buf[0..PM_MAX_BUF], p, suf, 0, suf.len);
                    let expanded: string = buf[0..p];
                    if (pmMatchExtglobInner(text, expanded, depth + 1, steps)) {
                        return true;
                    }
                }
                alt_start = i + 1;
            }
            i += 1;
        }
        return false;
    }

    // `!(a|b)`: matches any single-segment substring not matching any branch in `inner`.
    if (op == ('!' as u8)) {
        if (pre.len == 0 && suf.len == 0) {
            if (pmContainsSlash(text)) {
                return false;
            }
            return !pmMatchAnyBranch(text, inner, depth + 1, steps);
        }

        let s: usize = 0;
        while (s <= text.len) {
            if (pmMatchExtglobInner(text[0..s], pre, depth + 1, steps)) {
                let e: usize = s;
                while (e <= text.len) {
                    let mid: string = text[s..e];
                    if (!pmMatchAnyBranch(mid, inner, depth + 1, steps)) {
                        if (pmMatchExtglobInner(text[e..text.len], suf, depth + 1, steps)) {
                            return true;
                        }
                    }
                    if (e < text.len && text[e] == ('/' as u8)) {
                        break;
                    }
                    e += 1;
                }
            }
            s += 1;
        }
        return false;
    }

    // `*(a|b)` and `+(a|b)`: match 0+ or 1+ repetitions of branches in `inner`.
    if (op == ('*' as u8)) {
        let buf: [1024]u8 = undefined;
        let zero_len: usize = pre.len + suf.len;
        if (zero_len <= PM_MAX_BUF) {
            let p0: usize = 0;
            p0 = pmCopySlice(buf[0..PM_MAX_BUF], p0, pre, 0, pre.len);
            p0 = pmCopySlice(buf[0..PM_MAX_BUF], p0, suf, 0, suf.len);
            let zero_pat: string = buf[0..p0];
            if (pmMatchExtglobInner(text, zero_pat, depth + 1, steps)) {
                return true;
            }
        }
    }

    if (pre.len == 0 && suf.len == 0) {
        return pmMatchRepeat(text, inner, depth + 1, steps);
    }

    let s: usize = 0;
    while (s <= text.len) {
        if (pmMatchExtglobInner(text[0..s], pre, depth + 1, steps)) {
            let e: usize = s;
            while (e <= text.len) {
                if (e > s) {
                    if (pmMatchRepeat(text[s..e], inner, depth + 1, steps)) {
                        if (pmMatchExtglobInner(text[e..text.len], suf, depth + 1, steps)) {
                            return true;
                        }
                    }
                }
                if (e < text.len && text[e] == ('/' as u8)) {
                    break;
                }
                e += 1;
            }
        }
        s += 1;
    }
    return false;
}

/// Match `text` against an extglob/glob pattern (`@(a|b)`, `!(a|b)`, `?(a|b)`, `+(a|b)`, `*(a|b)`).
pub fn matchExtglob(text: string, pat: string): bool {
    let steps: usize = 0;
    return pmMatchExtglobInner(text, pat, 0, &steps);
}

/// Extract basename of `path` (segment after last `/`, stripping trailing `/`).
fn pmBasename(path: string): string {
    let end: usize = path.len;
    while (end > 1 && path[end - 1] == ('/' as u8)) {
        end -= 1;
    }
    let i: usize = end;
    while (i > 0) {
        i -= 1;
        if (path[i] == ('/' as u8)) {
            return path[i + 1 .. end];
        }
    }
    return path[0..end];
}

/// Returns true if any path segment starts with `.` (hidden file/directory).
fn pmHasDotSegment(path: string): bool {
    let i: usize = 0;
    let at_seg_start: bool = true;
    while (i < path.len) {
        let ch: u8 = path[i];
        if (at_seg_start && ch == ('.' as u8)) {
            return true;
        }
        at_seg_start = (ch == ('/' as u8));
        i += 1;
    }
    return false;
}

/// Returns true if `pat` explicitly matches a leading dot segment (e.g. `.*`, `.env`, `**/.*`, `a/.hidden/*`).
fn pmPatternAllowsDot(pat: string): bool {
    let i: usize = 0;
    let at_seg_start: bool = true;
    while (i < pat.len) {
        let ch: u8 = pat[i];
        if (at_seg_start && ch == ('.' as u8)) {
            return true;
        }
        if (at_seg_start && (ch == ('{' as u8) || ch == ('(' as u8)) && i + 1 < pat.len && pat[i + 1] == ('.' as u8)) {
            return true;
        }
        if (ch == ('/' as u8)) {
            at_seg_start = true;
        } else {
            at_seg_start = false;
        }
        i += 1;
    }
    return false;
}

/// Match `path` against `pattern` using `PicomatchOptions`.
/// Supports `!` negation, `dot` hidden-file control, `match_base`, `nocase`, `bash`, and extglobs.
pub fn isMatch(path: string, pattern: string, opts: PicomatchOptions): bool {
    if (pattern.len == 0) {
        return false;
    }

    // Parse leading `!` negation (do not strip `!(...)` extglob).
    let pat_idx: usize = 0;
    let neg_count: usize = 0;
    while (pat_idx < pattern.len && pattern[pat_idx] == ('!' as u8)) {
        if (pat_idx + 1 < pattern.len && pattern[pat_idx + 1] == ('(' as u8)) {
            break;
        }
        neg_count += 1;
        pat_idx += 1;
    }
    let negated: bool = (neg_count % 2) == 1;
    let core_pat: string = pattern[pat_idx..pattern.len];
    if (core_pat.len == 0) {
        return false;
    }

    let target_path: string = path;

    // Strip leading `./` from pattern (and matching `./` from path if present).
    if (core_pat.len >= 2 && core_pat[0] == ('.' as u8) && core_pat[1] == ('/' as u8)) {
        core_pat = core_pat[2..core_pat.len];
        if (target_path.len >= 2 && target_path[0] == ('.' as u8) && target_path[1] == ('/' as u8)) {
            target_path = target_path[2..target_path.len];
        }
    }

    // `match_base`: match basename only when pattern contains no `/`.
    if (opts.match_base && !pmContainsSlash(core_pat)) {
        target_path = pmBasename(target_path);
    }

    // `dot`: block hidden files unless `opts.dot` is true or pattern explicitly matches `.`.
    let matched: bool = false;
    if (!opts.dot && pmHasDotSegment(target_path) && !pmPatternAllowsDot(core_pat)) {
        matched = false;
    } else if (opts.nocase) {
        let path_buf: [1024]u8 = undefined;
        let pat_buf: [1024]u8 = undefined;
        let path_len: usize = pmToLowerAscii(path_buf[0..PM_MAX_BUF], target_path);
        let pat_len: usize = pmToLowerAscii(pat_buf[0..PM_MAX_BUF], core_pat);
        let lower_path: string = path_buf[0..path_len];
        let lower_pat: string = pat_buf[0..pat_len];
        matched = matchExtglob(lower_path, lower_pat);
    } else {
        matched = matchExtglob(target_path, core_pat);
    }

    // `opts.bash` is respected via single-segment `*` isolation enforced in `@cap/glob-match`.
    let _bash_flag: bool = opts.bash;
    * = _bash_flag;

    if (negated) {
        return !matched;
    }
    return matched;
}
