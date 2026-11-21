// Source: cp/cap/test/test_sql_splitter_production_deep.cp (Production deep tests for cap:sql_splitter)

import { test, expect_eq } from "cap:test";
import {
    SqlSplitStmtKind,
    SqlStmtIter,
    classifySqlStmt,
    isReadOnlySql,
    countSqlStatements,
} from "cap:sql";

test("sql_splitter: splits multi-statement script while protecting semicolons in literals/comments/dollar-quotes") {
    const script =
        "\xEF\xBB\xBF" ++
        "-- migration header;\n" ++
        "CREATE TABLE users (id INT, note TEXT DEFAULT 'a;b''c;d');\n" ++
        "CREATE FUNCTION f() RETURNS INT AS $$ BEGIN RETURN 1; END; $$ LANGUAGE plpgsql;\n" ++
        "/* block /* nested ; */ comment ; */\n" ++
        "INSERT INTO \"t;1\" VALUES (1, `x;y`);;\n" ++
        "-- trailing comment only;\n";

    expect_eq(countSqlStatements(script), 3);

    let it = SqlStmtIter.init(script);

    const s1 = it.next().?;
    expect_eq(classifySqlStmt(s1) == .create, true);

    const s2 = it.next().?;
    expect_eq(classifySqlStmt(s2) == .create, true);

    const s3 = it.next().?;
    expect_eq(classifySqlStmt(s3) == .insert, true);

    expect_eq(it.next() == null, true);
}

test("sql_splitter: classifySqlStmt covers all statement kinds and leading comments") {
    expect_eq(classifySqlStmt("/* hdr */ -- c\n select * from t") == .select, true);
    expect_eq(classifySqlStmt("INSERT INTO t VALUES (1)") == .insert, true);
    expect_eq(classifySqlStmt("UPDATE t SET a = 1") == .update, true);
    expect_eq(classifySqlStmt("DELETE FROM t") == .delete, true);
    expect_eq(classifySqlStmt("CREATE INDEX i ON t(a)") == .create, true);
    expect_eq(classifySqlStmt("DROP TABLE t") == .drop, true);
    expect_eq(classifySqlStmt("ALTER TABLE t ADD c INT") == .alter, true);
    expect_eq(classifySqlStmt("BEGIN TRANSACTION") == .begin, true);
    expect_eq(classifySqlStmt("COMMIT") == .commit, true);
    expect_eq(classifySqlStmt("ROLLBACK") == .rollback, true);
    expect_eq(classifySqlStmt("PRAGMA journal_mode=WAL") == .pragma, true);
    expect_eq(classifySqlStmt("VACUUM") == .other, true);
    expect_eq(std.mem.eql(u8, SqlSplitStmtKind.pragma.asStr(), "pragma"), true);
}

test("sql_splitter: WITH CTE classification distinguishes read-only SELECT vs mutating DELETE/INSERT") {
    const cte_sel = "WITH active AS (SELECT id FROM users WHERE ok = 1) SELECT * FROM active";
    expect_eq(classifySqlStmt(cte_sel) == .select, true);
    expect_eq(isReadOnlySql(cte_sel), true);

    const cte_del = "WITH old_rows AS (SELECT id FROM logs) DELETE FROM logs WHERE id IN (SELECT id FROM old_rows)";
    expect_eq(classifySqlStmt(cte_del) == .delete, true);
    expect_eq(isReadOnlySql(cte_del), false);

    const cte_ins = "WITH src AS (SELECT 1 AS x) INSERT INTO dst SELECT x FROM src";
    expect_eq(classifySqlStmt(cte_ins) == .insert, true);
    expect_eq(isReadOnlySql(cte_ins), false);
}

test("sql_splitter: isReadOnlySql identifies SELECT, EXPLAIN, SHOW and rejects mutations") {
    expect_eq(isReadOnlySql("  -- query\nSELECT 1"), true);
    expect_eq(isReadOnlySql("EXPLAIN ANALYZE SELECT * FROM t"), true);
    expect_eq(isReadOnlySql("SHOW TABLES"), true);

    expect_eq(isReadOnlySql("INSERT INTO t VALUES (1)"), false);
    expect_eq(isReadOnlySql("UPDATE t SET x = 2"), false);
    expect_eq(isReadOnlySql("DELETE FROM t"), false);
    expect_eq(isReadOnlySql("DROP TABLE t"), false);
    expect_eq(isReadOnlySql(""), false);
}

test("sql_splitter: empty scripts, semicolons-only and no-trailing-semicolon statement") {
    expect_eq(countSqlStatements(""), 0);
    expect_eq(countSqlStatements("  ; ; -- comment\n /* block */ ; "), 0);

    let it = SqlStmtIter.init("SELECT 1; SELECT 2");
    expect_eq(std.mem.eql(u8, it.next().?, "SELECT 1"), true);
    expect_eq(std.mem.eql(u8, it.next().?, "SELECT 2"), true);
    expect_eq(it.next() == null, true);
}
