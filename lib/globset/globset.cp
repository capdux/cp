// Source: cp/lib/globset/globset.cp (Multi-Pattern GlobSet & .gitignore Engine)
// Pure CP multi-pattern glob set with nested brace expansion ({ts,tsx,{c,h}})
// and .gitignore-compatible IgnoreMatcher (comments, ! negation, /, trailing /).

const std = @import("std");
import { GlobError, GlobOptions, GlobMatcher } from "./glob.cp";

const SetEntry = struct {
    matcher: GlobMatcher,
    pattern_index: usize,
};

fn expandBraces(
    alloc: std.mem.Allocator,
    pattern: string,
    out: *std.ArrayList([]u8),
): void ! GlobError {
    if (pattern.len == 0) {
        return GlobError.EmptyPattern;
    }

    let depth: usize = 0;
    let open_idx: usize = 0;
    let close_idx: usize = 0;
    let found_first = false;

    let i: usize = 0;
    while (i < pattern.len) {
        const c = pattern[i];
        if (c == '\\') {
            if (i + 1 >= pattern.len) {
                return GlobError.InvalidPattern;
            }
            i += 2;
            continue;
        }
        if (c == '[') {
            let j = i + 1;
            if (j < pattern.len && (pattern[j] == '!' || pattern[j] == '^')) {
                j += 1;
            }
            while (j < pattern.len && pattern[j] != ']') {
                if (pattern[j] == '\\') {
                    if (j + 1 >= pattern.len) {
                        return GlobError.UnclosedBracket;
                    }
                    j += 2;
                } else {
                    j += 1;
                }
            }
            if (j >= pattern.len) {
                return GlobError.UnclosedBracket;
            }
            i = j + 1;
            continue;
        }
        if (c == '{') {
            if (depth == 0 && !found_first) {
                open_idx = i;
            }
            depth += 1;
        } else if (c == '}') {
            if (depth == 0) {
                return GlobError.UnclosedBrace;
            }
            depth -= 1;
            if (depth == 0 && !found_first) {
                close_idx = i;
                found_first = true;
            }
        }
        i += 1;
    }

    if (depth != 0) {
        return GlobError.UnclosedBrace;
    }

    if (!found_first) {
        const dup = alloc.dupe(u8, pattern) catch return GlobError.OutOfMemory;
        errdefer alloc.free(dup);
        out.append(alloc, dup) catch return GlobError.OutOfMemory;
        return;
    }

    const prefix = pattern[0..open_idx];
    const inner = pattern[(open_idx + 1)..close_idx];
    const suffix = pattern[(close_idx + 1)..pattern.len];

    // Split `inner` on top-level commas
    let seg_start: usize = 0;
    let inner_depth: usize = 0;
    let k: usize = 0;
    while (k <= inner.len) : (k += 1) {
        const at_end = k == inner.len;
        let is_comma = false;
        if (!at_end) {
            const ch = inner[k];
            if (ch == '\\') {
                k += 1;
                continue;
            } else if (ch == '[') {
                let j = k + 1;
                while (j < inner.len && inner[j] != ']') {
                    if (inner[j] == '\\' && j + 1 < inner.len) {
                        j += 2;
                    } else {
                        j += 1;
                    }
                }
                k = j;
                continue;
            } else if (ch == '{') {
                inner_depth += 1;
            } else if (ch == '}') {
                if (inner_depth > 0) {
                    inner_depth -= 1;
                }
            } else if (ch == ',' && inner_depth == 0) {
                is_comma = true;
            }
        }

        if (at_end || is_comma) {
            const alt = inner[seg_start..k];
            const total_len = prefix.len + alt.len + suffix.len;
            const combined = alloc.alloc(u8, total_len) catch return GlobError.OutOfMemory;
            @memcpy(combined[0..prefix.len], prefix);
            @memcpy(combined[prefix.len..(prefix.len + alt.len)], alt);
            @memcpy(combined[(prefix.len + alt.len)..total_len], suffix);
            defer alloc.free(combined);

            expandBraces(alloc, combined, out)?;
            seg_start = k + 1;
        }
    }
}

/// Compiled multi-pattern glob set supporting brace expansion and index reporting.
pub struct GlobSet {
    alloc: std.mem.Allocator,
    entries: []SetEntry,
    pattern_count: usize,

