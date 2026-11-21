// Source: Absorbed from /data/dev/bun/src/env_loader.zig | Synced: 2026-11-21T15:40:17Z (.env Key Validator & Safe Value Serializer)

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

fn isKeyStart(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

fn isKeyCont(c: u8): bool {
    return isKeyStart(c) || (c >= '0' && c <= '9');
}

/// 校验 `.env` 环境变量键名合法性：`[A-Za-z_][A-Za-z0-9_]*`
pub fn isValidEnvKey(key: []const u8): bool {
    if (key.len == 0 || !isKeyStart(key[0])) {
        return false;
    }
    let i: usize = 1;
    while (i < key.len) {
        if (!isKeyCont(key[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 判断 `.env` 值是否需要加双引号保护（空串、空白、控制字符或 `#`/`"`/`'`/`\`/`$`/`` ` ``）
pub fn needsDotenvQuotes(val: []const u8): bool {
    if (val.len == 0) {
        return true;
    }
    let i: usize = 0;
    while (i < val.len) {
        const c = val[i];
        if (c <= 0x20 || c == 0x7F || c == '#' || c == '"' || c == '\'' || c == '\\' || c == '$' || c == '`') {
            return true;
        }
        i += 1;
    }
    return false;
}

fn writeDotenvValue(w: *OutWriter, val: []const u8) {
    if (!needsDotenvQuotes(val)) {
        w.writeStr(val);
        return;
    }
    w.writeByte('"');
    let i: usize = 0;
    while (i < val.len) {
        const c = val[i];
        switch (c) {
            '"' => w.writeStr("\\\""),
            '\\' => w.writeStr("\\\\"),
            '\n' => w.writeStr("\\n"),
            '\r' => w.writeStr("\\r"),
            '\t' => w.writeStr("\\t"),
            '$' => w.writeStr("\\$"),
            else => w.writeByte(c),
        }
        i += 1;
    }
    w.writeByte('"');
}

/// 安全序列化 `.env` 值：安全值原样输出；含特殊字符则双引号包裹并转义 `\"`/`\\`/`\n`/`\r`/`\t`/`\$`
pub fn formatDotenvValue(val: []const u8, out: []u8): ([]const u8)? {
    let w = OutWriter{ buf: out };
    writeDotenvValue(*w, val);
    return w.finish();
}

/// 校验 `key` 合法性并安全格式化整行 `KEY=VALUE` 写入 `out`
pub fn formatDotenvLine(key: []const u8, val: []const u8, out: []u8): ([]const u8)? {
    if (!isValidEnvKey(key)) {
        return null;
    }
    let w = OutWriter{ buf: out };
    w.writeStr(key);
    w.writeByte('=');
    writeDotenvValue(*w, val);
    return w.finish();
}

/// 还原双引号 `.env` 转义序列（`\n`/`\r`/`\t`/`\"`/`\\`/`\$`），兼容含外层 `"` 或纯内部切片
pub fn unescapeDotenvDouble(escaped: []const u8, out: []u8): ([]const u8)? {
    let body = escaped;
    if (escaped.len >= 1 && escaped[0] == '"') {
        if (escaped.len < 2 || escaped[escaped.len - 1] != '"') {
            return null;
        }
        body = escaped[1..(escaped.len - 1)];
    }

    let w = OutWriter{ buf: out };
    let i: usize = 0;
    while (i < body.len) {
        if (body[i] == '\\') {
            if (i + 1 >= body.len) {
                return null;
            }
            const esc = body[i + 1];
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
            w.writeByte(body[i]);
            i += 1;
        }
    }
    return w.finish();
}
