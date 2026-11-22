// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T12:49:17Z (RFC 9309 robots.txt Parser & Matcher)

fn isOws(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isOws(s[start])) { start += 1; }
    while (end > start && isOws(s[end - 1])) { end -= 1; }
    return s[start..end];
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') { ca += 32; }
        if (cb >= 'A' && cb <= 'Z') { cb += 32; }
        if (ca != cb) { return false; }
        i += 1;
    }
    return true;
}

fn startsWithIgnoreCaseAscii(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) { return false; }
    return eqlIgnoreCaseAscii(s[0..prefix.len], prefix);
}

fn stripBom(src: []const u8): []const u8 {
    if (src.len >= 3 && src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF) {
        return src[3..src.len];
    }
    return src;
}

fn stripCommentAndTrim(line: []const u8): []const u8 {
    let end = line.len;
    let i: usize = 0;
    while (i < line.len) {
        if (line[i] == '#') {
            end = i;
            break;
        }
        i += 1;
    }
    return trimOws(line[0..end]);
}

struct ParsedLine {
    field: []const u8,
    value: []const u8,
}

fn parseDirectiveLine(raw_line: []const u8): ParsedLine? {
    const clean = stripCommentAndTrim(raw_line);
    if (clean.len == 0) { return null; }

    let colon_idx: usize? = null;
    let i: usize = 0;
    while (i < clean.len) {
        if (clean[i] == ':') {
            colon_idx = i;
            break;
        }
        i += 1;
    }
    if (ci := colon_idx) {
        const field = trimOws(clean[0..ci]);
        const value = trimOws(clean[(ci + 1)..clean.len]);
        if (field.len == 0) { return null; }
        return ParsedLine{ field: field, value: value };
    }
    return null;
}

/// 匹配 RFC 9309 路径规则（支持 `*` 任意子串通配符与结尾 `$` 精确锚点）
/// 匹配成功返回规则特异度长度 `pattern.len`，否则返回 `null`
pub fn matchRobotsPattern(pattern: []const u8, path: []const u8): usize? {
    if (pattern.len == 0) {
        return null;
    }

    let anchor_end = false;
    let core = pattern;
    if (pattern[pattern.len - 1] == '$') {
        anchor_end = true;
        core = pattern[0..(pattern.len - 1)];
    }

    let pi: usize = 0;
    let si: usize = 0;
    let star_pi: usize? = null;
    let star_si: usize = 0;

    while (si < path.len || pi < core.len) {
        if (pi < core.len && core[pi] == '*') {
            star_pi = pi;
            star_si = si;
            pi += 1;
        } else if (pi < core.len && si < path.len && core[pi] == path[si]) {
            pi += 1;
            si += 1;
        } else if (pi == core.len && !anchor_end) {
            return pattern.len;
        } else if (sp := star_pi) {
            star_si += 1;
            if (star_si > path.len) {
                return null;
            }
            si = star_si;
            pi = sp + 1;
        } else {
            return null;
        }
    }

    return pattern.len;
}

fn matchUserAgentSpec(rule_ua: []const u8, target_ua: []const u8): i64 {
    if (rule_ua.len == 0) { return -1; }
    if (rule_ua.len == 1 && rule_ua[0] == '*') {
        return 0;
    }
    if (startsWithIgnoreCaseAscii(target_ua, rule_ua)) {
        return @as(i64, @intCast(rule_ua.len));
    }
    return -1;
}

fn findBestUaSpecificity(src: []const u8, user_agent: []const u8): i64 {
    let best: i64 = -1;
    let pos: usize = 0;
    while (pos < src.len) {
        const line_start = pos;
        while (pos < src.len && src[pos] != '\n') { pos += 1; }
        let line_end = pos;
        if (line_end > line_start && src[line_end - 1] == '\r') { line_end -= 1; }
        if (pos < src.len && src[pos] == '\n') { pos += 1; }

        if (d := parseDirectiveLine(src[line_start..line_end])) {
            if (eqlIgnoreCaseAscii(d.field, "user-agent")) {
                const spec = matchUserAgentSpec(d.value, user_agent);
                if (spec > best) {
                    best = spec;
                }
            }
        }
    }
    return best;
}

