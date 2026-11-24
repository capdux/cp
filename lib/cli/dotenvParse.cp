// Source: Absorbed from /data/dev/bun/src/bun_core/env.cp | Synced: 2026-11-23T21:04:41Z

/// `.env` 值的引号类型枚举
pub enum DotEnvQuote: u8 {
    unquoted = 0,
    single = 1,
    double = 2,
    backtick = 3,

    fn asStr(): string {
        return switch (_) {
            .unquoted => "unquoted",
            .single => "single",
            .double => "double",
            .backtick => "backtick",
        };
    }
}

/// 单条 `.env` 键值条目（零拷贝切片）
pub struct DotEnvEntry {
    key: []const u8 = "",
    raw_value: []const u8 = "",
    quote: DotEnvQuote = .unquoted,
}

fn isHorizSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn isVarStart(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

fn isVarContinue(c: u8): bool {
    return isVarStart(c) || (c >= '0' && c <= '9');
}

fn trimHorizSpace(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isHorizSpace(s[start])) { start += 1; }
    while (end > start && isHorizSpace(s[end - 1])) { end -= 1; }
    return s[start..end];
}

fn startsWithExport(s: []const u8): bool {
    if (s.len < 7) { return false; }
    return s[0] == 'e' && s[1] == 'x' && s[2] == 'p' && s[3] == 'o' &&
        s[4] == 'r' && s[5] == 't' && isHorizSpace(s[6]);
}

/// 零堆分配 `.env` 行级迭代器
pub struct DotEnvIterator {
    src: []const u8,
    pos: usize = 0,

    /// 初始化迭代器，自动剥离可选 UTF-8 BOM
    static fn init(src: []const u8): DotEnvIterator {
        let start_pos: usize = 0;
        if (src.len >= 3 && src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF) {
            start_pos = 3;
        }
        return DotEnvIterator{ src: src, pos: start_pos };
    }

    fn skipToNextLine(*_) {
        while (_.pos < _.src.len && _.src[_.pos] != '\n') {
            _.pos += 1;
        }
        if (_.pos < _.src.len && _.src[_.pos] == '\n') {
            _.pos += 1;
        }
    }

    /// 读取下一条合法 `.env` 条目；自动跳过空行、注释行、畸形无 `=` 行
    fn next(*_): DotEnvEntry? {
        while (_.pos < _.src.len) {
            // 跳过空行与行首空白
            while (_.pos < _.src.len && (isHorizSpace(_.src[_.pos]) || _.src[_.pos] == '\n' || _.src[_.pos] == '\r')) {
                _.pos += 1;
            }
            if (_.pos >= _.src.len) { return null; }

            // 跳过 # 注释行
            if (_.src[_.pos] == '#') {
                _.skipToNextLine();
                continue;
            }

            // 剥离可选前导 `export `
            if (startsWithExport(_.src[_.pos.._.src.len])) {
                _.pos += 6;
                while (_.pos < _.src.len && isHorizSpace(_.src[_.pos])) {
                    _.pos += 1;
                }
            }

            // 扫描键名直到 '=' 或换行
            const key_start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '=' && _.src[_.pos] != '\n' && _.src[_.pos] != '\r') {
                _.pos += 1;
            }
            if (_.pos >= _.src.len || _.src[_.pos] != '=') {
                _.skipToNextLine();
                continue;
            }
            const key = trimHorizSpace(_.src[key_start.._.pos]);
            _.pos += 1; // 跳过 '='

            if (key.len == 0) {
                _.skipToNextLine();
                continue;
            }

            // 跳过 '=' 后的水平空白
            while (_.pos < _.src.len && isHorizSpace(_.src[_.pos])) {
                _.pos += 1;
            }

            if (_.pos >= _.src.len || _.src[_.pos] == '\n' || _.src[_.pos] == '\r') {
                _.skipToNextLine();
                return DotEnvEntry{ key: key, raw_value: "", quote: .unquoted };
            }

            const ch = _.src[_.pos];
            if (ch == '\'' || ch == '"' || ch == '`') {
                const quote_kind: DotEnvQuote = if (ch == '\'') .single else if (ch == '"') .double else .backtick;
                _.pos += 1;
                const val_start = _.pos;
                while (_.pos < _.src.len) {
                    const c = _.src[_.pos];
                    if (ch == '"' && c == '\\' && _.pos + 1 < _.src.len) {
                        _.pos += 2;
                        continue;
                    }
                    if (c == ch) {
                        break;
                    }
                    _.pos += 1;
                }
                const val_end = _.pos;
                if (_.pos < _.src.len && _.src[_.pos] == ch) {
                    _.pos += 1;
                }
                _.skipToNextLine();
                return DotEnvEntry{
                    key: key,
                    raw_value: _.src[val_start..val_end],
                    quote: quote_kind,
                };
            }

            // 无引号值：扫描至行尾，仅剥离空白后的 `# comment`
            const val_start = _.pos;
            let val_end = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n' && _.src[_.pos] != '\r') {
                if (_.src[_.pos] == '#' && _.pos > val_start && isHorizSpace(_.src[_.pos - 1])) {
                    break;
                }
                _.pos += 1;
                val_end = _.pos;
            }
            _.skipToNextLine();
            return DotEnvEntry{
                key: key,
                raw_value: trimHorizSpace(_.src[val_start..val_end]),
                quote: .unquoted,
            };
        }
        return null;
    }
}

