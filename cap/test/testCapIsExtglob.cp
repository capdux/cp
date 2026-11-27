import { test, expect_eq } from "cap:test";
import {
    isExtglob,
    findExtglobStart,
    countExtglobs,
} from "../../packages/is-extglob/src/index.cp";

fn main() {
    test("1. @cap/is-extglob — all five prefixes @(..), !(..), *(..), +(..), ?(..)", () => {
        expect_eq(isExtglob("@(foo|bar)"), true);
        expect_eq(isExtglob("!(foo|bar)"), true);
        expect_eq(isExtglob("*(foo|bar)"), true);
        expect_eq(isExtglob("+(foo|bar)"), true);
        expect_eq(isExtglob("?(foo|bar)"), true);
        expect_eq(isExtglob("src/@(a|b).cp"), true);
        expect_eq(isExtglob("!()"), true);
    });

    test("2. @cap/is-extglob — nested parentheses & inner extglobs", () => {
        expect_eq(isExtglob("+(a|!(b|c))"), true);
        expect_eq(isExtglob("!(foo/@(bar|baz))"), true);
        expect_eq(countExtglobs("+(a|!(b|c))"), 1 as usize);
        expect_eq(findExtglobStart("dir/+(a|!(b|c))"), 4 as i64);
    });

    test("3. @cap/is-extglob — backslash escapes ignored", () => {
        expect_eq(isExtglob("\\!(foo)"), false);
        expect_eq(isExtglob("\\@(a|b)"), false);
        expect_eq(isExtglob("!(foo\\)"), false);
        // Double backslash escapes the backslash itself, so !(foo) is active
        expect_eq(isExtglob("\\\\!(foo)"), true);
        expect_eq(findExtglobStart("\\\\!(foo)"), 2 as i64);
    });

    test("4. @cap/is-extglob — unclosed parens & plain paths return false", () => {
        expect_eq(isExtglob("!(foo"), false);
        expect_eq(isExtglob("@(a|!(b"), false);
        expect_eq(isExtglob("src/index.cp"), false);
        expect_eq(isExtglob("a/b/(foo)"), false);
        expect_eq(isExtglob("*.cp"), false);
        expect_eq(isExtglob("a?b+c!d@e"), false);
    });

    test("5. @cap/is-extglob — findExtglobStart & countExtglobs accuracy", () => {
        expect_eq(findExtglobStart("plain/path.txt"), -1 as i64);
        expect_eq(findExtglobStart("packages/@(a|b)/!(c).cp"), 9 as i64);
        expect_eq(countExtglobs("packages/@(a|b)/!(c).cp"), 2 as usize);
        expect_eq(countExtglobs("*(x)?(y)+(z)"), 3 as usize);
        expect_eq(countExtglobs("\\!(escaped)/+(active)"), 1 as usize);
        expect_eq(findExtglobStart("\\!(escaped)/+(active)"), 12 as i64);
    });

    test("6. @cap/is-extglob — empty & short string edge cases", () => {
        expect_eq(isExtglob(""), false);
        expect_eq(isExtglob("!"), false);
        expect_eq(isExtglob("!("), false);
        expect_eq(findExtglobStart(""), -1 as i64);
        expect_eq(countExtglobs(""), 0 as usize);
    });
}
