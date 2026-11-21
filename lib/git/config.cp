// Source: Absorbed from /data/dev/bun/src/install/repository.cp | Synced: 2026-11-21T13:52:22Z

/// Git Config / INI 单条配置项（零拷贝切片）
pub struct GitConfigEntry {
    section: []const u8 = "",
    subsection: ([]const u8)? = null,
    key: []const u8 = "",
    value: []const u8 = "",
}

fn trimAscii(s: []const u8): []const u8 {
    return std.mem.trim(u8, s, " \t\r\n");
}

fn stripCommentOutsideQuotes(line: []const u8): []const u8 {
    let in_quotes = false;
    let escaped = false;
    let i: usize = 0;
    while (i < line.len) {
        const c = line[i];
        if (escaped) {
            escaped = false;
        } else if (c == '\\' && in_quotes) {
            escaped = true;
        } else if (c == '"') {
            in_quotes = !in_quotes;
        } else if (!in_quotes && (c == '#' || c == ';')) {
            return line[0..i];
        }
        i += 1;
    }
    return line;
}

fn stripOuterQuotes(s: []const u8): []const u8 {
    if (s.len >= 2 && s[0] == '"' && s[s.len - 1] == '"') {
        return s[1 .. s.len - 1];
    }
    return s;
}

fn subsectionEql(a: ([]const u8)?, b: ([]const u8)?): bool {
    if (va := a) {
        if (vb := b) {
            return std.mem.eql(u8, va, vb);
        }
        return false;
    }
    return b == null;
}

fn isValidKey(s: []const u8): bool {
    if (s.len == 0) { return false; }
    for (c of s) {
        if (c == ' ' || c == '\t' || c == '[' || c == ']') {
            return false;
        }
    }
    return true;
}

/// Git Config / INI 零堆分配配置解析器（固定容量 128 条目）
pub struct GitConfig {
    entries: [128]GitConfigEntry = [
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
        GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{}, GitConfigEntry{},
    ],
    len: usize = 0,

    /// 解析 Git Config / INI 文本（支持 `[section]`、`[section "subsection"]`、注释、引号值与隐式布尔键）
    static fn parse(text: []const u8): GitConfig {
        let cfg = GitConfig{};
        let cur_sec: []const u8 = "";
        let cur_sub: ([]const u8)? = null;

        let pos: usize = 0;
        while (pos < text.len) {
            const line_start = pos;
            while (pos < text.len && text[pos] != '\n') {
                pos += 1;
            }
            const raw_line = text[line_start..pos];
            if (pos < text.len && text[pos] == '\n') {
                pos += 1;
            }

            const cleaned = trimAscii(stripCommentOutsideQuotes(raw_line));
            if (cleaned.len == 0) {
                continue;
            }

            // Section header: `[core]` or `[remote "origin"]` or `[url "https://github.com/"]`
            if (cleaned[0] == '[') {
                if (close := std.mem.lastIndexOfScalar(u8, cleaned, ']')) {
                    const inner = trimAscii(cleaned[1..close]);
                    if (inner.len == 0) {
                        cur_sec = "";
                        cur_sub = null;
                        continue;
                    }
                    if (q1 := std.mem.indexOfScalar(u8, inner, '"')) {
                        if (q2 := std.mem.lastIndexOfScalar(u8, inner, '"')) {
                            if (q2 > q1) {
                                cur_sec = trimAscii(inner[0..q1]);
                                cur_sub = inner[q1 + 1 .. q2];
                                continue;
                            }
                        }
                        // Unclosed quote in section header -> ignore
                        cur_sec = "";
                        cur_sub = null;
                        continue;
                    }
                    // Deprecated `[section.subsection]` form
                    if (dot := std.mem.indexOfScalar(u8, inner, '.')) {
                        cur_sec = trimAscii(inner[0..dot]);
                        cur_sub = trimAscii(inner[dot + 1 ..]);
                    } else {
                        cur_sec = inner;
                        cur_sub = null;
                    }
                }
                continue;
            }

            if (cur_sec.len == 0 || cfg.len >= 128) {
                continue;
            }

            if (eq := std.mem.indexOfScalar(u8, cleaned, '=')) {
                const k = trimAscii(cleaned[0..eq]);
                const v = stripOuterQuotes(trimAscii(cleaned[eq + 1 ..]));
                if (isValidKey(k)) {
                    cfg.entries[cfg.len] = GitConfigEntry{
                        section: cur_sec,
                        subsection: cur_sub,
                        key: k,
                        value: v,
                    };
                    cfg.len += 1;
                }
            } else if (isValidKey(cleaned)) {
                // 无 `=` 的隐式布尔键（如 `bare` -> `"true"`）
                cfg.entries[cfg.len] = GitConfigEntry{
                    section: cur_sec,
                    subsection: cur_sub,
                    key: cleaned,
                    value: "true",
                };
                cfg.len += 1;
            }
        }

        return cfg;
    }

