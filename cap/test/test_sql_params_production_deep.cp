// Source: cp/cap/test/test_sql_params_production_deep.cp (Production deep tests for cap:sql_params)

import { test, expect_eq } from "cap:test";
import {
    countQuestionMarks,
    maxDollarParam,
    rewriteQuestionToDollar,
    NamedParamIter,
} from "cap:sql_params";

test("sql_params: countQuestionMarks ignores strings, identifiers, dollar-quotes and comments") {
    const sql =
        "SELECT 'what''s ?', \"col?\", `my?col`, $$dollar ? body$$, $tag$tagged ?$tag$ " ++
        "-- line comment ?\n" ++
        "/* block /* nested ? */ comment ? */ " ++
        "FROM users WHERE id = ? AND status = ? AND age > ?";

    expect_eq(countQuestionMarks(sql), 3);
    expect_eq(countQuestionMarks("SELECT 1"), 0);
}

test("sql_params: maxDollarParam finds highest $N and ignores strings, comments and dollar-quotes") {
    const sql =
        "SELECT '$99', $$ $88 $$, /* $77 */ $2, $1, $12 -- $100\n" ++
        "FROM items WHERE tag = $tag$ $55 $tag$ AND k = $3";

    expect_eq(maxDollarParam(sql), 12);
    expect_eq(maxDollarParam("SELECT $not_a_num, '$1'"), 0);
}

test("sql_params: rewriteQuestionToDollar preserves literals/comments and rewrites real ?") {
    const sql =
        "SELECT '?' AS mark, $$?$$ /* ? */ FROM t WHERE a = ? AND b = ? -- ?\n AND c = ?";
    let buf: [256]u8 = [0; 256];

    const pg = rewriteQuestionToDollar(sql, buf[0..]).?;
    const expected =
        "SELECT '?' AS mark, $$?$$ /* ? */ FROM t WHERE a = $1 AND b = $2 -- ?\n AND c = $3";
    expect_eq(std.mem.eql(u8, pg, expected), true);
}

test("sql_params: NamedParamIter extracts :name while skipping ::type casts and literals") {
    const sql =
        "SELECT id::int4, ':fake', $$:also_fake$$ /* :comment */ " ++
        "FROM accounts WHERE tenant = :tenant_id AND created_at > :since::timestamptz -- :ignored\n" ++
        "AND active = :is_active";

    let it = NamedParamIter.init(sql);
    expect_eq(std.mem.eql(u8, it.next().?, "tenant_id"), true);
    expect_eq(std.mem.eql(u8, it.next().?, "since"), true);
    expect_eq(std.mem.eql(u8, it.next().?, "is_active"), true);
    expect_eq(it.next() == null, true);
}

test("sql_params: unterminated quotes/comments and small output buffer overflow guards") {
    // 未闭合字符串/注释不应越界崩溃
    expect_eq(countQuestionMarks("SELECT 'unterminated ?"), 0);
    expect_eq(countQuestionMarks("SELECT /* unterminated ?"), 0);
    expect_eq(maxDollarParam("SELECT $$unterminated $99"), 0);

    // 过小输出缓冲区安全返回 null（10 个 ? 重写为 $1..$10 比原串更长）
    let tiny: [10]u8 = [0; 10];
    expect_eq(rewriteQuestionToDollar("?, ?, ?, ?, ?", tiny[0..]) == null, true);
}
