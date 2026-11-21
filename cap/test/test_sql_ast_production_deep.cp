// Source: cp/cap/test/test_sql_ast_production_deep.cp (Deep Production Tests for cap:sql_ast)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SqlError,
    SqlTokenKind,
    SqlExprKind,
    SqlStmtKind,
    tokenizeSql,
    parseSql,
    parseSqlWithMaxDepth,
    normalizeSqlFingerprint,
    extractTableNames,
    isReadOnlyQuery,
} from "cap:sql";

test("sql_ast: lexer tokens, comments, escaped strings, placeholders and error guards") {
    const alloc = std.testing.allocator;
    const sql = "-- line comment\nSELECT 'O''Reilly', \"col_a\", `tbl_b`, 42, 3.14, ?, $1 /* block */ FROM users WHERE a <> b AND c != d AND e <= 10 AND f >= 20 AND x || y :: text;";
    const tokens = try tokenizeSql(alloc, sql);
    defer alloc.free(tokens);

    try expect_eq(tokens[0].kind == SqlTokenKind.Keyword, true);
    try expect_eq(std.mem.eql(u8, tokens[0].lexeme, "SELECT"), true);
    try expect_eq(tokens[1].kind == SqlTokenKind.StringLiteral, true);
    try expect_eq(std.mem.eql(u8, tokens[1].lexeme, "'O''Reilly'"), true);
    try expect_eq(tokens[3].kind == SqlTokenKind.QuotedIdentifier, true);
    try expect_eq(tokens[5].kind == SqlTokenKind.QuotedIdentifier, true);
    try expect_eq(tokens[7].kind == SqlTokenKind.NumberLiteral, true);
    try expect_eq(tokens[9].kind == SqlTokenKind.NumberLiteral, true);
    try expect_eq(tokens[11].kind == SqlTokenKind.Placeholder, true);
    try expect_eq(tokens[13].kind == SqlTokenKind.Placeholder, true);
    try expect_eq(std.mem.eql(u8, tokens[13].lexeme, "$1"), true);

    // Error guards: unterminated string, unterminated comment, invalid char
    let got_unterm_str = false;
    * = tokenizeSql(alloc, "SELECT 'abc") catch |e| {
        if (e == SqlError.UnterminatedString) {
            got_unterm_str = true;
        }
    };
    try expect_eq(got_unterm_str, true);

    let got_unterm_cmt = false;
    * = tokenizeSql(alloc, "SELECT /* unclosed") catch |e| {
        if (e == SqlError.UnterminatedComment) {
            got_unterm_cmt = true;
        }
    };
    try expect_eq(got_unterm_cmt, true);

    let got_invalid_char = false;
    * = tokenizeSql(alloc, "SELECT $") catch |e| {
        if (e == SqlError.InvalidCharacter) {
            got_invalid_char = true;
        }
    };
    try expect_eq(got_invalid_char, true);
}

test("sql_ast: complex SELECT AST with JOIN, WHERE precedence, GROUP BY, ORDER BY, LIMIT/OFFSET") {
    const alloc = std.testing.allocator;
    const sql = "SELECT DISTINCT u.id, u.name AS user_name, COUNT(o.id) FROM users u LEFT JOIN orders o ON u.id = o.user_id WHERE (u.age >= 18 AND u.status = 'active') OR u.role = 'admin' GROUP BY u.id, u.name ORDER BY u.id DESC, o.id ASC LIMIT 10 OFFSET 20";

    let stmt = try parseSql(alloc, sql);
    defer stmt.deinit(alloc);

    try expect_eq(stmt.kind == SqlStmtKind.Select, true);
    const sel = stmt.select_stmt.?;
    try expect_eq(sel.distinct, true);
    try expect_eq(sel.columns.len, 3);
    try expect_eq(std.mem.eql(u8, sel.columns[0].expr.qualifier.?, "u"), true);
    try expect_eq(std.mem.eql(u8, sel.columns[0].expr.text, "id"), true);
    try expect_eq(std.mem.eql(u8, sel.columns[1].alias.?, "user_name"), true);
    try expect_eq(sel.columns[2].expr.kind == SqlExprKind.FuncCall, true);
    try expect_eq(std.mem.eql(u8, sel.from_table.?, "users"), true);
    try expect_eq(std.mem.eql(u8, sel.from_alias.?, "u"), true);
    try expect_eq(sel.joins.len, 1);
    try expect_eq(std.mem.eql(u8, sel.joins[0].join_type, "LEFT"), true);
    try expect_eq(std.mem.eql(u8, sel.joins[0].table_name, "orders"), true);

    // WHERE precedence: OR at root
    const where_e = sel.where_expr.?;
    try expect_eq(where_e.kind == SqlExprKind.BinaryOp, true);
    try expect_eq(std.mem.eql(u8, where_e.op, "OR"), true);
    try expect_eq(where_e.left.?.kind == SqlExprKind.Paren, true);
    try expect_eq(std.mem.eql(u8, where_e.left.?.left.?.op, "AND"), true);

    try expect_eq(sel.group_by.len, 2);
    try expect_eq(sel.order_by.len, 2);
    try expect_eq(sel.order_by[0].desc, true);
    try expect_eq(sel.order_by[1].desc, false);
    try expect_eq(std.mem.eql(u8, sel.limit.?.text, "10"), true);
    try expect_eq(std.mem.eql(u8, sel.offset.?.text, "20"), true);

    // Arithmetic precedence: 1 + 2 * 3 => (+ 1 (* 2 3))
    let arith_stmt = try parseSql(alloc, "SELECT 1 + 2 * 3");
    defer arith_stmt.deinit(alloc);
    const expr = arith_stmt.select_stmt.?.columns[0].expr;
    try expect_eq(std.mem.eql(u8, expr.op, "+"), true);
    try expect_eq(std.mem.eql(u8, expr.right.?.op, "*"), true);
}

