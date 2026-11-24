// Source: Absorbed from /data/dev/bun/src/env_loader.zig | Synced: 2026-11-21T15:54:12Z (POSIX Shell Env Assignment Parser)

pub struct ShellEnvEntry {
    key: []const u8;
    value: []const u8;
    is_exported: bool;
    line_no: usize;
}

fn isHWhitespace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn isIdentStart(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

fn isIdentChar(c: u8): bool {
    return isIdentStart(c) || (c >= '0' && c <= '9');
}

fn isValidEnvKey(k: []const u8): bool {
    if (k.len == 0) {
        return false;
    }
    if (!isIdentStart(k[0])) {
        return false;
    }
    let i: usize = 1;
    while (i < k.len) {
        if (!isIdentChar(k[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn startsWithKeyword(src: []const u8, pos: usize, kw: []const u8): bool {
    if (pos + kw.len >= src.len) {
        return false;
    }
    let i: usize = 0;
    while (i < kw.len) {
        if (src[pos + i] != kw[i]) {
            return false;
        }
        i += 1;
    }
    return isHWhitespace(src[pos + kw.len]);
}

pub struct ShellEnvIter {
    src: []const u8;
    pos: usize;
    line_no: usize;

    static fn init(content: []const u8): ShellEnvIter {
        let start: usize = 0;
        if (content.len >= 3 && content[0] == 0xEF && content[1] == 0xBB && content[2] == 0xBF) {
            start = 3;
        }
        return ShellEnvIter {
            src: content,
            pos: start,
            line_no: 1,
        };
    }

    fn skipHWhitespace(*_) {
        while (_.pos < _.src.len && isHWhitespace(_.src[_.pos])) {
            _.pos += 1;
        }
    }

    fn skipToNextLine(*_) {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            _.pos += 1;
            if (c == '\n') {
                _.line_no += 1;
                return;
            }
        }
    }

    fn parseSingleQuoted(*_, val_buf: []u8, write_out: bool): usize? {
        // 消费开头的 '\''
        _.pos += 1;
        let out_len: usize = 0;
        let overflow = false;

        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            _.pos += 1;
            if (c == '\'') {
                _.skipToNextLine();
                if (overflow) {
                    return null;
                }
                return out_len;
            }
            if (c == '\n') {
                _.line_no += 1;
            }
            if (write_out) {
                if (out_len < val_buf.len) {
                    val_buf[out_len] = c;
                    out_len += 1;
                } else {
                    overflow = true;
                }
            }
        }
        // 未闭合单引号
        return null;
    }

    fn parseDoubleQuoted(*_, val_buf: []u8, write_out: bool): usize? {
        // 消费开头的 '"'
        _.pos += 1;
        let out_len: usize = 0;
        let overflow = false;

        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            _.pos += 1;
            if (c == '"') {
                _.skipToNextLine();
                if (overflow) {
                    return null;
                }
                return out_len;
            }
            if (c == '\\') {
                if (_.pos >= _.src.len) {
                    return null;
                }
                const next_c = _.src[_.pos];
                _.pos += 1;

                // POSIX 行续接: \<LF> 或 \<CR><LF>
                if (next_c == '\n') {
                    _.line_no += 1;
                    continue;
                }
                if (next_c == '\r' && _.pos < _.src.len && _.src[_.pos] == '\n') {
                    _.pos += 1;
                    _.line_no += 1;
                    continue;
                }

                let decoded: u8 = 0;
                let has_single = true;
                if (next_c == 'n') {
                    decoded = '\n';
                } else if (next_c == 'r') {
                    decoded = '\r';
                } else if (next_c == 't') {
                    decoded = '\t';
                } else if (next_c == '"') {
                    decoded = '"';
                } else if (next_c == '\\') {
                    decoded = '\\';
                } else if (next_c == '$') {
                    decoded = '$';
                } else {
                    has_single = false;
                }

                if (write_out) {
                    if (has_single) {
                        if (out_len < val_buf.len) {
                            val_buf[out_len] = decoded;
                            out_len += 1;
                        } else {
                            overflow = true;
                        }
                    } else {
                        if (out_len + 2 <= val_buf.len) {
                            val_buf[out_len] = '\\';
                            val_buf[out_len + 1] = next_c;
                            out_len += 2;
                        } else {
                            overflow = true;
                        }
                    }
                }
                continue;
            }

            if (c == '\n') {
                _.line_no += 1;
            }
            if (write_out) {
                if (out_len < val_buf.len) {
                    val_buf[out_len] = c;
                    out_len += 1;
                } else {
                    overflow = true;
                }
            }
        }
        // 未闭合双引号
        return null;
    }

    fn parseUnquoted(*_, val_buf: []u8, write_out: bool): usize? {
        const start = _.pos;
        let end = start;
        let found_comment = false;

        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == '\n') {
                end = _.pos;
                _.pos += 1;
                _.line_no += 1;
                break;
            }
            if (c == '\r') {
                end = _.pos;
                _.pos += 1;
                if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                    _.pos += 1;
                }
                _.line_no += 1;
                break;
            }
            if (c == '#' && _.pos > start && isHWhitespace(_.src[_.pos - 1])) {
                end = _.pos;
                found_comment = true;
                break;
            }
            _.pos += 1;
            end = _.pos;
        }

        if (found_comment) {
            _.skipToNextLine();
        }

        let trim_start = start;
        while (trim_start < end && isHWhitespace(_.src[trim_start])) {
            trim_start += 1;
        }
        let trim_end = end;
        while (trim_end > trim_start && isHWhitespace(_.src[trim_end - 1])) {
            trim_end -= 1;
        }

        const val_len = trim_end - trim_start;
        if (!write_out) {
            return 0;
        }
        if (val_len > val_buf.len) {
            return null;
        }
        let i: usize = 0;
        while (i < val_len) {
            val_buf[i] = _.src[trim_start + i];
            i += 1;
        }
        return val_len;
    }

    fn parseValue(*_, val_buf: []u8, write_out: bool): usize? {
        if (_.pos < _.src.len) {
            if (_.src[_.pos] == '\'') {
                return _.parseSingleQuoted(val_buf, write_out);
            }
            if (_.src[_.pos] == '"') {
                return _.parseDoubleQuoted(val_buf, write_out);
            }
        }
        return _.parseUnquoted(val_buf, write_out);
    }

    fn next(*_, val_buf: []u8): ShellEnvEntry? {
        while (_.pos < _.src.len) {
            const stmt_line = _.line_no;
            _.skipHWhitespace();
            if (_.pos >= _.src.len) {
                return null;
            }

            const first = _.src[_.pos];
            if (first == '\n') {
                _.pos += 1;
                _.line_no += 1;
                continue;
            }
            if (first == '\r') {
                _.pos += 1;
                if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                    _.pos += 1;
                }
                _.line_no += 1;
                continue;
            }
            if (first == '#') {
                _.skipToNextLine();
                continue;
            }

            // 跳过 unset KEY 语句
            if (startsWithKeyword(_.src, _.pos, "unset")) {
                _.skipToNextLine();
                continue;
            }

            let is_exported = false;
            if (startsWithKeyword(_.src, _.pos, "export")) {
                is_exported = true;
                _.pos += 6;
                _.skipHWhitespace();
            }

            const key_start = _.pos;
            while (_.pos < _.src.len) {
                const c = _.src[_.pos];
                if (c == '=' || isHWhitespace(c) || c == '\n' || c == '\r' || c == '#') {
                    break;
                }
                _.pos += 1;
            }
            const key = _.src[key_start.._.pos];

            if (_.pos >= _.src.len || _.src[_.pos] != '=' || !isValidEnvKey(key)) {
                _.skipToNextLine();
                continue;
            }

            // 消费 '='
            _.pos += 1;

            if (val_len := _.parseValue(val_buf, true)) {
                return ShellEnvEntry {
                    key: key,
                    value: val_buf[0..val_len],
                    is_exported: is_exported,
                    line_no: stmt_line,
                };
            }
        }
        return null;
    }
}

