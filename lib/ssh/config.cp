// Source: Absorbed from /data/dev/bun/src/ini.zig (OpenSSH ssh_config Parser & Host Resolver)

/// OpenSSH 主机生效配置结构体（零拷贝切片，遵循首现优先生效规则）
pub struct SshHostConfig {
    hostname: ([]const u8)? = null,
    user: ([]const u8)? = null,
    port: u16? = null,
    identity_file: ([]const u8)? = null,
    proxy_jump: ([]const u8)? = null,
    forward_agent: bool? = null,
    strict_host_key_checking: ([]const u8)? = null,
}

fn isHorizSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimHoriz(s: []const u8): []const u8 {
    let start: usize = 0;
    let end = s.len;
    while (start < end && isHorizSpace(s[start])) { start += 1; }
    while (end > start && isHorizSpace(s[end - 1])) { end -= 1; }
    return s[start..end];
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') { return c + 32; }
    return c;
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) { return false; }
        i += 1;
    }
    return true;
}

fn matchWildcardIgnoreCase(pat: []const u8, text: []const u8): bool {
    let pi: usize = 0;
    let si: usize = 0;
    let star_pi: usize? = null;
    let star_si: usize = 0;

    while (si < text.len || pi < pat.len) {
        if (pi < pat.len && pat[pi] == '*') {
            star_pi = pi;
            star_si = si;
            pi += 1;
            continue;
        }
        if (pi < pat.len && si < text.len &&
            (pat[pi] == '?' || toLowerAscii(pat[pi]) == toLowerAscii(text[si])))
        {
            pi += 1;
            si += 1;
            continue;
        }
        if (sp := star_pi) {
            star_si += 1;
            if (star_si > text.len) { return false; }
            si = star_si;
            pi = sp + 1;
        } else {
            return false;
        }
    }
    return true;
}

/// 遵循 `ssh_config(5)` 匹配空格分隔的多 Host 模式（支持 `*`/`?` 通配，前导 `!` 否定模式一票否决）
pub fn matchHostPatterns(patterns: []const u8, host_alias: []const u8): bool {
    if (host_alias.len == 0) { return false; }

    let matched_pos = false;
    let i: usize = 0;
    while (i < patterns.len) {
        while (i < patterns.len && isHorizSpace(patterns[i])) { i += 1; }
        if (i >= patterns.len || patterns[i] == '#') { break; }

        const start = i;
        while (i < patterns.len && !isHorizSpace(patterns[i]) && patterns[i] != '#') {
            i += 1;
        }
        let tok = patterns[start..i];
        if (tok.len >= 2 && tok[0] == '"' && tok[tok.len - 1] == '"') {
            tok = tok[1..(tok.len - 1)];
        }
        if (tok.len == 0) { continue; }

        if (tok[0] == '!') {
            const neg_pat = tok[1..tok.len];
            if (neg_pat.len > 0 && matchWildcardIgnoreCase(neg_pat, host_alias)) {
                return false; // 否定模式命中，一票否决整个 Host 块
            }
        } else {
            if (matchWildcardIgnoreCase(tok, host_alias)) {
                matched_pos = true;
            }
        }
    }

    return matched_pos;
}

struct SshLine {
    key: []const u8,
    value: []const u8,
}

fn parseSshLine(raw_line: []const u8): SshLine? {
    const line = trimHoriz(raw_line);
    if (line.len == 0 || line[0] == '#') {
        return null;
    }

    // 读取关键字直到空白或 '='
    let k_end: usize = 0;
    while (k_end < line.len && !isHorizSpace(line[k_end]) && line[k_end] != '=') {
        k_end += 1;
    }
    if (k_end == 0) { return null; }
    const key = line[0..k_end];

    let v_start = k_end;
    while (v_start < line.len && isHorizSpace(line[v_start])) { v_start += 1; }
    if (v_start < line.len && line[v_start] == '=') {
        v_start += 1;
        while (v_start < line.len && isHorizSpace(line[v_start])) { v_start += 1; }
    }
    if (v_start >= line.len) { return null; }

    let val = trimHoriz(line[v_start..line.len]);
    if (val.len >= 2 && val[0] == '"' && val[val.len - 1] == '"') {
        val = val[1..(val.len - 1)];
    }

    return SshLine{ key: key, value: val };
}

