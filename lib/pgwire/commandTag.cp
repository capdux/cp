// Source: Absorbed from /data/dev/bun/src/sql/postgres/CommandTag.cp | Synced: 2026-11-22T04:11:45Z
// Pure-CP zero-allocation PostgreSQL v3.0 `CommandComplete` command tag parser and formatter.

/// `PgCommandTag` 格式化错误集
pub err PgCommandTagError {
    BufferTooSmall,
}

/// PostgreSQL `CommandComplete` 已知命令分类（对齐 Bun `KnownCommandEnum`）
pub enum PgCommandKind: u8 {
    Insert = 0,
    Delete = 1,
    Update = 2,
    Merge = 3,
    Select = 4,
    Move = 5,
    Fetch = 6,
    Copy = 7,
    Other = 8,
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

fn indexOfChar(s: []const u8, target: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == target) {
            return i;
        }
        i += 1;
    }
    return null;
}

/// 严格解析无符号十进制 `u64`（不允许空串、符号、前导/尾随空白，溢出返回 `null`）
fn parseStrictU64(s: []const u8): u64? {
    if (s.len == 0) {
        return null;
    }
    const max_div_10: u64 = 1844674407370955161;
    const max_mod_10: u64 = 5;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        const digit: u64 = @as(u64, c - '0');
        if (acc > max_div_10 || (acc == max_div_10 && digit > max_mod_10)) {
            return null;
        }
        acc = acc * 10 + digit;
        i += 1;
    }
    return acc;
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn appendChar(out: []u8, pos: usize, ch: u8): usize! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = ch;
    return pos + 1;
}

fn appendU64(out: []u8, pos: usize, value: u64): usize! {
    if (value == 0) {
        return appendChar(out, pos, '0');
    }
    let tmp: [20]u8 = [0; 20];
    let len: usize = 0;
    let v = value;
    while (v > 0) {
        tmp[len] = @as(u8, @intCast(v % 10)) + '0';
        v = v / 10;
        len += 1;
    }
    if (len > out.len - pos) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < len) {
        out[pos + i] = tmp[len - 1 - i];
        i += 1;
    }
    return pos + len;
}

fn matchKnownCommand(prefix: []const u8): PgCommandKind? {
    if (bytesEql(prefix, "INSERT")) return PgCommandKind.Insert;
    if (bytesEql(prefix, "DELETE")) return PgCommandKind.Delete;
    if (bytesEql(prefix, "UPDATE")) return PgCommandKind.Update;
    if (bytesEql(prefix, "MERGE")) return PgCommandKind.Merge;
    if (bytesEql(prefix, "SELECT")) return PgCommandKind.Select;
    if (bytesEql(prefix, "MOVE")) return PgCommandKind.Move;
    if (bytesEql(prefix, "FETCH")) return PgCommandKind.Fetch;
    if (bytesEql(prefix, "COPY")) return PgCommandKind.Copy;
    return null;
}

/// PostgreSQL `CommandComplete` 命令标签解析结果（对齐 Bun `CommandTag`）
///
/// 支持解析 `INSERT <oid> <rows>`、`DELETE <rows>`、`UPDATE <rows>`、`MERGE <rows>`、
/// `SELECT <rows>`、`MOVE <rows>`、`FETCH <rows>`、`COPY <rows>`，
/// 其他 DDL/事务标签或非法格式自动安全回退为 `PgCommandKind.Other`。
pub struct PgCommandTag {
    kind: PgCommandKind = PgCommandKind.Other,
    rows: u64 = 0,
    oid: u64 = 0,
    raw: []const u8 = "",

