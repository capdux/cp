import { test, expect_eq } from "cap:test";
import {
    escapeStringAnsi,
    escapeIdentifierAnsi,
    escapeIdentifierMysql,
    escapeLike,
    isSafeUnquotedIdent,
    countPlaceholders,
} from "../../packages/@cap/sql-escape/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
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

fn main() {
    test("1. @cap/sql-escape — escapeStringAnsi single-quote doubling & NUL rejection", () => {
        let buf: [128]u8 = undefined;
        const n1 = escapeStringAnsi("O'Reilly's Book", buf[0..128]);
        expect_eq(testEq(buf[0..n1], "'O''Reilly''s Book'"), true);

        const n_empty = escapeStringAnsi("", buf[0..128]);
        expect_eq(testEq(buf[0..n_empty], "''"), true);

        const with_nul: [4]u8 = [97, 98, 0, 99];
        expect_eq(escapeStringAnsi(with_nul[0..4], buf[0..128]), 0 as usize);
    });

    test("2. @cap/sql-escape — escapeIdentifierAnsi double-quote doubling", () => {
        let buf: [64]u8 = undefined;
        const n1 = escapeIdentifierAnsi("user\"name", buf[0..64]);
        expect_eq(testEq(buf[0..n1], "\"user\"\"name\""), true);

        expect_eq(escapeIdentifierAnsi("", buf[0..64]), 0 as usize);
    });

    test("3. @cap/sql-escape — escapeIdentifierMysql backtick doubling", () => {
        let buf: [64]u8 = undefined;
        const n1 = escapeIdentifierMysql("my`table", buf[0..64]);
        expect_eq(testEq(buf[0..n1], "`my``table`"), true);

        expect_eq(escapeIdentifierMysql("", buf[0..64]), 0 as usize);
    });

    test("4. @cap/sql-escape — escapeLike wildcards %, _ & escape char itself", () => {
        let buf: [64]u8 = undefined;
        const n = escapeLike("100%_off\\deal", 92, buf[0..64]); // '\'
        expect_eq(testEq(buf[0..n], "100\\%\\_off\\\\deal"), true);

        expect_eq(escapeLike("abc", 0, buf[0..64]), 0 as usize);
    });

    test("5. @cap/sql-escape — isSafeUnquotedIdent validation", () => {
        expect_eq(isSafeUnquotedIdent("users"), true);
        expect_eq(isSafeUnquotedIdent("_order_items_2026"), true);
        expect_eq(isSafeUnquotedIdent(""), false);
        expect_eq(isSafeUnquotedIdent("1starts_with_digit"), false);
        expect_eq(isSafeUnquotedIdent("has-hyphen"), false);
        expect_eq(isSafeUnquotedIdent("has space"), false);
        expect_eq(isSafeUnquotedIdent("drop;table"), false);
    });

    test("6. @cap/sql-escape — countPlaceholders ignores quoted '?' & undersized buffer guard", () => {
        const sql = "SELECT \"col?\" FROM users WHERE bio = 'hello ''?'' world?' AND id = ? AND status = ?";
        expect_eq(countPlaceholders(sql), 2 as usize);
        expect_eq(countPlaceholders("SELECT '?'"), 0 as usize);

        let tiny: [4]u8 = undefined;
        expect_eq(escapeStringAnsi("abcdef", tiny[0..4]), 0 as usize);
        expect_eq(escapeIdentifierAnsi("abcdef", tiny[0..4]), 0 as usize);
        expect_eq(escapeLike("a%b_c", 92, tiny[0..4]), 0 as usize);
    });
}
