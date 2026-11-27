import { test, expect_eq } from "cap:test";
import {
    WildMatch, wildmatch, wildmatchIgnoreCase, wildmatchCustom, hasWildcards
} from "../../packages/wildmatch/src/index.cp";

fn main() {
    test("1. @cap/wildmatch — Exact literals and empty strings", () => {
        expect_eq(wildmatch("", ""), true);
        expect_eq(wildmatch("*", ""), true);
        expect_eq(wildmatch("?", ""), false);
        expect_eq(wildmatch("hello", "hello"), true);
        expect_eq(wildmatch("hello", "hell"), false);
        expect_eq(hasWildcards("plain.txt"), false);
        expect_eq(hasWildcards("*.txt"), true);
    });

    test("2. @cap/wildmatch — Star '*' and question '?' wildcards", () => {
        let m = WildMatch.new("*.cp");
        expect_eq(m.matches("index.cp"), true);
        expect_eq(m.matches("a/b/c.cp"), true);
        expect_eq(m.matches("index.rs"), false);
        expect_eq(wildmatch("f?o*bar", "foobazbar"), true);
        expect_eq(wildmatch("f?o*bar", "fobar"), false);
    });

    test("3. @cap/wildmatch — Multiple consecutive stars & anti-backtracking", () => {
        expect_eq(wildmatch("a***b***c", "axxxbyyyc"), true);
        expect_eq(wildmatch("*a*a*a*a*a*b", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaac"), false);
        expect_eq(wildmatch("*a*a*a*a*a*b", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaab"), true);
    });

    test("4. @cap/wildmatch — Case-insensitive matching via @cap/unicase", () => {
        let m = WildMatch.newIgnoreCase("README*.MD");
        expect_eq(m.matches("readme_cn.md"), true);
        expect_eq(m.matches("README.md"), true);
        expect_eq(wildmatchIgnoreCase("HTTP://*.EXAMPLE.COM", "http://api.example.com"), true);
        expect_eq(wildmatch("README.MD", "readme.md"), false);
    });

    test("5. @cap/wildmatch — Custom wildcards (SQL LIKE '%' and '_')", () => {
        let sql = WildMatch.newWithWildcards("user_%_202_", 37, 95); // '%' = 37, '_' = 95
        expect_eq(sql.matches("user_admin_2026"), true);
        expect_eq(sql.matches("user_admin_20260"), false);
        expect_eq(wildmatchCustom("%world%", "hello world!", 37, 95), true);
    });

    test("6. @cap/wildmatch — Backslash escaping of literal wildcards", () => {
        let m = WildMatch.newWithEscape("file\\*\\?.txt", false);
        expect_eq(m.matches("file*?.txt"), true);
        expect_eq(m.matches("fileab.txt"), false);
        let m2 = WildMatch.newWithEscape("LOG\\_*.TXT", true);
        expect_eq(m2.matches("log_2026.txt"), true);
    });
}
