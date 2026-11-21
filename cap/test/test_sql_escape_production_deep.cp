// Source: cp/cap/test/test_sql_escape_production_deep.cp (Production deep tests for cap:sql_escape)

import { test, expect_eq } from "cap:test";
import {
    escapeAnsiIdent,
    escapeMysqlIdent,
    escapeQualifiedAnsiIdent,
    escapeSqlStringAnsi,
    escapeSqlStringMysql,
    escapeLikePattern,
} from "cap:sql";

test("sql_escape: ANSI and MySQL identifier quoting with internal quote doubling and NUL rejection") {
    let buf: [128]u8 = [0; 128];

    const a1 = escapeAnsiIdent("users", buf[0..]).?;
    expect_eq(std.mem.eql(u8, a1, "\"users\""), true);

    const a2 = escapeAnsiIdent("weird\"col\"name", buf[0..]).?;
    expect_eq(std.mem.eql(u8, a2, "\"weird\"\"col\"\"name\""), true);

    const m1 = escapeMysqlIdent("orders", buf[0..]).?;
    expect_eq(std.mem.eql(u8, m1, "`orders`"), true);

    const m2 = escapeMysqlIdent("my`table`1", buf[0..]).?;
    expect_eq(std.mem.eql(u8, m2, "`my``table``1`"), true);

    // 拒绝空标识符与含 NUL 字节的异常输入
    expect_eq(escapeAnsiIdent("", buf[0..]) == null, true);
    expect_eq(escapeAnsiIdent("bad\x00col", buf[0..]) == null, true);
    expect_eq(escapeMysqlIdent("", buf[0..]) == null, true);
    expect_eq(escapeMysqlIdent("bad\x00tbl", buf[0..]) == null, true);
}

test("sql_escape: escapeQualifiedAnsiIdent schema.table splitting and empty-segment guards") {
    let buf: [128]u8 = [0; 128];

    const q1 = escapeQualifiedAnsiIdent("public.users", buf[0..]).?;
    expect_eq(std.mem.eql(u8, q1, "\"public\".\"users\""), true);

    const q2 = escapeQualifiedAnsiIdent("cat.my\"schema.tbl", buf[0..]).?;
    expect_eq(std.mem.eql(u8, q2, "\"cat\".\"my\"\"schema\".\"tbl\""), true);

    const q3 = escapeQualifiedAnsiIdent("single", buf[0..]).?;
    expect_eq(std.mem.eql(u8, q3, "\"single\""), true);

    // 空串、前导/尾随点、双点空段与 NUL 均拒绝
    expect_eq(escapeQualifiedAnsiIdent("", buf[0..]) == null, true);
    expect_eq(escapeQualifiedAnsiIdent(".users", buf[0..]) == null, true);
    expect_eq(escapeQualifiedAnsiIdent("public.", buf[0..]) == null, true);
    expect_eq(escapeQualifiedAnsiIdent("public..users", buf[0..]) == null, true);
    expect_eq(escapeQualifiedAnsiIdent("public.us\x00ers", buf[0..]) == null, true);
}

test("sql_escape: escapeSqlStringAnsi single-quote doubling and NUL guard") {
    let buf: [128]u8 = [0; 128];

    expect_eq(std.mem.eql(u8, escapeSqlStringAnsi("", buf[0..]).?, "''"), true);
    expect_eq(std.mem.eql(u8, escapeSqlStringAnsi("O'Reilly", buf[0..]).?, "'O''Reilly'"), true);
    expect_eq(std.mem.eql(u8, escapeSqlStringAnsi("''", buf[0..]).?, "''''''"), true);
    expect_eq(escapeSqlStringAnsi("abc\x00def", buf[0..]) == null, true);
}

test("sql_escape: escapeSqlStringMysql escapes quotes, backslashes, newlines, NUL and Ctrl+Z") {
    let buf: [128]u8 = [0; 128];

    const raw = "a'b\"c\\d\ne\rf\x00g\x1Ah";
    const escaped = escapeSqlStringMysql(raw, buf[0..]).?;
    expect_eq(std.mem.eql(u8, escaped, "'a\\'b\\\"c\\\\d\\ne\\rf\\0g\\Zh'"), true);
}

test("sql_escape: escapeLikePattern wildcards and small output buffer guards") {
    let buf: [128]u8 = [0; 128];

    const p1 = escapeLikePattern("100%_pure\\match", '\\', buf[0..]).?;
    expect_eq(std.mem.eql(u8, p1, "100\\%\\_pure\\\\match"), true);

    const p2 = escapeLikePattern("a!b%c_d", '!', buf[0..]).?;
    expect_eq(std.mem.eql(u8, p2, "a!!b!%c!_d"), true);

    expect_eq(escapeLikePattern("abc", 0, buf[0..]) == null, true);

    // 缓冲区容量不足防护
    let tiny: [4]u8 = [0; 4];
    expect_eq(escapeAnsiIdent("users", tiny[0..]) == null, true);
    expect_eq(escapeMysqlIdent("users", tiny[0..]) == null, true);
    expect_eq(escapeSqlStringAnsi("hello", tiny[0..]) == null, true);
    expect_eq(escapeSqlStringMysql("a'b'c", tiny[0..]) == null, true);
    expect_eq(escapeLikePattern("a%b%c", '\\', tiny[0..]) == null, true);
}
