// cap/test/testCapSqlformat.cp
// Comprehensive unit tests for @cap/sqlformat

import { test, expect_eq } from "cap:test";
import {
    SqlFormatOptions,
    formatSql,
    formatSqlWith,
    minifySql,
} from "../../packages/@cap/sqlformat/src/index.cp";

fn containsSub(hay: string, needle: string) -> bool {
    if (needle.len > hay.len) {
        return false;
    }
    let i: usize = 0;
    while (i + needle.len <= hay.len) {
        let j: usize = 0;
        let ok = true;
        while (j < needle.len) {
            if (hay[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn main() {
    test("1. @cap/sqlformat — complex SELECT / JOIN / WHERE / GROUP BY / ORDER BY / LIMIT", () => {
        const raw = "select u.id, u.name, count(o.id) as order_count from users u left join orders o on u.id = o.user_id where u.active = 1 and o.total >= 100 group by u.id, u.name order by order_count desc limit 10;";
        let buf: [2048]u8 = undefined;
        const formatted = formatSql(raw, buf[0..2048]);

        const expected = "SELECT\n  u.id,\n  u.name,\n  COUNT(o.id) AS order_count\nFROM users u\nLEFT JOIN orders o ON u.id = o.user_id\nWHERE u.active = 1\n  AND o.total >= 100\nGROUP BY u.id, u.name\nORDER BY order_count DESC\nLIMIT 10;";
        expect_eq(formatted, expected);
    });

    test("2. @cap/sqlformat — INSERT / UPDATE / DELETE statements", () => {
        let buf1: [1024]u8 = undefined;
        const ins = formatSql("insert into users (id, name) values (1, 'Alice');", buf1[0..1024]);
        expect_eq(ins, "INSERT INTO users (id, name)\nVALUES (1, 'Alice');");

        let buf2: [1024]u8 = undefined;
        const upd = formatSql("update users set name = 'Bob', active = 0 where id = 1 and deleted_at is null;", buf2[0..1024]);
        expect_eq(upd, "UPDATE users\nSET name = 'Bob',\n  active = 0\nWHERE id = 1\n  AND deleted_at IS NULL;");

        let buf3: [1024]u8 = undefined;
        const del = formatSql("delete from sessions where expired_at < 1000 or revoked = 1;", buf3[0..1024]);
        expect_eq(del, "DELETE FROM sessions\nWHERE expired_at < 1000\n  OR revoked = 1;");
    });

    test("3. @cap/sqlformat — preserve lowercase keywords & escaped quotes inside string literals", () => {
        const raw = "select 'select from where and it''s ok' as msg, \"from\" from `select_tbl` where note = 'keep   spaces';";
        let buf: [1024]u8 = undefined;
        const formatted = formatSql(raw, buf[0..1024]);

        expect_eq(containsSub(formatted, "'select from where and it''s ok'"), true);
        expect_eq(containsSub(formatted, "\"from\""), true);
        expect_eq(containsSub(formatted, "`select_tbl`"), true);
        expect_eq(containsSub(formatted, "'keep   spaces'"), true);
        expect_eq(containsSub(formatted, "SELECT\n"), true);
        expect_eq(containsSub(formatted, "\nFROM `select_tbl`"), true);
    });

    test("4. @cap/sqlformat — preserve line comments & block comments verbatim", () => {
        const raw = "-- keep lowercase select in line comment\nselect /* keep where in block */ id from users where active = 1;";
        let buf: [1024]u8 = undefined;
        const formatted = formatSql(raw, buf[0..1024]);

        expect_eq(containsSub(formatted, "-- keep lowercase select in line comment"), true);
        expect_eq(containsSub(formatted, "/* keep where in block */"), true);
        expect_eq(containsSub(formatted, "SELECT"), true);
        expect_eq(containsSub(formatted, "FROM users"), true);
    });

    test("5. @cap/sqlformat — multi-statement separation & custom SqlFormatOptions", () => {
        let opts = SqlFormatOptions.defaults();
        opts.indent_width = 4;
        opts.uppercase_keywords = false;
        opts.lines_between_queries = 1;

        const raw = "select a, b from t1; select c from t2;";
        let buf: [1024]u8 = undefined;
        const formatted = formatSqlWith(raw, opts, buf[0..1024]);

        expect_eq(formatted, "select\n    a,\n    b\nfrom t1;\n\nselect\n    c\nfrom t2;");
    });

    test("6. @cap/sqlformat — minifySql strips comments & extra whitespace while keeping literals", () => {
        const raw = "-- header comment\nSELECT   u.id,   u.name  /* inline comment */\nFROM   users   u\nWHERE  u.bio = 'hello   from   cp'  AND  u.active = 1;";
        let buf: [1024]u8 = undefined;
        const minified = minifySql(raw, buf[0..1024]);

        expect_eq(containsSub(minified, "--"), false);
        expect_eq(containsSub(minified, "/*"), false);
        expect_eq(containsSub(minified, "\n"), false);
        expect_eq(containsSub(minified, "'hello   from   cp'"), true);
        expect_eq(minified, "SELECT u.id,u.name FROM users u WHERE u.bio = 'hello   from   cp' AND u.active = 1;");
    });
}