    /// 解析 PostgreSQL `CommandComplete` 报文中的命令标签字符串
    static fn parse(tag: []const u8): PgCommandTag {
        const first_space = indexOfChar(tag, ' ') ?? return PgCommandTag{
            kind: PgCommandKind.Other,
            rows: 0,
            oid: 0,
            raw: tag,
        };
        const cmd = matchKnownCommand(tag[0..first_space]) ?? return PgCommandTag{
            kind: PgCommandKind.Other,
            rows: 0,
            oid: 0,
            raw: tag,
        };

        if (cmd == PgCommandKind.Insert) {
            const after_first = tag[first_space + 1 ..];
            const second_space = indexOfChar(after_first, ' ') ?? return PgCommandTag{
                kind: PgCommandKind.Other,
                rows: 0,
                oid: 0,
                raw: tag,
            };
            const oid_val = parseStrictU64(after_first[0..second_space]) ?? return PgCommandTag{
                kind: PgCommandKind.Other,
                rows: 0,
                oid: 0,
                raw: tag,
            };
            const rows_val = parseStrictU64(after_first[second_space + 1 ..]) ?? return PgCommandTag{
                kind: PgCommandKind.Other,
                rows: 0,
                oid: 0,
                raw: tag,
            };
            return PgCommandTag{
                kind: PgCommandKind.Insert,
                rows: rows_val,
                oid: oid_val,
                raw: tag,
            };
        }

        const rows_val = parseStrictU64(tag[first_space + 1 ..]) ?? return PgCommandTag{
            kind: PgCommandKind.Other,
            rows: 0,
            oid: 0,
            raw: tag,
        };
        return PgCommandTag{
            kind: cmd,
            rows: rows_val,
            oid: 0,
            raw: tag,
        };
    }

    /// 返回规范命令名（已知命令返回大写关键字，`Other` 返回原始标签）
    fn commandName(): []const u8 {
        if (_.kind == PgCommandKind.Insert) return "INSERT";
        if (_.kind == PgCommandKind.Delete) return "DELETE";
        if (_.kind == PgCommandKind.Update) return "UPDATE";
        if (_.kind == PgCommandKind.Merge) return "MERGE";
        if (_.kind == PgCommandKind.Select) return "SELECT";
        if (_.kind == PgCommandKind.Move) return "MOVE";
        if (_.kind == PgCommandKind.Fetch) return "FETCH";
        if (_.kind == PgCommandKind.Copy) return "COPY";
        return _.raw;
    }

    /// 若为带影响/返回行数的 8 类标准命令，返回行数；否则返回 `null`
    fn rowCount(): u64? {
        if (_.kind == PgCommandKind.Other) {
            return null;
        }
        return _.rows;
    }

    /// 是否为数据变更 DML 命令（`INSERT` / `DELETE` / `UPDATE` / `MERGE`）
    fn isDml(): bool {
        return _.kind == PgCommandKind.Insert ||
            _.kind == PgCommandKind.Delete ||
            _.kind == PgCommandKind.Update ||
            _.kind == PgCommandKind.Merge;
    }

    /// 是否为查询或游标移动命令（`SELECT` / `MOVE` / `FETCH`）
    fn isQueryOrCursor(): bool {
        return _.kind == PgCommandKind.Select ||
            _.kind == PgCommandKind.Move ||
            _.kind == PgCommandKind.Fetch;
    }

    fn eql(other: PgCommandTag): bool {
        if (_.kind != other.kind) {
            return false;
        }
        if (_.kind == PgCommandKind.Other) {
            return bytesEql(_.raw, other.raw);
        }
        return _.rows == other.rows && _.oid == other.oid;
    }

    /// 将命令标签格式化为标准 PostgreSQL `CommandComplete` 字符串
    fn format(out: []u8): usize! {
        if (_.kind == PgCommandKind.Other) {
            return appendBytes(out, 0, _.raw);
        }
        let pos: usize = 0;
        pos = appendBytes(out, pos, _.commandName())?;
        if (_.kind == PgCommandKind.Insert) {
            pos = appendChar(out, pos, ' ')?;
            pos = appendU64(out, pos, _.oid)?;
        }
        pos = appendChar(out, pos, ' ')?;
        pos = appendU64(out, pos, _.rows)?;
        return pos;
    }
}
