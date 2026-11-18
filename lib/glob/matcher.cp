// Source: bun/src/glob/matcher.cp
// lib/glob/matcher.cp
// High-performance Unix Glob matcher in pure CP

const std = @import("std");

pub const Matcher = struct {
    pub fn hasMagic(pattern: []const u8): bool {
        let i: usize = 0;
        while (i < pattern.len) {
            const c = pattern[i];
            if (c == '\\') {
                i += 2;
                continue;
            }
            if (c == '*' || c == '?' || c == '[' || c == '{' || c == '!') {
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn match(pattern: []const u8, path: []const u8): bool {
        let pat = pattern;
        let negated = false;
        while (pat.len > 0 && pat[0] == '!') {
            negated = !negated;
            pat = pat[1..];
        }

        // Expand brace patterns like *.{cp,zig} or {src,test}/**/*.cp
        if (hasBraces(pat)) {
            const res = expandBracesAndMatch(pat, path);
            return if (negated) !res else res;
        }

        let pat_segs: [64][]const u8 = undefined;
        let pat_count: usize = 0;
        let iter_pat = std.mem.splitScalar(u8, pat, '/');
        while (s := iter_pat.next()) {
            if (s.len == 0) continue;
            if (pat_count < 64) {
                pat_segs[pat_count] = s;
                pat_count += 1;
            }
        }

        let path_segs: [64][]const u8 = undefined;
        let path_count: usize = 0;
        let iter_path = std.mem.splitScalar(u8, path, '/');
        while (s := iter_path.next()) {
            if (s.len == 0) continue;
            if (path_count < 64) {
                path_segs[path_count] = s;
                path_count += 1;
            }
        }

        const res = matchSegments(pat_segs[0..pat_count], path_segs[0..path_count]);
        return if (negated) !res else res;
    }

    fn hasBraces(pattern: []const u8): bool {
        let in_bracket = false;
        let i: usize = 0;
        while (i < pattern.len) {
            const c = pattern[i];
            if (c == '\\') {
                i += 2;
                continue;
            }
            if (c == '[') {
                in_bracket = true;
            } else if (c == ']') {
                in_bracket = false;
            } else if (!in_bracket && c == '{') {
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn expandBracesAndMatch(pattern: []const u8, path: []const u8): bool {
        let open_idx: usize? = null;
        let close_idx: usize? = null;
        let depth: i32 = 0;
        let in_bracket = false;
        let i: usize = 0;

        while (i < pattern.len) {
            const c = pattern[i];
            if (c == '\\') {
                i += 2;
                continue;
            }
            if (c == '[') {
                in_bracket = true;
            } else if (c == ']') {
                in_bracket = false;
            } else if (!in_bracket) {
                if (c == '{') {
                    if (depth == 0) {
                        open_idx = i;
                    }
                    depth += 1;
                } else if (c == '}') {
                    depth -= 1;
                    if (depth == 0) {
                        close_idx = i;
                        break;
                    }
                }
            }
            i += 1;
        }

        if (o_idx := open_idx) {
            if (c_idx := close_idx) {
                const prefix = pattern[0..o_idx];
                const inner = pattern[o_idx + 1 .. c_idx];
                const suffix = pattern[c_idx + 1 ..];

                let opt_start: usize = 0;
                let opt_depth: i32 = 0;
                let j: usize = 0;
                while (j <= inner.len) {
                    if (j == inner.len || (inner[j] == ',' && opt_depth == 0)) {
                        const opt = inner[opt_start..j];
                        let combined_buf: [1024]u8 = undefined;
                        const total_len = prefix.len + opt.len + suffix.len;
                        if (total_len <= 1024) {
                            @memcpy(combined_buf[0..prefix.len], prefix);
                            @memcpy(combined_buf[prefix.len .. prefix.len + opt.len], opt);
                            @memcpy(combined_buf[prefix.len + opt.len .. total_len], suffix);
                            if (Matcher.match(combined_buf[0..total_len], path)) {
                                return true;
                            }
                        }
                        opt_start = j + 1;
                    } else if (inner[j] == '{') {
                        opt_depth += 1;
                    } else if (inner[j] == '}') {
                        opt_depth -= 1;
                    } else if (inner[j] == '\\') {
                        j += 1;
                    }
                    j += 1;
                }
                return false;
            }
        }
        return false;
    }

    fn matchSegments(pat: []const []const u8, p: []const []const u8): bool {
        if (pat.len == 0) {
            return p.len == 0;
        }

        if (std.mem.eql(u8, pat[0], "**")) {
            let i: usize = 0;
            while (i <= p.len) {
                if (matchSegments(pat[1..], p[i..])) {
                    return true;
                }
                i += 1;
            }
            return false;
        }

        if (p.len == 0) {
            return false;
        }

        if (matchSingleSegment(pat[0], p[0])) {
            return matchSegments(pat[1..], p[1..]);
        }

        return false;
    }

    fn matchSingleSegment(pat: []const u8, str: []const u8): bool {
        let p: usize = 0;
        let s: usize = 0;
        let star_p: usize? = null;
        let star_s: usize = 0;

        while (s < str.len) {
            if (p < pat.len && pat[p] == '\\') {
                p += 1;
                if (p < pat.len && pat[p] == str[s]) {
                    p += 1;
                    s += 1;
                } else if (sp := star_p) {
                    star_s += 1;
                    s = star_s;
                    p = sp;
                } else {
                    return false;
                }
            } else if (p < pat.len && pat[p] == '?') {
                p += 1;
                s += 1;
            } else if (p < pat.len && pat[p] == '[') {
                let close_idx: usize? = null;
                let k = p + 1;
                while (k < pat.len) {
                    if (pat[k] == ']') {
                        close_idx = k;
                        break;
                    }
                    if (pat[k] == '\\') {
                        k += 1;
                    }
                    k += 1;
                }
                if (c_idx := close_idx) {
                    if (matchCharClass(pat[p + 1 .. c_idx], str[s])) {
                        p = c_idx + 1;
                        s += 1;
                    } else if (sp := star_p) {
                        star_s += 1;
                        s = star_s;
                        p = sp;
                    } else {
                        return false;
                    }
                } else {
                    if (pat[p] == str[s]) {
                        p += 1;
                        s += 1;
                    } else return false;
                }
            } else if (p < pat.len && pat[p] == '*') {
                p += 1;
                star_p = p;
                star_s = s;
            } else if (p < pat.len && pat[p] == str[s]) {
                p += 1;
                s += 1;
            } else if (sp := star_p) {
                star_s += 1;
                s = star_s;
                p = sp;
            } else {
                return false;
            }
        }

        while (p < pat.len && pat[p] == '*') {
            p += 1;
        }

        return p == pat.len && s == str.len;
    }

    fn matchCharClass(content: []const u8, c: u8): bool {
        let inverted = false;
        let s = content;
        if (s.len > 0 && (s[0] == '!' || s[0] == '^')) {
            inverted = true;
            s = s[1..];
        }

        let matched = false;
        let i: usize = 0;
        while (i < s.len) {
            if (i + 2 < s.len && s[i + 1] == '-') {
                const start = s[i];
                const end = s[i + 2];
                if (c >= start && c <= end) {
                    matched = true;
                    break;
                }
                i += 3;
            } else {
                if (c == s[i]) {
                    matched = true;
                    break;
                }
                i += 1;
            }
        }

        return if (inverted) !matched else matched;
    }
};

pub const GlobMatcher = Matcher;
