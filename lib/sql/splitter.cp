// Source: Absorbed from /data/dev/bun/src/sql/postgres/PostgresSQLQuery.zig (Multi-Statement SQL Script Splitter)

/// SQL 语句顶层类型枚举
pub enum SqlSplitStmtKind: u8 {
    select = 0,
    insert = 1,
    update = 2,
    delete = 3,
    create = 4,
    drop = 5,
    alter = 6,
    begin = 7,
    commit = 8,
    rollback = 9,
    pragma = 10,
    other = 11,

    fn asStr(): string {
        return switch (_) {
            .select => "select",
            .insert => "insert",
            .update => "update",
            .delete => "delete",
            .create => "create",
            .drop => "drop",
            .alter => "alter",
            .begin => "begin",
            .commit => "commit",
            .rollback => "rollback",
            .pragma => "pragma",
            .other => "other",
        };
    }
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0C;
}

fn isAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn isIdentCont(c: u8): bool {
    return isAlpha(c) || (c >= '0' && c <= '9');
}

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    let end = s.len;
    while (start < end && isSpace(s[start])) { start += 1; }
    while (end > start && isSpace(s[end - 1])) { end -= 1; }
    return s[start..end];
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

fn skipComment(sql: []const u8, pos: usize): usize? {
    if (pos + 1 >= sql.len) { return null; }
    if (sql[pos] == '-' && sql[pos + 1] == '-') {
        let i = pos + 2;
        while (i < sql.len && sql[i] != '\n') { i += 1; }
        if (i < sql.len && sql[i] == '\n') { i += 1; }
        return i;
    }
    if (sql[pos] == '/' && sql[pos + 1] == '*') {
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
    return null;
}

fn skipQuoted(sql: []const u8, pos: usize): usize? {
    if (pos >= sql.len) { return null; }
    const c = sql[pos];

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

    if (c == '$') {
        let k = pos + 1;
        if (k < sql.len && sql[k] == '$') {
            let scan = k + 1;
            while (scan + 1 < sql.len) {
                if (sql[scan] == '$' && sql[scan + 1] == '$') {
                    return scan + 2;
                }
                scan += 1;
            }
            return sql.len;
        }
        if (k < sql.len && isAlpha(sql[k])) {
            while (k < sql.len && isIdentCont(sql[k])) { k += 1; }
            if (k < sql.len && sql[k] == '$') {
                const tag = sql[pos..(k + 1)];
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

fn skipSpaceAndComments(sql: []const u8, start: usize): usize {
    let i = start;
    while (i < sql.len) {
        if (isSpace(sql[i])) {
            i += 1;
            continue;
        }
        if (nxt := skipComment(sql, i)) {
            i = nxt;
            continue;
        }
        break;
    }
    return i;
}

/// 零堆分配多语句 SQL 脚本分号切分迭代器
/// 按顶层 `;` 切分，安全跳过字符串/引号标识符/注释/Dollar-quoted 内的 `;`，自动跳过纯注释与空语句
pub struct SqlStmtIter {
    script: []const u8,
    pos: usize = 0,

    static fn init(script: []const u8): SqlStmtIter {
        let start_pos: usize = 0;
        if (script.len >= 3 && script[0] == 0xEF && script[1] == 0xBB && script[2] == 0xBF) {
            start_pos = 3;
        }
        return SqlStmtIter{ script: script, pos: start_pos };
    }

    fn next(*_): ([]const u8)? {
        while (_.pos < _.script.len) {
            const stmt_start = _.pos;
            let has_code = false;

            while (_.pos < _.script.len) {
                const c = _.script[_.pos];
                if (isSpace(c)) {
                    _.pos += 1;
                    continue;
                }
                if (nxt := skipComment(_.script, _.pos)) {
                    _.pos = nxt;
                    continue;
                }
                if (nxt := skipQuoted(_.script, _.pos)) {
                    has_code = true;
                    _.pos = nxt;
                    continue;
                }
                if (c == ';') {
                    break;
                }
                has_code = true;
                _.pos += 1;
            }

            const stmt_end = _.pos;
            if (_.pos < _.script.len && _.script[_.pos] == ';') {
                _.pos += 1;
            }

            if (has_code) {
                return trimSpaces(_.script[stmt_start..stmt_end]);
            }
        }
        return null;
    }
}

fn keywordToKind(kw: []const u8): SqlSplitStmtKind {
    if (eqlIgnoreCaseAscii(kw, "select") || eqlIgnoreCaseAscii(kw, "values")) { return .select; }
    if (eqlIgnoreCaseAscii(kw, "insert") || eqlIgnoreCaseAscii(kw, "replace")) { return .insert; }
    if (eqlIgnoreCaseAscii(kw, "update")) { return .update; }
    if (eqlIgnoreCaseAscii(kw, "delete")) { return .delete; }
    if (eqlIgnoreCaseAscii(kw, "create")) { return .create; }
    if (eqlIgnoreCaseAscii(kw, "drop")) { return .drop; }
    if (eqlIgnoreCaseAscii(kw, "alter")) { return .alter; }
    if (eqlIgnoreCaseAscii(kw, "begin") || eqlIgnoreCaseAscii(kw, "start")) { return .begin; }
    if (eqlIgnoreCaseAscii(kw, "commit")) { return .commit; }
    if (eqlIgnoreCaseAscii(kw, "rollback")) { return .rollback; }
    if (eqlIgnoreCaseAscii(kw, "pragma")) { return .pragma; }
    return .other;
}

/// 跳过 `WITH ...` CTE 定义体，定位其后的主语句关键字（如 `SELECT`/`INSERT`/`UPDATE`/`DELETE`）
fn classifyWithCte(stmt: []const u8, after_with: usize): SqlSplitStmtKind {
    let i = after_with;
    let paren_depth: usize = 0;
    let saw_paren = false;

    while (i < stmt.len) {
        i = skipSpaceAndComments(stmt, i);
        if (i >= stmt.len) { break; }

        if (nxt := skipQuoted(stmt, i)) {
            i = nxt;
            continue;
        }

        const c = stmt[i];
        if (c == '(') {
            paren_depth += 1;
            saw_paren = true;
            i += 1;
            continue;
        }
        if (c == ')') {
            if (paren_depth > 0) { paren_depth -= 1; }
            i += 1;
            continue;
        }

        if (paren_depth == 0 && saw_paren && isAlpha(c)) {
            const start = i;
            while (i < stmt.len && isIdentCont(stmt[i])) { i += 1; }
            const word = stmt[start..i];
            const k = keywordToKind(word);
            if (k == .select || k == .insert || k == .update || k == .delete) {
                return k;
            }
            continue;
        }

        i += 1;
    }

    return .select;
}

/// 跳过前导空白与注释后大小写不敏感识别 SQL 语句类型（支持 `WITH` CTE 主语句识别）
pub fn classifySqlStmt(stmt: []const u8): SqlSplitStmtKind {
    const start = skipSpaceAndComments(stmt, 0);
    if (start >= stmt.len || !isAlpha(stmt[start])) {
        return .other;
    }
    let end = start;
    while (end < stmt.len && isIdentCont(stmt[end])) {
        end += 1;
    }
    const kw = stmt[start..end];
    if (eqlIgnoreCaseAscii(kw, "with")) {
        return classifyWithCte(stmt, end);
    }
    return keywordToKind(kw);
}

/// 判断 SQL 语句是否为只读查询（`SELECT` / `EXPLAIN` / `SHOW` / 只读 `WITH ... SELECT`）
pub fn isReadOnlySql(stmt: []const u8): bool {
    const start = skipSpaceAndComments(stmt, 0);
    if (start >= stmt.len || !isAlpha(stmt[start])) {
        return false;
    }
    let end = start;
    while (end < stmt.len && isIdentCont(stmt[end])) {
        end += 1;
    }
    const kw = stmt[start..end];
    if (eqlIgnoreCaseAscii(kw, "select") ||
        eqlIgnoreCaseAscii(kw, "explain") ||
        eqlIgnoreCaseAscii(kw, "show") ||
        eqlIgnoreCaseAscii(kw, "values"))
    {
        return true;
    }
    if (eqlIgnoreCaseAscii(kw, "with")) {
        return classifyWithCte(stmt, end) == .select;
    }
    return false;
}

/// 统计多语句 SQL 脚本中的有效语句总数
pub fn countSqlStatements(script: []const u8): usize {
    let it = SqlStmtIter.init(script);
    let n: usize = 0;
    while (it.next() != null) {
        n += 1;
    }
    return n;
}
