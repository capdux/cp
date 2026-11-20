// Source: cp/lib/git/ignore.cp (Gitignore Pattern Engine with Negation & Double-Star)
// Pure CP `.gitignore` pattern compiler and matcher supporting `#` comments,
// `!` negation (last-match-wins), trailing `/` directory-only rules,
// leading `/` and internal `/` anchoring, `**` multi-directory globs, and `*`/`?`/`[...]` wildcards.

const std = @import("std");
import { GitError } from "./object.cp";

struct IgnoreRule {
    pattern_buf: []u8,
    negated: bool,
    dir_only: bool,
    anchored: bool,
}

fn stripTrailingUnescapedSpaces(line: []const u8): []const u8 {
    let end = line.len;
    while (end > 0 && line[end - 1] == ' ') {
        let bs_count: usize = 0;
        let idx = end - 1;
        while (idx > 0 && line[idx - 1] == '\\') {
            bs_count += 1;
            idx -= 1;
        }
        if ((bs_count % 2) == 1) {
            break;
        }
        end -= 1;
    }
    return line[0..end];
}

fn matchCharClass(pat: []const u8, start_idx: usize, ch: u8): usize? {
    // start_idx points just after '['
    let i = start_idx;
    if (i >= pat.len) return null;

    let negate = false;
    if (pat[i] == '!' || pat[i] == '^') {
        negate = true;
        i += 1;
    }

    let matched = false;
    let first = true;
    while (i < pat.len) {
        if (pat[i] == ']' && !first) {
            if (negate) {
                return if (!matched) i + 1 else null;
            }
            return if (matched) i + 1 else null;
        }
        first = false;

        let c1 = pat[i];
        if (c1 == '\\' && i + 1 < pat.len) {
            i += 1;
            c1 = pat[i];
        }

        if (i + 2 < pat.len && pat[i + 1] == '-' && pat[i + 2] != ']') {
            let c2 = pat[i + 2];
            let step: usize = 3;
            if (c2 == '\\' && i + 3 < pat.len) {
                c2 = pat[i + 3];
                step = 4;
            }
            if (c1 <= c2 && ch >= c1 && ch <= c2) {
                matched = true;
            }
            i += step;
        } else {
            if (ch == c1) {
                matched = true;
            }
            i += 1;
        }
    }
    // Unclosed '[' does not match
    return null;
}

fn findClosingBracket(pat: []const u8, start_idx: usize): usize? {
    let i = start_idx;
    if (i < pat.len && (pat[i] == '!' || pat[i] == '^')) {
        i += 1;
    }
    let first = true;
    while (i < pat.len) {
        if (pat[i] == ']' && !first) {
            return i + 1;
        }
        first = false;
        if (pat[i] == '\\' && i + 1 < pat.len) {
            i += 2;
        } else {
            i += 1;
        }
    }
    return null;
}

fn matchSegment(pat: []const u8, name: []const u8): bool {
    let pi: usize = 0;
    let ni: usize = 0;
    let star_pi: usize? = null;
    let star_ni: usize = 0;

    while (ni < name.len) {
        if (pi < pat.len) {
            const pc = pat[pi];
            if (pc == '*') {
                star_pi = pi;
                star_ni = ni;
                pi += 1;
                continue;
            }
            if (pc == '?') {
                pi += 1;
                ni += 1;
                continue;
            }
            if (pc == '[') {
                if (next_pi := matchCharClass(pat, pi + 1, name[ni])) {
                    pi = next_pi;
                    ni += 1;
                    continue;
                }
                // Check if bracket was valid or unclosed; only backtrack if valid class missed
                if (findClosingBracket(pat, pi + 1) == null) {
                    return false;
                }
                if (sp := star_pi) {
                    star_ni += 1;
                    ni = star_ni;
                    pi = sp + 1;
                    continue;
                }
                return false;
            }
            if (pc == '\\' && pi + 1 < pat.len) {
                if (pat[pi + 1] == name[ni]) {
                    pi += 2;
                    ni += 1;
                    continue;
                }
            } else if (pc == name[ni]) {
                pi += 1;
                ni += 1;
                continue;
            }
        }

        if (sp := star_pi) {
            star_ni += 1;
            ni = star_ni;
            pi = sp + 1;
        } else {
            return false;
        }
    }

    while (pi < pat.len && pat[pi] == '*') {
        pi += 1;
    }
    return pi == pat.len;
}

fn matchAnchoredSegments(p_segs: []const []const u8, t_segs: []const []const u8): bool {
    let pi: usize = 0;
    let ti: usize = 0;
    let star_pi: usize? = null;
    let star_ti: usize = 0;

    while (ti < t_segs.len) {
        if (pi < p_segs.len) {
            const ps = p_segs[pi];
            if (std.mem.eql(u8, ps, "**")) {
                if (pi + 1 == p_segs.len) {
                    // Trailing `/**` (or bare `**`) matches 1 or more remaining segments
                    return true;
                }
                star_pi = pi;
                star_ti = ti;
                pi += 1;
                continue;
            }
            if (matchSegment(ps, t_segs[ti])) {
                pi += 1;
                ti += 1;
                continue;
            }
        }

        if (sp := star_pi) {
            star_ti += 1;
            ti = star_ti;
            pi = sp + 1;
        } else {
            return false;
        }
    }

    while (pi < p_segs.len && std.mem.eql(u8, p_segs[pi], "**")) {
        // Trailing `/**` after all target segments already consumed does NOT match `dir` itself
        // unless the entire pattern was bare `**` (already returned above if `t_segs.len > 0`).
        if (pi + 1 == p_segs.len && pi > 0) {
            return false;
        }
        pi += 1;
    }
    return pi == p_segs.len;
}

