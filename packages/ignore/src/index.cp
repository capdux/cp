// @cap/ignore — Zero-heap .gitignore specification rule engine
// Clean-room MIT CP implementation of npm `ignore` (250M+/wk, ESLint / Prettier / Git tooling)
// Reuses `@cap/minimatch` and `@cap/normalize-path` with zero inline duplication.

import { MinimatchOptions, minimatchWithOptions } from "../../minimatch/src/index.cp";
import { normalizePath } from "../../normalize-path/src/index.cp";

pub const IGNORE_MAX_RULES: usize = 64;
pub const IGNORE_MAX_PAT_LEN: usize = 128;

pub struct IgnoreRule {
    pat: [128]u8,
    pat_len: usize,
    negated: bool,
    dir_only: bool,
    anchored: bool,
}

fn igTrimRightSpaces(s: string): usize {
    let end = s.len;
    while (end > 0 && (s[end - 1] == 32 || s[end - 1] == 9 || s[end - 1] == 13)) {
        // Respect escaped trailing space "\ "
        if (end >= 2 && s[end - 2] == 92 && s[end - 1] == 32) {
            break;
        }
        end = end - 1;
    }
    return end;
}

fn igHasInnerSlash(s: string, start: usize, end: usize): bool {
    let i = start;
    while (i < end) {
        if (s[i] == 47) { return true; }
        i = i + 1;
    }
    return false;
}

fn igRuleMatches(rule: IgnoreRule, rel_path: string, is_dir: bool, case_insensitive: bool): bool {
    if (rule.dir_only && !is_dir) {
        // A dir-only rule `build/` also ignores files inside `build/...`
        let pat_slice = rule.pat[0..rule.pat_len];
        // Check if any directory prefix of rel_path matches rule
        let i: usize = 0;
        while (i < rel_path.len) {
            if (rel_path[i] == 47) {
                let prefix = rel_path[0..i];
                if (igMatchSingle(pat_slice, rule.anchored, prefix, case_insensitive)) {
                    return true;
                }
            }
            i = i + 1;
        }
        return false;
    }

    let pat_slice = rule.pat[0..rule.pat_len];
    if (igMatchSingle(pat_slice, rule.anchored, rel_path, case_insensitive)) {
        return true;
    }
    // Also check if any parent directory of rel_path is ignored
    let i: usize = 0;
    while (i < rel_path.len) {
        if (rel_path[i] == 47) {
            let prefix = rel_path[0..i];
            if (igMatchSingle(pat_slice, rule.anchored, prefix, case_insensitive)) {
                return true;
            }
        }
        i = i + 1;
    }
    return false;
}

fn igMatchSingle(pat: []const u8, anchored: bool, path: string, case_insensitive: bool): bool {
    let opts = MinimatchOptions.defaults();
    opts.dot = true;
    opts.nocase = case_insensitive;
    opts.nonegate = true;
    opts.nocomment = true;
    opts.match_base = !anchored;
    return minimatchWithOptions(path, pat, opts);
}

/// Zero-heap `.gitignore` rule matcher (up to 64 rules).
pub struct IgnoreMatcher {
    rules: [64]IgnoreRule,
    count: usize,
    ignore_case: bool,

    pub static fn new(): IgnoreMatcher {
        return IgnoreMatcher {
            rules: undefined,
            count: 0,
            ignore_case: false,
        };
    }

    pub static fn newCaseInsensitive(): IgnoreMatcher {
        return IgnoreMatcher {
            rules: undefined,
            count: 0,
            ignore_case: true,
        };
    }

    pub fn ruleCount(): usize {
        return _.count;
    }

    /// Adds a single `.gitignore` rule line (skips blank lines and `#` comments).
    pub fn addRule(*_, raw_line: string): bool {
        let end = igTrimRightSpaces(raw_line);
        if (end == 0) { return false; }
        let start: usize = 0;
        if (raw_line[0] == 35) { return false; } // '#' comment

        let negated = false;
        if (raw_line[start] == 33) { // '!'
            negated = true;
            start = start + 1;
        } else if (start + 1 < end && raw_line[start] == 92 && (raw_line[start + 1] == 35 || raw_line[start + 1] == 33)) {
            start = start + 1; // unescape \# or \!
        }
        if (start >= end) { return false; }

        let dir_only = false;
        if (raw_line[end - 1] == 47) {
            dir_only = true;
            end = end - 1;
        }
        if (start >= end) { return false; }

        let anchored = false;
        if (raw_line[start] == 47) {
            anchored = true;
            start = start + 1;
        } else if (igHasInnerSlash(raw_line, start, end)) {
            anchored = true;
        }
        if (start >= end) { return false; }
        if (_.count >= IGNORE_MAX_RULES) { return false; }

        let pat_len = end - start;
        if (pat_len > IGNORE_MAX_PAT_LEN) { pat_len = IGNORE_MAX_PAT_LEN; }
        let r = IgnoreRule {
            pat: undefined,
            pat_len: pat_len,
            negated: negated,
            dir_only: dir_only,
            anchored: anchored,
        };
        let i: usize = 0;
        while (i < pat_len) {
            r.pat[i] = raw_line[start + i];
            i = i + 1;
        }
        _.rules[_.count] = r;
        _.count = _.count + 1;
        return true;
    }

    /// Parses a multi-line `.gitignore` file content and adds all active rules.
    /// Returns the number of rules added.
    pub fn addLines(*_, content: string): usize {
        let added: usize = 0;
        let line_start: usize = 0;
        let i: usize = 0;
        while (i <= content.len) {
            if (i == content.len || content[i] == 10) {
                if (_.addRule(content[line_start..i])) {
                    added = added + 1;
                }
                line_start = i + 1;
            }
            i = i + 1;
        }
        return added;
    }

    /// Returns `true` if `path` is ignored by the accumulated `.gitignore` rules.
    /// Trailing `/` on `path` marks it as a directory.
    pub fn ignores(path: string): bool {
        if (path.len == 0 || _.count == 0) { return false; }
        let norm_buf: [256]u8 = undefined;
        let is_dir = path[path.len - 1] == 47 || path[path.len - 1] == 92;
        let nlen = normalizePath(path, true, norm_buf[0..256]);
        if (nlen == 0) { return false; }
        // Strip leading "./" or "/"
        let s: usize = 0;
        if (nlen >= 2 && norm_buf[0] == 46 && norm_buf[1] == 47) {
            s = 2;
        } else if (norm_buf[0] == 47) {
            s = 1;
        }
        if (s >= nlen) { return false; }
        let rel = norm_buf[s..nlen];

        let ignored = false;
        let i: usize = 0;
        while (i < _.count) {
            let r = _.rules[i];
            if (igRuleMatches(r, rel, is_dir, _.ignore_case)) {
                ignored = !r.negated;
            }
            i = i + 1;
        }
        return ignored;
    }

    /// Filters `paths`, writing indices of NON-ignored paths into `out_indices`.
    pub fn filter(paths: []const string, out_indices: []usize): usize {
        let kept: usize = 0;
        let i: usize = 0;
        while (i < paths.len) {
            if (!_.ignores(paths[i])) {
                if (kept < out_indices.len) {
                    out_indices[kept] = i;
                }
                kept = kept + 1;
            }
            i = i + 1;
        }
        if (kept > out_indices.len) { return out_indices.len; }
        return kept;
    }
}
