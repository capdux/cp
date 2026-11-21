// Source: Absorbed from /data/dev/bun/src/sql/postgres/PostgresSQLQuery.zig | Synced: 2026-11-21T14:48:18Z (SQL Identifier & Literal Escaper)

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

fn containsNul(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 0) { return true; }
        i += 1;
    }
    return false;
}

/// PostgreSQL / ANSI SQL 标识符转义：双引号包裹，内部 `"` 转义为 `""`；拒绝空串与 `\0` 字节
pub fn escapeAnsiIdent(ident: []const u8, out: []u8): ([]const u8)? {
    if (ident.len == 0 || containsNul(ident)) {
        return null;
    }
    let w = OutWriter{ buf: out };
    w.writeByte('"');
    let i: usize = 0;
    while (i < ident.len) {
        if (ident[i] == '"') {
            w.writeByte('"');
            w.writeByte('"');
        } else {
            w.writeByte(ident[i]);
        }
        i += 1;
    }
    w.writeByte('"');
    return w.finish();
}

/// MySQL 标识符转义：反引号包裹，内部 `` ` `` 转义为 `` `` ``；拒绝空串与 `\0` 字节
pub fn escapeMysqlIdent(ident: []const u8, out: []u8): ([]const u8)? {
    if (ident.len == 0 || containsNul(ident)) {
        return null;
    }
    let w = OutWriter{ buf: out };
    w.writeByte('`');
    let i: usize = 0;
    while (i < ident.len) {
        if (ident[i] == '`') {
            w.writeByte('`');
            w.writeByte('`');
        } else {
            w.writeByte(ident[i]);
        }
        i += 1;
    }
    w.writeByte('`');
    return w.finish();
}

/// 按 `.` 切分并逐段转义限定 ANSI 标识符（如 `"public.users"` -> `"\"public\".\"users\""`）
/// 拒绝空串、空段（如 `".a"`、`"a."`、`"a..b"`）与 `\0` 字节
pub fn escapeQualifiedAnsiIdent(qual_name: []const u8, out: []u8): ([]const u8)? {
    if (qual_name.len == 0 || containsNul(qual_name)) {
        return null;
    }

    let w = OutWriter{ buf: out };
    let pos: usize = 0;
    let first = true;

    while (pos <= qual_name.len) {
        const start = pos;
        while (pos < qual_name.len && qual_name[pos] != '.') {
            pos += 1;
        }
        const seg = qual_name[start..pos];
        if (seg.len == 0) {
            return null;
        }

        if (!first) {
            w.writeByte('.');
        }
        first = false;

        w.writeByte('"');
        let k: usize = 0;
        while (k < seg.len) {
            if (seg[k] == '"') {
                w.writeByte('"');
                w.writeByte('"');
            } else {
                w.writeByte(seg[k]);
            }
            k += 1;
        }
        w.writeByte('"');

        if (pos == qual_name.len) {
            break;
        }
        pos += 1;
    }

    return w.finish();
}

/// PostgreSQL / ANSI SQL 字符串字面量转义：单引号包裹，内部 `'` 转义为 `''`；拒绝 `\0` 字节
pub fn escapeSqlStringAnsi(val: []const u8, out: []u8): ([]const u8)? {
    if (containsNul(val)) {
        return null;
    }
    let w = OutWriter{ buf: out };
    w.writeByte('\'');
    let i: usize = 0;
    while (i < val.len) {
        if (val[i] == '\'') {
            w.writeByte('\'');
            w.writeByte('\'');
        } else {
            w.writeByte(val[i]);
        }
        i += 1;
    }
    w.writeByte('\'');
    return w.finish();
}

/// MySQL 字符串字面量转义：单引号包裹，转义 `\'`、`\"`、`\\`、`\n`、`\r`、`\0`、`\Z`（0x1A）
pub fn escapeSqlStringMysql(val: []const u8, out: []u8): ([]const u8)? {
    let w = OutWriter{ buf: out };
    w.writeByte('\'');
    let i: usize = 0;
    while (i < val.len) {
        const c = val[i];
        switch (c) {
            '\'' => w.writeStr("\\'"),
            '"' => w.writeStr("\\\""),
            '\\' => w.writeStr("\\\\"),
            '\n' => w.writeStr("\\n"),
            '\r' => w.writeStr("\\r"),
            0x00 => w.writeStr("\\0"),
            0x1A => w.writeStr("\\Z"),
            else => w.writeByte(c),
        }
        i += 1;
    }
    w.writeByte('\'');
    return w.finish();
}

/// 转义 SQL `LIKE` 模式中的 `%`、`_` 与转义字符 `esc_char` 本身
pub fn escapeLikePattern(val: []const u8, esc_char: u8, out: []u8): ([]const u8)? {
    if (esc_char == 0) {
        return null;
    }
    let w = OutWriter{ buf: out };
    let i: usize = 0;
    while (i < val.len) {
        const c = val[i];
        if (c == '%' || c == '_' || c == esc_char) {
            w.writeByte(esc_char);
        }
        w.writeByte(c);
        i += 1;
    }
    return w.finish();
}