fn matchRule(rule: IgnoreRule, t_segs: []const []const u8, is_dir: bool): bool {
    if (t_segs.len == 0) return false;
    if (rule.dir_only && !is_dir) return false;

    if (!rule.anchored) {
        return matchSegment(rule.pattern_buf, t_segs[t_segs.len - 1]);
    }

    let p_segs_buf: [128][]const u8 = undefined;
    let p_count: usize = 0;
    let iter = std.mem.splitScalar(u8, rule.pattern_buf, '/');
    while (seg := iter.next()) {
        if (seg.len == 0) continue;
        // Collapse consecutive "**" segments for clean trailing `/**` semantics
        if (p_count > 0 && std.mem.eql(u8, seg, "**") && std.mem.eql(u8, p_segs_buf[p_count - 1], "**")) {
            continue;
        }
        if (p_count >= p_segs_buf.len) return false;
        p_segs_buf[p_count] = seg;
        p_count += 1;
    }
    if (p_count == 0) return false;
    return matchAnchoredSegments(p_segs_buf[0..p_count], t_segs);
}

/// Pure CP `.gitignore` rule engine with `#` comments, `!` negation, trailing `/` directory filter,
/// leading/internal `/` anchoring, `**` multi-directory globs, and last-match-wins evaluation.
pub struct GitIgnore {
    alloc: std.mem.Allocator,
    rules: std.ArrayList(IgnoreRule) = .empty,

    /// Creates an empty `GitIgnore` rule set backed by `alloc`.
    pub static fn init(alloc: std.mem.Allocator): GitIgnore {
        return GitIgnore{
            alloc: alloc,
            rules: .empty,
        };
    }

    /// Frees all compiled pattern buffers and rule storage.
    pub fn deinit(*_) {
        for (i of 0.._.rules.items.len) {
            _.alloc.free(_.rules.items[i].pattern_buf);
        }
        _.rules.deinit(_.alloc);
    }

    /// Returns the number of compiled rules currently loaded.
    pub fn ruleCount(): usize {
        return _.rules.items.len;
    }

    /// Parses and appends all rules from a `.gitignore` file text (supports LF and CRLF).
    pub fn addPatterns(*_, text: string): void ! GitError {
        let iter = std.mem.splitScalar(u8, text, '\n');
        while (raw_line := iter.next()) {
            let line = raw_line;
            if (line.len > 0 && line[line.len - 1] == '\r') {
                line = line[0 .. line.len - 1];
            }
            _.addSingleLine(line)?;
        }
    }

    fn addSingleLine(*_, raw_line: string): void ! GitError {
        let line = stripTrailingUnescapedSpaces(raw_line);
        if (line.len > 0 && line[0] != '#') {
            let negated = false;
            if (line[0] == '!') {
                negated = true;
                line = line[1..];
            }
            if (line.len > 0) {
                let dir_only = false;
                if (line[line.len - 1] == '/') {
                    dir_only = true;
                    line = line[0 .. line.len - 1];
                }
                if (line.len > 0) {
                    let anchored = false;
                    if (line[0] == '/') {
                        anchored = true;
                        line = line[1..];
                    } else if (std.mem.indexOfScalar(u8, line, '/') != null) {
                        anchored = true;
                    }
                    if (line.len > 0) {
                        const owned_pat = _.alloc.dupe(u8, line) catch return GitError.OutOfMemory;
                        _.rules.append(_.alloc, IgnoreRule{
                            pattern_buf: owned_pat,
                            negated: negated,
                            dir_only: dir_only,
                            anchored: anchored,
                        }) catch {
                            _.alloc.free(owned_pat);
                            return GitError.OutOfMemory;
                        };
                    }
                }
            }
        }
    }

    /// Evaluates whether `rel_path` (relative to repo root, using `/` separators) is ignored.
    /// Checks ancestor directory prefixes in order, honoring last-match-wins and `!` negation.
    pub fn isIgnored(rel_path: string, is_dir: bool): bool {
        if (_.rules.items.len == 0) return false;

        let clean = rel_path;
        while (std.mem.startsWith(u8, clean, "./")) {
            clean = clean[2..];
        }
        while (clean.len > 0 && clean[0] == '/') {
            clean = clean[1..];
        }
        while (clean.len > 0 && clean[clean.len - 1] == '/') {
            clean = clean[0 .. clean.len - 1];
        }
        if (clean.len == 0 || std.mem.eql(u8, clean, ".")) return false;

        let t_segs_buf: [128][]const u8 = undefined;
        let t_count: usize = 0;
        let iter = std.mem.splitScalar(u8, clean, '/');
        while (seg := iter.next()) {
            if (seg.len == 0 || std.mem.eql(u8, seg, ".")) continue;
            if (t_count >= t_segs_buf.len) return false;
            t_segs_buf[t_count] = seg;
            t_count += 1;
        }
        if (t_count == 0) return false;

        // Check each prefix (ancestor directories first with prefix_is_dir = true,
        // then full path with caller-supplied is_dir).
        let depth: usize = 1;
        while (depth <= t_count) : (depth += 1) {
            const prefix_segs = t_segs_buf[0..depth];
            const prefix_is_dir = if (depth < t_count) true else is_dir;

            let ignored = false;
            for (r of 0.._.rules.items.len) {
                const rule = _.rules.items[r];
                if (matchRule(rule, prefix_segs, prefix_is_dir)) {
                    ignored = !rule.negated;
                }
            }
            if (ignored) {
                return true;
            }
        }
        return false;
    }
}
