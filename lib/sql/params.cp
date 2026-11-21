// Source: Absorbed from /data/dev/bun/src/sql/postgres/PostgresSQLQuery.zig (SQL Parameter Scanner & Dialect Rewriter)

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn isIdentCont(c: u8): bool {
    return isIdentStart(c) || isDigit(c);
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

/// 若 `sql[pos]` 起始于字符串字面量、引号标识符、Dollar-quoted 字符串或注释，则返回跳过后的位置；否则返回 `null`
fn skipQuotedOrComment(sql: []const u8, pos: usize): usize? {
    if (pos >= sql.len) { return null; }
    const c = sql[pos];

    // 1. 单引号字符串 '...' (含 '' 转义)、双引号标识符 "..." (含 "" 转义)、反引号标识符 `...` (含 `` 转义)
    if (c == '\'' || c == '"' || c == '`') {
        let i = pos + 1;
        while (i < sql.len) {
            if (sql[i] == c) {
                if (i + 1 < sql.len && sql[i + 1] == c) {
                    i += 2;
                    continue;
                }
                return i + 1;
            }
            i += 1;
        }
        return sql.len;
    }

    // 2. 行注释 -- ...\n
    if (c == '-' && pos + 1 < sql.len && sql[pos + 1] == '-') {
        let i = pos + 2;
        while (i < sql.len && sql[i] != '\n') {
            i += 1;
        }
        if (i < sql.len && sql[i] == '\n') {
            i += 1;
        }
        return i;
    }

    // 3. 块注释 /* ... */（支持 PostgreSQL 嵌套块注释）
    if (c == '/' && pos + 1 < sql.len && sql[pos + 1] == '*') {
        let i = pos + 2;
        let depth: usize = 1;
        while (i + 1 < sql.len && depth > 0) {
            if (sql[i] == '/' && sql[i + 1] == '*') {
                depth += 1;
                i += 2;
            } else if (sql[i] == '*' && sql[i + 1] == '/') {
                depth -= 1;
                i += 2;
            } else {
                i += 1;
            }
        }
        return if (depth == 0) i else sql.len;
    }

    // 4. PostgreSQL Dollar-quoted 字符串 $$...$$ 或 $tag$...$tag$
    if (c == '$') {
        let k = pos + 1;
        if (k < sql.len && sql[k] == '$') {
            // $$...$$
            let scan = k + 1;
            while (scan + 1 < sql.len) {
                if (sql[scan] == '$' && sql[scan + 1] == '$') {
                    return scan + 2;
                }
                scan += 1;
            }
            return sql.len;
        }
        if (k < sql.len && isIdentStart(sql[k])) {
            while (k < sql.len && isIdentCont(sql[k])) {
                k += 1;
            }
            if (k < sql.len && sql[k] == '$') {
                const tag = sql[pos..(k + 1)]; // 含首尾 '$'
                let scan = k + 1;
                while (scan + tag.len <= sql.len) {
                    if (sql[scan] == '$' && eqlBytes(sql[scan..(scan + tag.len)], tag)) {
                        return scan + tag.len;
                    }
                    scan += 1;
                }
                return sql.len;
            }
        }
    }

    return null;
}

/// 统计 SQL 中真实 `?` 参数占位符数量（自动跳过字符串、引号标识符、Dollar-quoted 与注释）
pub fn countQuestionMarks(sql: []const u8): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < sql.len) {
        if (nxt := skipQuotedOrComment(sql, i)) {
            i = nxt;
            continue;
        }
        if (sql[i] == '?') {
            count += 1;
        }
        i += 1;
    }
    return count;
}

/// 统计 SQL 中最大 PostgreSQL `$N` 参数编号（如 `SELECT $2, $1` -> `2`），忽略字符串与注释内 `$N`
pub fn maxDollarParam(sql: []const u8): u32 {
    let max_n: u32 = 0;
    let i: usize = 0;
    while (i < sql.len) {
        if (nxt := skipQuotedOrComment(sql, i)) {
            i = nxt;
            continue;
        }
        if (sql[i] == '$' && i + 1 < sql.len && isDigit(sql[i + 1])) {
            let k = i + 1;
            let acc: u64 = 0;
            let overflow = false;
            while (k < sql.len && isDigit(sql[k])) {
                acc = acc * 10 + @as(u64, sql[k] - '0');
                if (acc > 4294967295) {
                    overflow = true;
                }
                k += 1;
            }
            if (!overflow) {
                const n: u32 = @intCast(acc);
                if (n > max_n) {
                    max_n = n;
                }
            }
            i = k;
            continue;
        }
        i += 1;
    }
    return max_n;
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

    fn writeUsize(*_, val: usize) {
        let digits: [24]u8 = [0; 24];
        let dlen: usize = 0;
        let tmp = val;
        if (tmp == 0) {
            _.writeByte('0');
            return;
        }
        while (tmp > 0 && dlen < 24) {
            digits[dlen] = @intCast((tmp % 10) + '0');
            tmp /= 10;
            dlen += 1;
        }
        let k = dlen;
        while (k > 0) {
            k -= 1;
            _.writeByte(digits[k]);
        }
    }

    fn finish(): ([]const u8)? {
        if (_.overflow) { return null; }
        return _.buf[0.._.pos];
    }
}

/// 将 MySQL/SQLite 风格 `?` 占位符按序重写为 PostgreSQL `$1`, `$2`, ...
/// 原样保留字符串字面量、标识符、Dollar-quoted 与注释中的 `?`；输出缓冲不足返回 `null`
pub fn rewriteQuestionToDollar(sql: []const u8, out: []u8): ([]const u8)? {
    let w = OutWriter{ buf: out };
    let idx: usize = 0;
    let i: usize = 0;

    while (i < sql.len) {
        if (nxt := skipQuotedOrComment(sql, i)) {
            w.writeStr(sql[i..nxt]);
            i = nxt;
            continue;
        }
        if (sql[i] == '?') {
            idx += 1;
            w.writeByte('$');
            w.writeUsize(idx);
            i += 1;
            continue;
        }
        w.writeByte(sql[i]);
        i += 1;
    }

    return w.finish();
}

/// 零堆分配 SQL 命名参数 `:name` 迭代器（自动跳过 PostgreSQL `::type` 类型转换及字符串/注释）
pub struct NamedParamIter {
    sql: []const u8,
    pos: usize = 0,

    static fn init(sql: []const u8): NamedParamIter {
        return NamedParamIter{ sql: sql, pos: 0 };
    }

    /// 返回下一个命名参数名（不含前导 `:`）
    fn next(*_): ([]const u8)? {
        while (_.pos < _.sql.len) {
            if (nxt := skipQuotedOrComment(_.sql, _.pos)) {
                _.pos = nxt;
                continue;
            }
            if (_.sql[_.pos] == ':') {
                // 跳过 PostgreSQL 双冒号类型转换 ::type
                if (_.pos + 1 < _.sql.len && _.sql[_.pos + 1] == ':') {
                    _.pos += 2;
                    continue;
                }
                if (_.pos + 1 < _.sql.len && isIdentStart(_.sql[_.pos + 1])) {
                    const start = _.pos + 1;
                    let end = start + 1;
                    while (end < _.sql.len && isIdentCont(_.sql[end])) {
                        end += 1;
                    }
                    _.pos = end;
                    return _.sql[start..end];
                }
            }
            _.pos += 1;
        }
        return null;
    }
}