    /// 查询配置项（section/key 大小写无关，subsection 大小写敏感，遵循后定义覆盖前定义规则）
    fn get(section: []const u8, subsection: ([]const u8)?, key: []const u8): ([]const u8)? {
        let i = _.len;
        while (i > 0) {
            i -= 1;
            const e = _.entries[i];
            if (std.ascii.eqlIgnoreCase(e.section, section)
                && subsectionEql(e.subsection, subsection)
                && std.ascii.eqlIgnoreCase(e.key, key))
            {
                return e.value;
            }
        }
        return null;
    }

    /// 解析布尔配置值（`true`/`yes`/`on`/`1` -> `true`；`false`/`no`/`off`/`0`/空串 -> `false`）
    fn getBool(section: []const u8, subsection: ([]const u8)?, key: []const u8): bool? {
        const raw = _.get(section, subsection, key) ?? return null;
        if (raw.len == 0
            || std.ascii.eqlIgnoreCase(raw, "false")
            || std.ascii.eqlIgnoreCase(raw, "no")
            || std.ascii.eqlIgnoreCase(raw, "off")
            || std.mem.eql(u8, raw, "0"))
        {
            return false;
        }
        if (std.ascii.eqlIgnoreCase(raw, "true")
            || std.ascii.eqlIgnoreCase(raw, "yes")
            || std.ascii.eqlIgnoreCase(raw, "on")
            || std.mem.eql(u8, raw, "1"))
        {
            return true;
        }
        return null;
    }

    /// 检测是否配置了 `core.askpass`
    fn hasCoreAskpass(): bool {
        return _.get("core", null, "askpass") != null;
    }

    /// 检测是否配置了 `core.sshCommand`
    fn hasCoreSshCommand(): bool {
        return _.get("core", null, "sshCommand") != null;
    }

    /// 按 `url.<base>.insteadOf` 规则重写 URL（采用最长前缀匹配；无匹配时原样输出；缓冲区不足返回 `null`）
    fn rewriteUrl(url: []const u8, out: []u8): ([]const u8)? {
        let best_len: usize = 0;
        let best_base: ([]const u8)? = null;

        let i: usize = 0;
        while (i < _.len) {
            const e = _.entries[i];
            if (std.ascii.eqlIgnoreCase(e.section, "url") && std.ascii.eqlIgnoreCase(e.key, "insteadOf")) {
                if (base := e.subsection) {
                    const pref = e.value;
                    if (pref.len > best_len && url.len >= pref.len && std.mem.eql(u8, url[0..pref.len], pref)) {
                        best_len = pref.len;
                        best_base = base;
                    }
                }
            }
            i += 1;
        }

        if (base := best_base) {
            const suffix = url[best_len..];
            const total = base.len + suffix.len;
            if (out.len < total) { return null; }
            @memcpy(out[0..base.len], base);
            @memcpy(out[base.len..total], suffix);
            return out[0..total];
        }

        if (out.len < url.len) { return null; }
        @memcpy(out[0..url.len], url);
        return out[0..url.len];
    }
}