test("sql_ast: INSERT, UPDATE, DELETE, CREATE TABLE and max_depth overflow guard") {
    const alloc = std.testing.allocator;

    let ins_stmt = try parseSql(alloc, "INSERT INTO users (id, name) VALUES (1, 'Alice'), (2, 'Bob')");
    defer ins_stmt.deinit(alloc);
    try expect_eq(ins_stmt.kind == SqlStmtKind.Insert, true);
    try expect_eq(std.mem.eql(u8, ins_stmt.insert_stmt.?.table_name, "users"), true);
    try expect_eq(ins_stmt.insert_stmt.?.columns.len, 2);
    try expect_eq(ins_stmt.insert_stmt.?.rows.len, 2);

    let upd_stmt = try parseSql(alloc, "UPDATE users SET name = 'Charlie', age = 30 WHERE id = $1");
    defer upd_stmt.deinit(alloc);
    try expect_eq(upd_stmt.kind == SqlStmtKind.Update, true);
    try expect_eq(upd_stmt.update_stmt.?.assignments.len, 2);
    try expect_eq(upd_stmt.update_stmt.?.where_expr != null, true);

    let del_stmt = try parseSql(alloc, "DELETE FROM users WHERE id IN (1, 2, 3) AND deleted_at IS NOT NULL");
    defer del_stmt.deinit(alloc);
    try expect_eq(del_stmt.kind == SqlStmtKind.Delete, true);
    try expect_eq(std.mem.eql(u8, del_stmt.delete_stmt.?.table_name, "users"), true);

    let ct_stmt = try parseSql(alloc, "CREATE TABLE IF NOT EXISTS users (id INT PRIMARY KEY NOT NULL, name VARCHAR(64) NOT NULL)");
    defer ct_stmt.deinit(alloc);
    try expect_eq(ct_stmt.kind == SqlStmtKind.CreateTable, true);
    try expect_eq(ct_stmt.create_table_stmt.?.if_not_exists, true);
    try expect_eq(ct_stmt.create_table_stmt.?.columns.len, 2);
    try expect_eq(ct_stmt.create_table_stmt.?.columns[0].primary_key, true);
    try expect_eq(ct_stmt.create_table_stmt.?.columns[0].not_null, true);

    // Recursion depth guard (max_depth = 32)
    const deep_sql = "SELECT (((((((((((((((((((((((((((((((((1)))))))))))))))))))))))))))))))))";
    let got_depth_err = false;
    * = parseSql(alloc, deep_sql) catch |e| {
        if (e == SqlError.MaxDepthExceeded) {
            got_depth_err = true;
        }
    };
    try expect_eq(got_depth_err, true);

    // Custom depth limit
    let custom_depth_err = false;
    * = parseSqlWithMaxDepth(alloc, "SELECT ((1))", 2) catch |e| {
        if (e == SqlError.MaxDepthExceeded) {
            custom_depth_err = true;
        }
    };
    try expect_eq(custom_depth_err, true);
}

test("sql_ast: normalizeSqlFingerprint, extractTableNames and isReadOnlyQuery injection guard") {
    const alloc = std.testing.allocator;

    const raw = "select  id, name  from users where id = 123 and name = 'Alice' and score > 9.5 -- secret";
    const fp = try normalizeSqlFingerprint(alloc, raw);
    defer alloc.free(fp);
    try expect_eq(std.mem.eql(u8, fp, "SELECT id, name FROM users WHERE id = ? AND name = ? AND score > ?"), true);

    const join_sql = "SELECT * FROM users u JOIN orders o ON u.id = o.user_id JOIN users u2 ON u.id = u2.id";
    const tables = try extractTableNames(alloc, join_sql);
    defer alloc.free(tables);
    try expect_eq(tables.len, 2);
    try expect_eq(std.mem.eql(u8, tables[0], "users"), true);
    try expect_eq(std.mem.eql(u8, tables[1], "orders"), true);

    try expect_eq(isReadOnlyQuery("SELECT id, name FROM users WHERE id = 1;"), true);
    try expect_eq(isReadOnlyQuery("/* hint */ SELECT 'a;b' FROM users"), true);
    try expect_eq(isReadOnlyQuery("SELECT 1; DROP TABLE users;"), false);
    try expect_eq(isReadOnlyQuery("INSERT INTO users VALUES (1)"), false);
    try expect_eq(isReadOnlyQuery("DELETE FROM users"), false);
    try expect_eq(isReadOnlyQuery("SELECT 'unterminated"), false);
}

test("sql_ast: cap.sql_ast dynamic mount and std.testing.allocator zero-leak verification") {
    const alloc = std.testing.allocator;

    try expect_eq(cap.sql_ast.isReadOnlyQuery("SELECT 1"), true);
    const toks = try cap.sql_ast.tokenizeSql(alloc, "SELECT $1");
    defer alloc.free(toks);
    try expect_eq(toks.len, 3);

    let stmt = try cap.sql_ast.parseSql(alloc, "SELECT id FROM items WHERE id = ?");
    defer stmt.deinit(alloc);
    try expect_eq(stmt.kind == SqlStmtKind.Select, true);

    const fp = try cap.sql_ast.normalizeSqlFingerprint(alloc, "SELECT id FROM items WHERE id = 99");
    defer alloc.free(fp);
    try expect_eq(std.mem.eql(u8, fp, "SELECT id FROM items WHERE id = ?"), true);

    const tbls = try cap.sql_ast.extractTableNames(alloc, "SELECT id FROM items");
    defer alloc.free(tbls);
    try expect_eq(tbls.len, 1);
}