fn parsePortU16(s: []const u8): u16? {
    if (s.len == 0 || s.len > 5) { return null; }
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') { return null; }
        acc = acc * 10 + @as(u32, c - '0');
        i += 1;
    }
    if (acc == 0 || acc > 65535) { return null; }
    return @as(u16, @intCast(acc));
}

fn parseYesNo(s: []const u8): bool? {
    if (eqlIgnoreCaseAscii(s, "yes") || eqlIgnoreCaseAscii(s, "true")) { return true; }
    if (eqlIgnoreCaseAscii(s, "no") || eqlIgnoreCaseAscii(s, "false")) { return false; }
    return null;
}

fn stripBom(s: []const u8): []const u8 {
    if (s.len >= 3 && s[0] == 0xEF && s[1] == 0xBB && s[2] == 0xBF) {
        return s[3..s.len];
    }
    return s;
}

/// 解析 `~/.ssh/config` 并按 OpenSSH 首现优先生效（first-obtained-value-wins）规则计算目标主机的生效配置
pub fn resolveSshConfig(content: []const u8, host_alias: []const u8): SshHostConfig {
    let cfg = SshHostConfig{};
    const src = stripBom(content);
    let active = true; // 首个 Host/Match 块之前的全局选项生效

    let pos: usize = 0;
    while (pos < src.len) {
        const line_start = pos;
        while (pos < src.len && src[pos] != '\n') { pos += 1; }
        let line_end = pos;
        if (line_end > line_start && src[line_end - 1] == '\r') { line_end -= 1; }
        if (pos < src.len && src[pos] == '\n') { pos += 1; }

        if (entry := parseSshLine(src[line_start..line_end])) {
            if (eqlIgnoreCaseAscii(entry.key, "host")) {
                active = matchHostPatterns(entry.value, host_alias);
                continue;
            }
            if (eqlIgnoreCaseAscii(entry.key, "match")) {
                active = eqlIgnoreCaseAscii(entry.value, "all");
                continue;
            }
            if (!active) { continue; }

            if (eqlIgnoreCaseAscii(entry.key, "hostname")) {
                if (cfg.hostname == null) { cfg.hostname = entry.value; }
            } else if (eqlIgnoreCaseAscii(entry.key, "user")) {
                if (cfg.user == null) { cfg.user = entry.value; }
            } else if (eqlIgnoreCaseAscii(entry.key, "port")) {
                if (cfg.port == null) {
                    if (p := parsePortU16(entry.value)) {
                        cfg.port = p;
                    }
                }
            } else if (eqlIgnoreCaseAscii(entry.key, "identityfile")) {
                if (cfg.identity_file == null) { cfg.identity_file = entry.value; }
            } else if (eqlIgnoreCaseAscii(entry.key, "proxyjump")) {
                if (cfg.proxy_jump == null) { cfg.proxy_jump = entry.value; }
            } else if (eqlIgnoreCaseAscii(entry.key, "forwardagent")) {
                if (cfg.forward_agent == null) {
                    if (b := parseYesNo(entry.value)) {
                        cfg.forward_agent = b;
                    }
                }
            } else if (eqlIgnoreCaseAscii(entry.key, "stricthostkeychecking")) {
                if (cfg.strict_host_key_checking == null) { cfg.strict_host_key_checking = entry.value; }
            }
        }
    }

    return cfg;
}

/// 查询 `~/.ssh/config` 中对 `host_alias` 首次生效的任意配置项值（关键字大小写不敏感）
pub fn getSshOption(content: []const u8, host_alias: []const u8, key: []const u8): ([]const u8)? {
    if (key.len == 0) { return null; }
    const src = stripBom(content);
    let active = true;

    let pos: usize = 0;
    while (pos < src.len) {
        const line_start = pos;
        while (pos < src.len && src[pos] != '\n') { pos += 1; }
        let line_end = pos;
        if (line_end > line_start && src[line_end - 1] == '\r') { line_end -= 1; }
        if (pos < src.len && src[pos] == '\n') { pos += 1; }

        if (entry := parseSshLine(src[line_start..line_end])) {
            if (eqlIgnoreCaseAscii(entry.key, "host")) {
                active = matchHostPatterns(entry.value, host_alias);
                continue;
            }
            if (eqlIgnoreCaseAscii(entry.key, "match")) {
                active = eqlIgnoreCaseAscii(entry.value, "all");
                continue;
            }
            if (!active) { continue; }
            if (eqlIgnoreCaseAscii(entry.key, key)) {
                return entry.value;
            }
        }
    }

    return null;
}
