// Source: UNVERIFIED (origin to be confirmed: Bun / Zig std / custom)
// lib/glob/matcher.cp
// High-performance Unix Glob matcher in CP

pub const Matcher = struct {
    pub fn hasMagic(pattern: []const u8): bool {
        let i: usize = 0;
        while (i < pattern.len) : (i += 1) {
            const c = pattern[i];
            if (c == '*' or c == '?' or c == '[' or c == '{' or c == '!') {
                let backslashes: usize = 0;
                let j = i;
                while (j > 0 and pattern[j - 1] == '\\') {
                    backslashes += 1;
                    j -= 1;
                }
                if (backslashes % 2 == 0) return true;
            }
        }
        return false;
    }

    pub fn match(pattern: []const u8, path: []const u8): bool {
        let pat = pattern;
        let negated = false;
        while (pat.len > 0 and pat[0] == '!') {
            negated = !negated;
            pat = pat[1..];
        }

        let pat_segs: [32][]const u8 = undefined;
        let pat_count: usize = 0;
        let iter_pat = std.mem.splitScalar(u8, pat, '/');
        while (iter_pat.next()) |s| {
            if (s.len == 0) continue;
            if (pat_count < 32) {
                pat_segs[pat_count] = s;
                pat_count += 1;
            }
        }

        let path_segs: [32][]const u8 = undefined;
        let path_count: usize = 0;
        let iter_path = std.mem.splitScalar(u8, path, '/');
        while (iter_path.next()) |s| {
            if (s.len == 0) continue;
            if (path_count < 32) {
                path_segs[path_count] = s;
                path_count += 1;
            }
        }

        const res = matchSegments(pat_segs[0..pat_count], path_segs[0..path_count]);
        return if (negated) !res else res;
    }

    fn matchSegments(pat: []const []const u8, p: []const []const u8): bool {
        if (pat.len == 0) {
            return p.len == 0;
        }

        if (std.mem.eql(u8, pat[0], "**")) {
            // ** matches 0 or more segments
            let i: usize = 0;
            while (i <= p.len) : (i += 1) {
                if (matchSegments(pat[1..], p[i..])) {
                    return true;
                }
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
            if (p < pat.len and pat[p] == '?') {
                p += 1;
                s += 1;
            } else if (p < pat.len and pat[p] == '[') {
                let close_idx: usize? = null;
                let k = p + 1;
                while (k < pat.len) : (k += 1) {
                    if (pat[k] == ']') {
                        close_idx = k;
                        break;
                    }
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
            } else if (p < pat.len and pat[p] == '*') {
                p += 1;
                star_p = p;
                star_s = s;
            } else if (p < pat.len and pat[p] == str[s]) {
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

        while (p < pat.len and pat[p] == '*') {
            p += 1;
        }

        return p == pat.len and s == str.len;
    }

    fn matchCharClass(content: []const u8, c: u8): bool {
        let inverted = false;
        let s = content;
        if (s.len > 0 and (s[0] == '!' or s[0] == '^')) {
            inverted = true;
            s = s[1..];
        }

        let matched = false;
        let i: usize = 0;
        while (i < s.len) {
            if (i + 2 < s.len and s[i + 1] == '-') {
                const start = s[i];
                const end = s[i + 2];
                if (c >= start and c <= end) {
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