/// 按 RFC 9309 规则判断指定 `user_agent` 是否允许访问 `path`
/// 优先匹配最具体 `User-agent` 组（未命中回退 `*`）；组内取最长匹配规则，同长度 `Allow` 优先于 `Disallow`
pub fn isAllowed(robots_txt: []const u8, user_agent: []const u8, path: []const u8): bool {
    const src = stripBom(robots_txt);
    const best_ua = findBestUaSpecificity(src, user_agent);
    if (best_ua < 0) {
        return true;
    }

    let in_ua_header = false;
    let group_active = false;
    let has_rule = false;
    let best_rule_len: usize = 0;
    let best_allowed = true;

    let pos: usize = 0;
    while (pos < src.len) {
        const line_start = pos;
        while (pos < src.len && src[pos] != '\n') { pos += 1; }
        let line_end = pos;
        if (line_end > line_start && src[line_end - 1] == '\r') { line_end -= 1; }
        if (pos < src.len && src[pos] == '\n') { pos += 1; }

        if (d := parseDirectiveLine(src[line_start..line_end])) {
            if (eqlIgnoreCaseAscii(d.field, "user-agent")) {
                if (!in_ua_header) {
                    in_ua_header = true;
                    group_active = false;
                }
                if (matchUserAgentSpec(d.value, user_agent) == best_ua) {
                    group_active = true;
                }
            } else if (eqlIgnoreCaseAscii(d.field, "sitemap")) {
                // Sitemap 为全局指令，不影响当前组状态
                continue;
            } else {
                in_ua_header = false;
                if (!group_active) { continue; }

                if (eqlIgnoreCaseAscii(d.field, "allow")) {
                    if (d.value.len > 0) {
                        if (mlen := matchRobotsPattern(d.value, path)) {
                            if (!has_rule || mlen > best_rule_len || (mlen == best_rule_len && !best_allowed)) {
                                has_rule = true;
                                best_rule_len = mlen;
                                best_allowed = true;
                            }
                        }
                    }
                } else if (eqlIgnoreCaseAscii(d.field, "disallow")) {
                    // 空 Disallow: 表示全部允许，不记录拒绝规则
                    if (d.value.len > 0) {
                        if (mlen := matchRobotsPattern(d.value, path)) {
                            if (!has_rule || mlen > best_rule_len) {
                                has_rule = true;
                                best_rule_len = mlen;
                                best_allowed = false;
                            }
                        }
                    }
                }
            }
        }
    }

    return if (has_rule) best_allowed else true;
}

fn parseU64(s: []const u8): u64? {
    if (s.len == 0) { return null; }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') { return null; }
        const d: u64 = @as(u64, c - '0');
        if (acc > ((@as(u64, 18446744073709551615) - d) / 10)) { return null; }
        acc = acc * 10 + d;
        i += 1;
    }
    return acc;
}

/// 提取对应 `user_agent` 匹配组的 `Crawl-delay` 秒数（未设置或非法值返回 `null`）
pub fn crawlDelaySec(robots_txt: []const u8, user_agent: []const u8): u64? {
    const src = stripBom(robots_txt);
    const best_ua = findBestUaSpecificity(src, user_agent);
    if (best_ua < 0) {
        return null;
    }

    let in_ua_header = false;
    let group_active = false;

    let pos: usize = 0;
    while (pos < src.len) {
        const line_start = pos;
        while (pos < src.len && src[pos] != '\n') { pos += 1; }
        let line_end = pos;
        if (line_end > line_start && src[line_end - 1] == '\r') { line_end -= 1; }
        if (pos < src.len && src[pos] == '\n') { pos += 1; }

        if (d := parseDirectiveLine(src[line_start..line_end])) {
            if (eqlIgnoreCaseAscii(d.field, "user-agent")) {
                if (!in_ua_header) {
                    in_ua_header = true;
                    group_active = false;
                }
                if (matchUserAgentSpec(d.value, user_agent) == best_ua) {
                    group_active = true;
                }
            } else if (eqlIgnoreCaseAscii(d.field, "sitemap")) {
                continue;
            } else {
                in_ua_header = false;
                if (!group_active) { continue; }
                if (eqlIgnoreCaseAscii(d.field, "crawl-delay")) {
                    if (sec := parseU64(d.value)) {
                        return sec;
                    }
                }
            }
        }
    }

    return null;
}

/// 零堆分配 `Sitemap:` URL 迭代器
pub struct SitemapIter {
    src: []const u8,
    pos: usize = 0,

    static fn init(robots_txt: []const u8): SitemapIter {
        return SitemapIter{
            src: stripBom(robots_txt),
            pos: 0,
        };
    }

    fn next(*_): ([]const u8)? {
        while (_.pos < _.src.len) {
            const line_start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n') { _.pos += 1; }
            let line_end = _.pos;
            if (line_end > line_start && _.src[line_end - 1] == '\r') { line_end -= 1; }
            if (_.pos < _.src.len && _.src[_.pos] == '\n') { _.pos += 1; }

            if (d := parseDirectiveLine(_.src[line_start..line_end])) {
                if (eqlIgnoreCaseAscii(d.field, "sitemap") && d.value.len > 0) {
                    return d.value;
                }
            }
        }
        return null;
    }
}
