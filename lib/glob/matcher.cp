// Source: Absorbed from /data/dev/bun/src/glob/matcher.cp | Synced: 2026-11-22T12:49:17Z
// lib/glob/matcher.cp
// High-performance Unix Glob matcher in pure CP

const std = @import("std");

pub struct GlobMatchOptions {
    case_sensitive: bool = true,
    dot: bool = true,
    normalize: bool = true,
    windows_paths: bool = false,
}

pub const Matcher = struct {
    pub static fn hasMagic(pattern: []const u8): bool {
        let i: usize = 0;
        while (i < pattern.len) {
            const c = pattern[i];
            if (c == '\\') {
                i += 2;
                continue;
            }
            if (i == 0 && c == '!') {
                return true;
            }
            if (c == '*' || c == '?' || c == '[' || c == '{') {
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub static fn normalizePath(path: []const u8, out_buf: []u8, windows_paths: bool): []const u8 {
        if (path.len == 0 || out_buf.len == 0) {
            return "";
        }

        let tmp: [4096]u8 = undefined;
        let n_len: usize = 0;
        let i: usize = 0;
        while (i < path.len && n_len < tmp.len) {
            let ch = path[i];
            if (windows_paths && ch == '\\') {
                ch = '/';
            }
            tmp[n_len] = ch;
            n_len += 1;
            i += 1;
        }
        const raw = tmp[0..n_len];
        const is_abs = raw.len > 0 && raw[0] == '/';

        let segs: [256][]const u8 = undefined;
        let seg_count: usize = 0;
        let iter = std.mem.splitScalar(u8, raw, '/');
        while (seg := iter.next()) {
            if (seg.len == 0 || std.mem.eql(u8, seg, ".")) {
                continue;
            }
            if (std.mem.eql(u8, seg, "..")) {
                if (seg_count > 0 && !std.mem.eql(u8, segs[seg_count - 1], "..")) {
                    seg_count -= 1;
                } else if (!is_abs && seg_count < 256) {
                    segs[seg_count] = "..";
                    seg_count += 1;
                }
            } else if (seg_count < 256) {
                segs[seg_count] = seg;
                seg_count += 1;
            }
        }

        let pos: usize = 0;
        if (is_abs) {
            out_buf[pos] = '/';
            pos += 1;
        }
        let s: usize = 0;
        while (s < seg_count) {
            if (s > 0 && pos < out_buf.len) {
                out_buf[pos] = '/';
                pos += 1;
            }
            const seg = segs[s];
            let k: usize = 0;
            while (k < seg.len && pos < out_buf.len) {
                out_buf[pos] = seg[k];
                pos += 1;
                k += 1;
            }
            s += 1;
        }

        if (pos == 0 && std.mem.eql(u8, raw, ".")) {
            out_buf[0] = '.';
            return out_buf[0..1];
        }
        return out_buf[0..pos];
    }

    static fn normalizePattern(pattern: []const u8, out_buf: []u8): []const u8 {
        if (pattern.len == 0 || out_buf.len == 0) {
            return "";
        }
        let p = pattern;
        // Strip leading "./" sequences
        while (p.len >= 2 && p[0] == '.' && p[1] == '/') {
            p = p[2..];
        }
        if (std.mem.eql(u8, p, ".")) {
            out_buf[0] = '.';
            return out_buf[0..1];
        }

        let pos: usize = 0;
        let i: usize = 0;
        let prev_slash = false;
        while (i < p.len && pos < out_buf.len) {
            const c = p[i];
            if (c == '\\' && i + 1 < p.len) {
                if (pos + 2 > out_buf.len) break;
                out_buf[pos] = c;
                out_buf[pos + 1] = p[i + 1];
                pos += 2;
                i += 2;
                prev_slash = false;
                continue;
            }
            if (c == '/') {
                if (!prev_slash) {
                    out_buf[pos] = '/';
                    pos += 1;
                    prev_slash = true;
                }
                i += 1;
                // Skip "/./" inside pattern
                while (i + 1 < p.len && p[i] == '.' && p[i + 1] == '/') {
                    i += 2;
                }
                continue;
            }
            out_buf[pos] = c;
            pos += 1;
            prev_slash = false;
            i += 1;
        }
        // Trim trailing '/' unless pattern is "/"
        while (pos > 1 && out_buf[pos - 1] == '/') {
            pos -= 1;
        }
        return out_buf[0..pos];
    }

    pub static fn match(pattern: []const u8, path: []const u8): bool {
        return Matcher.matchWithOptions(pattern, path, GlobMatchOptions{});
    }

    pub static fn matchWithOptions(pattern: []const u8, path: []const u8, options: GlobMatchOptions): bool {
        let pat = pattern;
        let negated = false;
        while (pat.len > 0 && pat[0] == '!') {
            negated = !negated;
            pat = pat[1..];
        }

        let norm_path_buf: [4096]u8 = undefined;
        let target_path = path;
        if (options.normalize) {
            target_path = Matcher.normalizePath(path, &norm_path_buf, options.windows_paths);
        }

        let norm_pat_buf: [4096]u8 = undefined;
        let target_pat = pat;
        if (options.normalize) {
            target_pat = Matcher.normalizePattern(pat, &norm_pat_buf);
        }

        const res = Matcher.matchNoNegation(target_pat, target_path, options);
        return if (negated) !res else res;
    }

    static fn matchNoNegation(pat: []const u8, path: []const u8, options: GlobMatchOptions): bool {
        if (hasBraces(pat)) {
            return expandBracesAndMatch(pat, path, options);
        }

        const pat_abs = pat.len > 0 && pat[0] == '/';
        const path_abs = path.len > 0 && path[0] == '/';
        if (pat_abs != path_abs) {
            return false;
        }

        let pat_segs: [256][]const u8 = undefined;
        let pat_count: usize = 0;
        let iter_pat = std.mem.splitScalar(u8, pat, '/');
        while (s := iter_pat.next()) {
            if (s.len == 0) continue;
            // Collapse consecutive "**" segments to prevent exponential backtracking
            if (std.mem.eql(u8, s, "**") && pat_count > 0 && std.mem.eql(u8, pat_segs[pat_count - 1], "**")) {
                continue;
            }
            if (pat_count < 256) {
                pat_segs[pat_count] = s;
                pat_count += 1;
            }
        }

        let path_segs: [256][]const u8 = undefined;
        let path_count: usize = 0;
        let iter_path = std.mem.splitScalar(u8, path, '/');
        while (s := iter_path.next()) {
            if (s.len == 0) continue;
            if (path_count < 256) {
                path_segs[path_count] = s;
                path_count += 1;
            }
        }

        return matchSegments(pat_segs[0..pat_count], path_segs[0..path_count], options);
    }

    static fn hasBraces(pattern: []const u8): bool {
        let in_bracket = false;
        let i: usize = 0;
        let depth: i32 = 0;
        while (i < pattern.len) {
            const c = pattern[i];
            if (c == '\\') {
                i += 2;
                continue;
            }
            if (c == '[' && !in_bracket) {
                in_bracket = true;
            } else if (c == ']' && in_bracket) {
                in_bracket = false;
            } else if (!in_bracket) {
                if (c == '{') {
                    depth += 1;
                } else if (c == '}' && depth > 0) {
                    return true;
                }
            }
            i += 1;
        }
        return false;
    }

    static fn expandBracesAndMatch(pattern: []const u8, path: []const u8, options: GlobMatchOptions): bool {
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
            if (c == '[' && !in_bracket) {
                in_bracket = true;
            } else if (c == ']' && in_bracket) {
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
                let inner_bracket = false;
                let j: usize = 0;
                while (j <= inner.len) {
                    if (j == inner.len || (inner[j] == ',' && opt_depth == 0 && !inner_bracket)) {
                        const opt = inner[opt_start..j];
                        let combined_buf: [4096]u8 = undefined;
                        const total_len = prefix.len + opt.len + suffix.len;
                        if (total_len <= combined_buf.len) {
                            @memcpy(combined_buf[0..prefix.len], prefix);
                            @memcpy(combined_buf[prefix.len .. prefix.len + opt.len], opt);
                            @memcpy(combined_buf[prefix.len + opt.len .. total_len], suffix);
                            if (Matcher.matchNoNegation(combined_buf[0..total_len], path, options)) {
                                return true;
                            }
                        }
                        opt_start = j + 1;
                    } else if (inner[j] == '\\') {
                        j += 1;
                    } else if (inner[j] == '[' && !inner_bracket) {
                        inner_bracket = true;
                    } else if (inner[j] == ']' && inner_bracket) {
                        inner_bracket = false;
                    } else if (!inner_bracket) {
                        if (inner[j] == '{') {
                            opt_depth += 1;
                        } else if (inner[j] == '}') {
                            opt_depth -= 1;
                        }
                    }
                    j += 1;
                }
                return false;
            }
        }
        return false;
    }

    static fn matchSegments(pat: []const []const u8, p: []const []const u8, options: GlobMatchOptions): bool {
        if (pat.len == 0) {
            return p.len == 0;
        }

        if (std.mem.eql(u8, pat[0], "**")) {
            let i: usize = 0;
            while (i <= p.len) {
                if (i > 0 && !options.dot && p[i - 1].len > 0 && p[i - 1][0] == '.') {
                    break;
                }
                if (matchSegments(pat[1..], p[i..], options)) {
                    return true;
                }
                i += 1;
            }
            return false;
        }

        if (p.len == 0) {
            return false;
        }

        if (matchSingleSegment(pat[0], p[0], options)) {
            return matchSegments(pat[1..], p[1..], options);
        }

        return false;
    }

    static fn asciiFold(c: u8, case_sensitive: bool): u8 {
        if (!case_sensitive && c >= 'A' && c <= 'Z') {
            return c + 32;
        }
        return c;
    }

    static fn utf8SeqLen(first_byte: u8): usize {
        if (first_byte < 0x80) return 1;
        if ((first_byte & 0xE0) == 0xC0) return 2;
        if ((first_byte & 0xF0) == 0xE0) return 3;
        if ((first_byte & 0xF8) == 0xF0) return 4;
        return 1;
    }

    static fn decodeUtf8Rune(slice: []const u8, byte_len: *usize): u32 {
        if (slice.len == 0) {
            byte_len.* = 0;
            return 0;
        }
        const len = utf8SeqLen(slice[0]);
        if (len == 1 || len > slice.len) {
            byte_len.* = 1;
            return slice[0] as u32;
        }
        byte_len.* = len;
        if (len == 2) {
            return (((slice[0] & 0x1F) as u32) << 6) | ((slice[1] & 0x3F) as u32);
        }
        if (len == 3) {
            return (((slice[0] & 0x0F) as u32) << 12) | (((slice[1] & 0x3F) as u32) << 6) | ((slice[2] & 0x3F) as u32);
        }
        return (((slice[0] & 0x07) as u32) << 18) | (((slice[1] & 0x3F) as u32) << 12) | (((slice[2] & 0x3F) as u32) << 6) | ((slice[3] & 0x3F) as u32);
    }

    static fn patternStartsWithExplicitDot(pat: []const u8): bool {
        if (pat.len == 0) return false;
        if (pat[0] == '.') return true;
        if (pat.len >= 2 && pat[0] == '\\' && pat[1] == '.') return true;
        return false;
    }

    static fn matchSingleSegment(pat: []const u8, str: []const u8, options: GlobMatchOptions): bool {
        if (!options.dot && str.len > 0 && str[0] == '.' && !patternStartsWithExplicitDot(pat)) {
            return false;
        }

        let p: usize = 0;
        let s: usize = 0;
        let star_p: usize? = null;
        let star_s: usize = 0;

        while (s < str.len) {
            if (p < pat.len && pat[p] == '\\') {
                if (p + 1 >= pat.len) {
                    return false;
                }
                const esc_c = pat[p + 1];
                let esc_len: usize = 1;
                const s_len = utf8SeqLen(str[s]);
                const p_len = utf8SeqLen(esc_c);
                let is_eq = false;
                if (p_len == 1 && s_len == 1) {
                    is_eq = asciiFold(esc_c, options.case_sensitive) == asciiFold(str[s], options.case_sensitive);
                    esc_len = 1;
                } else if (p + 1 + p_len <= pat.len && s + s_len <= str.len && p_len == s_len) {
                    is_eq = std.mem.eql(u8, pat[p + 1 .. p + 1 + p_len], str[s .. s + s_len]);
                    esc_len = p_len;
                }
                if (is_eq) {
                    p = p + 1 + esc_len;
                    s += s_len;
                } else if (sp := star_p) {
                    const step = utf8SeqLen(str[star_s]);
                    star_s += if (star_s + step <= str.len) step else 1;
                    s = star_s;
                    p = sp;
                } else {
                    return false;
                }
            } else if (p < pat.len && pat[p] == '?') {
                const step = utf8SeqLen(str[s]);
                p += 1;
                s += if (s + step <= str.len) step else 1;
            } else if (p < pat.len && pat[p] == '[') {
                let close_idx: usize? = null;
                let k = p + 1;
                if (k < pat.len && (pat[k] == '!' || pat[k] == '^')) {
                    k += 1;
                }
                // Allow ']' as the very first character inside [...]
                if (k < pat.len && pat[k] == ']') {
                    k += 1;
                }
                while (k < pat.len) {
                    if (pat[k] == ']') {
                        close_idx = k;
                        break;
                    }
                    if (pat[k] == '\\' && k + 1 < pat.len) {
                        k += 2;
                        continue;
                    }
                    k += 1;
                }
                if (c_idx := close_idx) {
                    let rune_len: usize = 1;
                    const rune = decodeUtf8Rune(str[s..], &rune_len);
                    if (matchCharClass(pat[p + 1 .. c_idx], rune, options.case_sensitive)) {
                        p = c_idx + 1;
                        s += if (rune_len > 0) rune_len else 1;
                    } else if (sp := star_p) {
                        const step = utf8SeqLen(str[star_s]);
                        star_s += if (star_s + step <= str.len) step else 1;
                        s = star_s;
                        p = sp;
                    } else {
                        return false;
                    }
                } else {
                    if (asciiFold(pat[p], options.case_sensitive) == asciiFold(str[s], options.case_sensitive)) {
                        p += 1;
                        s += 1;
                    } else if (sp := star_p) {
                        const step = utf8SeqLen(str[star_s]);
                        star_s += if (star_s + step <= str.len) step else 1;
                        s = star_s;
                        p = sp;
                    } else {
                        return false;
                    }
                }
            } else if (p < pat.len && pat[p] == '*') {
                while (p < pat.len && pat[p] == '*') {
                    p += 1;
                }
                star_p = p;
                star_s = s;
            } else {
                const p_len = if (p < pat.len) utf8SeqLen(pat[p]) else 0;
                const s_len = utf8SeqLen(str[s]);
                let matched_char = false;
                if (p < pat.len) {
                    if (p_len == 1 && s_len == 1) {
                        matched_char = asciiFold(pat[p], options.case_sensitive) == asciiFold(str[s], options.case_sensitive);
                    } else if (p + p_len <= pat.len && s + s_len <= str.len && p_len == s_len) {
                        matched_char = std.mem.eql(u8, pat[p .. p + p_len], str[s .. s + s_len]);
                    }
                }
                if (matched_char) {
                    p += p_len;
                    s += s_len;
                } else if (sp := star_p) {
                    const step = utf8SeqLen(str[star_s]);
                    star_s += if (star_s + step <= str.len) step else 1;
                    s = star_s;
                    p = sp;
                } else {
                    return false;
                }
            }
        }

        while (p < pat.len && pat[p] == '*') {
            p += 1;
        }

        return p == pat.len && s == str.len;
    }

    static fn foldRune(r: u32, case_sensitive: bool): u32 {
        if (!case_sensitive && r >= 65 && r <= 90) {
            return r + 32;
        }
        return r;
    }

    static fn readClassRune(s: []const u8, idx: *usize): u32? {
        if (idx.* >= s.len) return null;
        if (s[idx.*] == '\\') {
            if (idx.* + 1 >= s.len) return null;
            idx.* += 1;
            let len: usize = 1;
            const r = decodeUtf8Rune(s[idx.*..], &len);
            idx.* += if (len > 0) len else 1;
            return r;
        }
        let len: usize = 1;
        const r = decodeUtf8Rune(s[idx.*..], &len);
        idx.* += if (len > 0) len else 1;
        return r;
    }

    static fn matchCharClass(content: []const u8, c: u32, case_sensitive: bool): bool {
        let inverted = false;
        let s = content;
        if (s.len > 0 && (s[0] == '!' || s[0] == '^')) {
            inverted = true;
            s = s[1..];
        }

        const target = foldRune(c, case_sensitive);
        let matched = false;
        let i: usize = 0;
        while (i < s.len) {
            const first_opt = readClassRune(s, &i);
            if (first_opt == null) break;
            const start = first_opt.?;

            if (i + 1 < s.len && s[i] == '-') {
                let save_i = i;
                i += 1; // skip '-'
                const end_opt = readClassRune(s, &i);
                if (end_opt == null) {
                    i = save_i;
                    if (target == foldRune(start, case_sensitive)) {
                        matched = true;
                        break;
                    }
                } else {
                    const end_r = end_opt.?;
                    const lo = foldRune(start, case_sensitive);
                    const hi = foldRune(end_r, case_sensitive);
                    if (lo <= hi && target >= lo && target <= hi) {
                        matched = true;
                        break;
                    }
                }
            } else {
                if (target == foldRune(start, case_sensitive)) {
                    matched = true;
                    break;
                }
            }
        }

        return if (inverted) !matched else matched;
    }
};

pub const GlobMatcher = Matcher;