fn lineUnsetsKey(line: []const u8, target_key: []const u8): bool {
    let i: usize = 0;
    let is_func_mode = false;
    let allow_flags = true;

    while (i < line.len) {
        while (i < line.len && isHWhitespace(line[i])) {
            i += 1;
        }
        if (i >= line.len || line[i] == '#') {
            break;
        }
        const tok_start = i;
        while (i < line.len && !isHWhitespace(line[i]) && line[i] != '#') {
            i += 1;
        }
        const tok = line[tok_start..i];
        if (allow_flags && tok.len >= 2 && tok[0] == '-') {
            if (bytesEql(tok, "--")) {
                allow_flags = false;
                continue;
            }
            if (bytesEql(tok, "-f")) {
                is_func_mode = true;
                continue;
            }
            if (bytesEql(tok, "-v")) {
                is_func_mode = false;
                continue;
            }
            continue;
        }
        allow_flags = false;
        if (!is_func_mode && bytesEql(tok, target_key)) {
            return true;
        }
    }
    return false;
}

pub fn getShellEnv(content: []const u8, key: []const u8, val_buf: []u8): ([]const u8)? {
    if (!isValidEnvKey(key)) {
        return null;
    }

    let it = ShellEnvIter.init(content);
    let found = false;
    let found_len: usize = 0;

    while (it.pos < it.src.len) {
        it.skipHWhitespace();
        if (it.pos >= it.src.len) {
            break;
        }

        const first = it.src[it.pos];
        if (first == '\n') {
            it.pos += 1;
            it.line_no += 1;
            continue;
        }
        if (first == '\r') {
            it.pos += 1;
            if (it.pos < it.src.len && it.src[it.pos] == '\n') {
                it.pos += 1;
            }
            it.line_no += 1;
            continue;
        }
        if (first == '#') {
            it.skipToNextLine();
            continue;
        }

        if (startsWithKeyword(it.src, it.pos, "unset")) {
            it.pos += 5;
            const arg_start = it.pos;
            while (it.pos < it.src.len && it.src[it.pos] != '\n' && it.src[it.pos] != '\r') {
                it.pos += 1;
            }
            const args = it.src[arg_start..it.pos];
            if (it.pos < it.src.len) {
                if (it.src[it.pos] == '\r') {
                    it.pos += 1;
                    if (it.pos < it.src.len && it.src[it.pos] == '\n') {
                        it.pos += 1;
                    }
                } else if (it.src[it.pos] == '\n') {
                    it.pos += 1;
                }
                it.line_no += 1;
            }
            if (lineUnsetsKey(args, key)) {
                found = false;
                found_len = 0;
            }
            continue;
        }

        if (startsWithKeyword(it.src, it.pos, "export")) {
            it.pos += 6;
            it.skipHWhitespace();
        }

        const key_start = it.pos;
        while (it.pos < it.src.len) {
            const c = it.src[it.pos];
            if (c == '=' || isHWhitespace(c) || c == '\n' || c == '\r' || c == '#') {
                break;
            }
            it.pos += 1;
        }
        const cur_key = it.src[key_start..it.pos];

        if (it.pos >= it.src.len || it.src[it.pos] != '=' || !isValidEnvKey(cur_key)) {
            it.skipToNextLine();
            continue;
        }

        // 消费 '='
        it.pos += 1;

        if (bytesEql(cur_key, key)) {
            if (val_len := it.parseValue(val_buf, true)) {
                found = true;
                found_len = val_len;
            } else {
                // 目标键被覆盖但值畸形或超出 val_buf 容量，清空旧值
                found = false;
                found_len = 0;
            }
        } else {
            * = it.parseValue(val_buf, false);
        }
    }

    if (!found) {
        return null;
    }
    return val_buf[0..found_len];
}