struct OutWriter {
    buf: []u8,
    pos: usize = 0,
    overflow: bool = false,

    fn writeByte(*_, c: u8) {
        if (_.pos < _.buf.len) {
            _.buf[_.pos] = c;
            _.pos += 1;
        } else {
            _.overflow = true;
        }
    }

    fn writeStr(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) {
            _.writeByte(s[i]);
            i += 1;
        }
    }

    fn finish(): ([]const u8)? {
        if (_.overflow) { return null; }
        return _.buf[0.._.pos];
    }
}

/// 解转义双引号 `.env` 值（支持 `\n`, `\r`, `\t`, `\"`, `\\`, `\$`）
pub fn unescapeDoubleQuoted(raw: []const u8, out: []u8): ([]const u8)? {
    let w = OutWriter{ buf: out };
    let i: usize = 0;
    while (i < raw.len) {
        if (raw[i] == '\\' && i + 1 < raw.len) {
            const esc = raw[i + 1];
            switch (esc) {
                'n' => { w.writeByte('\n'); i += 2; },
                'r' => { w.writeByte('\r'); i += 2; },
                't' => { w.writeByte('\t'); i += 2; },
                '"' => { w.writeByte('"'); i += 2; },
                '\\' => { w.writeByte('\\'); i += 2; },
                '$' => { w.writeByte('$'); i += 2; },
                else => {
                    w.writeByte('\\');
                    w.writeByte(esc);
                    i += 2;
                },
            }
        } else {
            w.writeByte(raw[i]);
            i += 1;
        }
    }
    return w.finish();
}

/// 环境变量查询回调类型
pub type EnvLookupFn = fn ([]const u8): ([]const u8)?;

/// 展开 `.env` 变量引用：支持 `$VAR`、`${VAR}`、`${VAR:-default}` 与 `\$` 逃逸
pub fn expandVars(input: []const u8, lookup_fn: EnvLookupFn, out: []u8): ([]const u8)? {
    let w = OutWriter{ buf: out };
    let i: usize = 0;

    while (i < input.len) {
        // \$ 逃逸输出字面量 '$'
        if (input[i] == '\\' && i + 1 < input.len && input[i + 1] == '$') {
            w.writeByte('$');
            i += 2;
            continue;
        }

        if (input[i] == '$') {
            if (i + 1 < input.len && input[i + 1] == '{') {
                // 查找闭合 '}'
                let close_idx: usize? = null;
                let k = i + 2;
                while (k < input.len) {
                    if (input[k] == '}') {
                        close_idx = k;
                        break;
                    }
                    k += 1;
                }
                if (end_brace := close_idx) {
                    const expr = input[(i + 2)..end_brace];
                    // 检查 :- 默认值语法
                    let sep_idx: usize? = null;
                    let s: usize = 0;
                    while (s + 1 < expr.len) {
                        if (expr[s] == ':' && expr[s + 1] == '-') {
                            sep_idx = s;
                            break;
                        }
                        s += 1;
                    }
                    if (sep := sep_idx) {
                        const var_name = expr[0..sep];
                        const fallback = expr[(sep + 2)..expr.len];
                        if (val := lookup_fn(var_name)) {
                            if (val.len > 0) {
                                w.writeStr(val);
                            } else {
                                w.writeStr(fallback);
                            }
                        } else {
                            w.writeStr(fallback);
                        }
                    } else {
                        if (val := lookup_fn(expr)) {
                            w.writeStr(val);
                        }
                    }
                    i = end_brace + 1;
                    continue;
                }
            } else if (i + 1 < input.len && isVarStart(input[i + 1])) {
                const name_start = i + 1;
                let name_end = name_start + 1;
                while (name_end < input.len && isVarContinue(input[name_end])) {
                    name_end += 1;
                }
                const var_name = input[name_start..name_end];
                if (val := lookup_fn(var_name)) {
                    w.writeStr(val);
                }
                i = name_end;
                continue;
            }
        }

        w.writeByte(input[i]);
        i += 1;
    }

    return w.finish();
}