    /// Releases all compiled matchers in the set.
    pub fn deinit(*_) {
        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            _.entries[i].matcher.deinit();
        }
        if (_.entries.len > 0) {
            _.alloc.free(_.entries);
            _.entries = [];
        }
        _.pattern_count = 0;
    }

    /// Returns the number of patterns added to this `GlobSet`.
    pub fn len(): usize {
        return _.pattern_count;
    }

    /// Returns `true` if the `GlobSet` contains no patterns.
    pub fn isEmpty(): bool {
        return _.pattern_count == 0;
    }

    /// Returns `true` if any pattern in the set matches `path`.
    pub fn matches(path: string): bool {
        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            if (_.entries[i].matcher.matches(path)) {
                return true;
            }
        }
        return false;
    }

    /// Returns a sorted, deduplicated slice of 0-based pattern indices that match `path`.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    pub fn matchesWhich(alloc: std.mem.Allocator, path: string): []usize ! GlobError {
        let out: std.ArrayList(usize) = .empty;
        errdefer out.deinit(alloc);

        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            const idx = _.entries[i].pattern_index;
            if (out.items.len > 0 && out.items[out.items.len - 1] == idx) {
                continue;
            }
            if (_.entries[i].matcher.matches(path)) {
                out.append(alloc, idx) catch return GlobError.OutOfMemory;
            }
        }

        return out.toOwnedSlice(alloc) catch return GlobError.OutOfMemory;
    }
}

/// Builder for constructing a `GlobSet` from multiple patterns with brace expansion.
pub struct GlobSetBuilder {
    alloc: std.mem.Allocator,
    opts: GlobOptions,
    entries: std.ArrayList(SetEntry),
    next_index: usize,

    /// Creates a new `GlobSetBuilder` with default `GlobOptions`.
    pub fn init(alloc: std.mem.Allocator): GlobSetBuilder {
        return GlobSetBuilder{
            alloc: alloc,
            opts: GlobOptions{},
            entries: .empty,
            next_index: 0,
        };
    }

    /// Creates a new `GlobSetBuilder` with custom default `GlobOptions`.
    pub fn initWithOptions(alloc: std.mem.Allocator, opts: GlobOptions): GlobSetBuilder {
        return GlobSetBuilder{
            alloc: alloc,
            opts: opts,
            entries: .empty,
            next_index: 0,
        };
    }

    /// Releases any unbuilt matchers held by the builder.
    pub fn deinit(*_) {
        let i: usize = 0;
        while (i < _.entries.items.len) : (i += 1) {
            _.entries.items[i].matcher.deinit();
        }
        _.entries.deinit(_.alloc);
        _.next_index = 0;
    }

    /// Adds a pattern using the builder's default `GlobOptions`.
    pub fn add(*_, pattern: string): void ! GlobError {
        return _.addWithOptions(pattern, _.opts);
    }

    /// Adds a pattern with explicit `GlobOptions`, expanding any `{...}` braces.
    pub fn addWithOptions(*_, pattern: string, opts: GlobOptions): void ! GlobError {
        if (pattern.len == 0) {
            return GlobError.EmptyPattern;
        }

        let expanded: std.ArrayList([]u8) = .empty;
        defer {
            let j: usize = 0;
            while (j < expanded.items.len) : (j += 1) {
                _.alloc.free(expanded.items[j]);
            }
            expanded.deinit(_.alloc);
        }

        expandBraces(_.alloc, pattern, &expanded)?;

        let i: usize = 0;
        while (i < expanded.items.len) : (i += 1) {
            let m = GlobMatcher.compile(_.alloc, expanded.items[i], opts)?;
            errdefer m.deinit();
            _.entries.append(_.alloc, SetEntry{
                matcher: m,
                pattern_index: _.next_index,
            }) catch return GlobError.OutOfMemory;
        }

        _.next_index += 1;
    }

    /// Finalizes and returns the compiled `GlobSet`, transferring ownership of matchers.
    pub fn build(*_): GlobSet ! GlobError {
        const count = _.next_index;
        const owned = _.entries.toOwnedSlice(_.alloc) catch return GlobError.OutOfMemory;
        _.next_index = 0;
        return GlobSet{
            alloc: _.alloc,
            entries: owned,
            pattern_count: count,
        };
    }
}

const IgnoreRule = struct {
    matcher: GlobMatcher,
    negated: bool,
    dir_only: bool,
};

/// `.gitignore`-compatible rule engine supporting `#` comments, blank lines,
/// `!` negation (last-match-wins), trailing `/` directory-only rules, and leading `/` root anchoring.
pub struct IgnoreMatcher {
    alloc: std.mem.Allocator,
    rules: std.ArrayList(IgnoreRule),

