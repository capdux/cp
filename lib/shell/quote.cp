// Source: Absorbed from /data/dev/bun/src/shell/shell.zig | Synced: 2026-11-22T12:49:17Z (POSIX Shell Quoting & Tokenizer)

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isSafeChar(c: u8): bool {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
        return true;
    }
    return c == '_' || c == '@' || c == '%' || c == '+' || c == '='
        || c == ':' || c == ',' || c == '.' || c == '/' || c == '-';
}

/// 判断字符串在 POSIX Shell 命令行中是否需要引号包裹
/// 空串或含任何非 `[A-Za-z0-9_@%+=:,./-]` 字符时返回 true
export fn needsQuoting(s: []const u8): bool {
    if (s.len == 0) {
        return true;
    }
    for (c of s) {
        if (!isSafeChar(c)) {
            return true;
        }
    }
    return false;
}

/// 按 POSIX Shell 单引号规则安全转义字符串（零堆分配）
/// 安全字符串原样写入；空串写入 `''`；含特殊字符用单引号包裹，内部 `'` 转义为 `'\''`
export fn quotePosix(s: []const u8, out: []u8): ([]const u8)? {
    if (s.len == 0) {
        if (out.len < 2) {
            return null;
        }
        out[0] = '\'';
        out[1] = '\'';
        return out[0..2];
    }

    if (!needsQuoting(s)) {
        if (out.len < s.len) {
            return null;
        }
        let i: usize = 0;
        while (i < s.len) {
            out[i] = s[i];
            i += 1;
        }
        return out[0..s.len];
    }

    let pos: usize = 0;
    if (pos >= out.len) return null;
    out[pos] = '\'';
    pos += 1;

    for (c of s) {
        if (c == '\'') {
            if (pos + 4 > out.len) return null;
            out[pos] = '\'';
            out[pos + 1] = '\\';
            out[pos + 2] = '\'';
            out[pos + 3] = '\'';
            pos += 4;
        } else {
            if (pos >= out.len) return null;
            out[pos] = c;
            pos += 1;
        }
    }

    if (pos >= out.len) return null;
    out[pos] = '\'';
    pos += 1;
    return out[0..pos];
}

/// 将多个命令行参数按 POSIX 规则安全转义并以空格拼接（零堆分配）
export fn joinArgs(args: []const []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < args.len) {
        if (i > 0) {
            if (pos >= out.len) return null;
            out[pos] = ' ';
            pos += 1;
        }
        const written = quotePosix(args[i], out[pos..]) ?? return null;
        pos += written.len;
        i += 1;
    }
    return out[0..pos];
}

/// POSIX Shell 命令行参数流式拆分器（零堆分配）
/// 支持单引号字面量、双引号内 `\"`/`\\`/`\$`/`\`` 转义、非引号反斜杠转义、相邻拼接及未闭合引号检测
export struct ShellArgIter {
    src: []const u8;
    pos: usize;
    had_error: bool;

    /// 初始化命令行参数迭代器
    fn init(cmdline: []const u8): ShellArgIter {
        return ShellArgIter{
            src: cmdline,
            pos: 0,
            had_error: false,
        };
    }

    /// 解析过程中是否遇到词法错误（如未闭合引号、孤立尾部反斜杠或缓冲区不足）
    fn hasError(): bool {
        return _.had_error;
    }

    /// 读取下一个展开后的命令行参数写入 out；到达末尾或出错返回 null
    fn next(*_, out: []u8): ([]const u8)? {
        if (_.had_error) {
            return null;
        }

        while (_.pos < _.src.len && isSpace(_.src[_.pos])) {
            _.pos += 1;
        }
        if (_.pos >= _.src.len) {
            return null;
        }

        let out_pos: usize = 0;

        while (_.pos < _.src.len && !isSpace(_.src[_.pos])) {
            const c = _.src[_.pos];

            // 1. 单引号字面量：内部全部原样保留直至闭合单引号
            if (c == '\'') {
                _.pos += 1;
                let closed = false;
                while (_.pos < _.src.len) {
                    if (_.src[_.pos] == '\'') {
                        _.pos += 1;
                        closed = true;
                        break;
                    }
                    if (out_pos >= out.len) {
                        _.had_error = true;
                        return null;
                    }
                    out[out_pos] = _.src[_.pos];
                    out_pos += 1;
                    _.pos += 1;
                }
                if (!closed) {
                    _.had_error = true;
                    return null;
                }
                continue;
            }

            // 2. 双引号字符串：仅对 \" \\ \$ \` 与续行 \n 生效转义
            if (c == '"') {
                _.pos += 1;
                let closed = false;
                while (_.pos < _.src.len) {
                    const ch = _.src[_.pos];
                    if (ch == '"') {
                        _.pos += 1;
                        closed = true;
                        break;
                    }
                    if (ch == '\\' && _.pos + 1 < _.src.len) {
                        const nxt = _.src[_.pos + 1];
                        if (nxt == '\n') {
                            _.pos += 2;
                            continue;
                        }
                        if (nxt == '"' || nxt == '\\' || nxt == '$' || nxt == '`') {
                            if (out_pos >= out.len) {
                                _.had_error = true;
                                return null;
                            }
                            out[out_pos] = nxt;
                            out_pos += 1;
                            _.pos += 2;
                            continue;
                        }
                    }
                    if (out_pos >= out.len) {
                        _.had_error = true;
                        return null;
                    }
                    out[out_pos] = ch;
                    out_pos += 1;
                    _.pos += 1;
                }
                if (!closed) {
                    _.had_error = true;
                    return null;
                }
                continue;
            }

            // 3. 非引号反斜杠转义
            if (c == '\\') {
                if (_.pos + 1 >= _.src.len) {
                    _.had_error = true;
                    return null;
                }
                const nxt = _.src[_.pos + 1];
                if (nxt == '\n') {
                    _.pos += 2;
                    continue;
                }
                if (out_pos >= out.len) {
                    _.had_error = true;
                    return null;
                }
                out[out_pos] = nxt;
                out_pos += 1;
                _.pos += 2;
                continue;
            }

            // 4. 普通非空白字符
            if (out_pos >= out.len) {
                _.had_error = true;
                return null;
            }
            out[out_pos] = c;
            out_pos += 1;
            _.pos += 1;
        }

        return out[0..out_pos];
    }
}