    /// Creates an empty `IgnoreMatcher`.
    pub fn init(alloc: std.mem.Allocator): IgnoreMatcher {
        return IgnoreMatcher{
            alloc: alloc,
            rules: .empty,
        };
    }

    /// Releases all compiled ignore rules.
    pub fn deinit(*_) {
        let i: usize = 0;
        while (i < _.rules.items.len) : (i += 1) {
            _.rules.items[i].matcher.deinit();
        }
        _.rules.deinit(_.alloc);
    }

    /// Parses and appends a single `.gitignore` rule line.
    pub fn addRule(*_, raw_line: string): void ! GlobError {
        let end = raw_line.len;
        if (end > 0 && raw_line[end - 1] == '\r') {
            end -= 1;
        }

        // Strip trailing unescaped spaces
        while (end > 0 && raw_line[end - 1] == ' ') {
            let bs_count: usize = 0;
            let p = end - 1;
            while (p > 0 && raw_line[p - 1] == '\\') {
                bs_count += 1;
                p -= 1;
            }
            if ((bs_count & 1) == 1) {
                break;
            }
            end -= 1;
        }

        let s = raw_line[0..end];
        if (s.len == 0) {
            return;
        }

        // Skip whitespace-only lines
        let all_ws = true;
        let k: usize = 0;
        while (k < s.len) : (k += 1) {
            if (s[k] != ' ' && s[k] != '\t') {
                all_ws = false;
                break;
            }
        }
        if (all_ws) {
            return;
        }

        // Skip comments
        if (s[0] == '#') {
            return;
        }

        let negated = false;
        if (s[0] == '!') {
            negated = true;
            s = s[1..s.len];
            if (s.len == 0) {
                return;
            }
        }

        let dir_only = false;
        if (s.len > 0 && s[s.len - 1] == '/') {
            dir_only = true;
            s = s[0..(s.len - 1)];
            if (s.len == 0) {
                return;
            }
        }

        let anchored = false;
        if (s[0] == '/') {
            anchored = true;
            s = s[1..s.len];
            if (s.len == 0) {
                return;
            }
        } else {
            let idx: usize = 0;
            while (idx < s.len) : (idx += 1) {
                if (s[idx] == '/') {
                    anchored = true;
                    break;
                }
            }
        }

        const opts = GlobOptions{
            case_insensitive: false,
            literal_separator: true,
        };

        let matcher: GlobMatcher = undefined;
        if (anchored) {
            matcher = GlobMatcher.compile(_.alloc, s, opts)?;
        } else {
            const pref_len = 3 + s.len;
            let buf = _.alloc.alloc(u8, pref_len) catch return GlobError.OutOfMemory;
            defer _.alloc.free(buf);
            buf[0] = '*';
            buf[1] = '*';
            buf[2] = '/';
            @memcpy(buf[3..pref_len], s);
            matcher = GlobMatcher.compile(_.alloc, buf, opts)?;
        }
        errdefer matcher.deinit();

        _.rules.append(_.alloc, IgnoreRule{
            matcher: matcher,
            negated: negated,
            dir_only: dir_only,
        }) catch return GlobError.OutOfMemory;
    }

    /// Parses multi-line `.gitignore` content separated by `\n` (or `\r\n`).
    pub fn parse(*_, content: string): void ! GlobError {
        let start: usize = 0;
        let i: usize = 0;
        while (i <= content.len) : (i += 1) {
            if (i == content.len || content[i] == '\n') {
                _.addRule(content[start..i])?;
                start = i + 1;
            }
        }
    }

    /// Evaluates whether `path` (with directory flag `is_dir`) is ignored.
    pub fn isIgnored(path: string, is_dir: bool): bool {
        let p = path;
        if (p.len >= 2 && p[0] == '.' && p[1] == '/') {
            p = p[2..p.len];
        }
        while (p.len > 0 && p[0] == '/') {
            p = p[1..p.len];
        }
        while (p.len > 0 && p[p.len - 1] == '/') {
            p = p[0..(p.len - 1)];
        }

        let ignored = false;
        let i: usize = 0;
        while (i < _.rules.items.len) : (i += 1) {
            const rule = _.rules.items[i];
            if (rule.dir_only && !is_dir) {
                continue;
            }
            if (rule.matcher.matches(p)) {
                ignored = !rule.negated;
            }
        }
        return ignored;
    }
}
