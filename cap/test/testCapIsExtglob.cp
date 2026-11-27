import { test, expect_eq } from "cap:test";
import {
    ExtglobKind,
    isExtglobPrefix,
    extglobKindFromByte,
    extglobPrefixByte,
    findExtglob,
    findExtglobFrom,
    findExtglobStart,
    isExtglob,
    hasNestedExtglob,
    isNegatedExtglob,
    countExtglobs,
} from "../../packages/is-extglob/src/index.cp";

fn main() {
    test("1. is-extglob — all five prefixes @(..), !(..), *(..), +(..), ?(..)", () => {
        expect_eq(isExtglob("@(foo|bar)"), true);
        expect_eq(isExtglob("!(foo|bar)"), true);
        expect_eq(isExtglob("*(foo|bar)"), true);
        expect_eq(isExtglob("+(foo|bar)"), true);
        expect_eq(isExtglob("?(foo|bar)"), true);
        expect_eq(isExtglob("src/@(a|b).cp"), true);
        expect_eq(isExtglob("!()"), true);
    });

    test("2. is-extglob — nested parentheses & inner extglobs", () => {
        expect_eq(isExtglob("+(a|!(b|c))"), true);
        expect_eq(isExtglob("!(foo/@(bar|baz))"), true);
        expect_eq(countExtglobs("+(a|!(b|c))"), 1 as usize);
        expect_eq(findExtglobStart("dir/+(a|!(b|c))"), 4 as i64);
        expect_eq(hasNestedExtglob("+(a|!(b|c))"), true);
        expect_eq(hasNestedExtglob("+(a|b)"), false);
    });

    test("3. is-extglob — backslash escapes ignored", () => {
        expect_eq(isExtglob("\\!(foo)"), false);
        expect_eq(isExtglob("\\@(a|b)"), false);
        expect_eq(isExtglob("!(foo\\)"), false);
        // Double backslash escapes the backslash itself, so !(foo) is active
        expect_eq(isExtglob("\\\\!(foo)"), true);
        expect_eq(findExtglobStart("\\\\!(foo)"), 2 as i64);
    });

    test("4. is-extglob — unclosed parens & plain paths return false", () => {
        expect_eq(isExtglob("!(foo"), false);
        expect_eq(isExtglob("@(a|!(b"), false);
        expect_eq(isExtglob("src/index.cp"), false);
        expect_eq(isExtglob("a/b/(foo)"), false);
        expect_eq(isExtglob("*.cp"), false);
        expect_eq(isExtglob("a?b+c!d@e"), false);
    });

    test("5. is-extglob — findExtglobStart & countExtglobs accuracy", () => {
        expect_eq(findExtglobStart("plain/path.txt"), -1 as i64);
        expect_eq(findExtglobStart("packages/@(a|b)/!(c).cp"), 9 as i64);
        expect_eq(countExtglobs("packages/@(a|b)/!(c).cp"), 2 as usize);
        expect_eq(countExtglobs("*(x)?(y)+(z)"), 3 as usize);
        expect_eq(countExtglobs("\\!(escaped)/+(active)"), 1 as usize);
        expect_eq(findExtglobStart("\\!(escaped)/+(active)"), 12 as i64);
    });

    test("6. is-extglob — empty & short string edge cases", () => {
        expect_eq(isExtglob(""), false);
        expect_eq(isExtglob("!"), false);
        expect_eq(isExtglob("!("), false);
        expect_eq(findExtglobStart(""), -1 as i64);
        expect_eq(countExtglobs(""), 0 as usize);
        expect_eq(findExtglob("") == null, true);
    });

    test("7. is-extglob — findExtglob & findExtglobFrom return exact spans and kinds", () => {
        const pat = "packages/@(a|b)/!(c|d).cp";
        const first = findExtglob(pat);
        expect_eq(first != null, true);
        if (s1 := first) {
            expect_eq(s1.kind == .ExactlyOne, true);
            expect_eq(s1.start, 9 as usize);
            expect_eq(s1.end, 15 as usize);
            expect_eq(pat[s1.start..s1.end], "@(a|b)");

            const second = findExtglobFrom(pat, s1.end);
            expect_eq(second != null, true);
            if (s2 := second) {
                expect_eq(s2.kind == .Negated, true);
                expect_eq(s2.start, 16 as usize);
                expect_eq(s2.end, 22 as usize);
                expect_eq(pat[s2.start..s2.end], "!(c|d)");
                expect_eq(findExtglobFrom(pat, s2.end) == null, true);
            }
        }
    });

    test("8. is-extglob — kind helpers & isNegatedExtglob", () => {
        expect_eq(isExtglobPrefix('?'), true);
        expect_eq(isExtglobPrefix('a'), false);
        expect_eq(extglobKindFromByte('?') == .Optional, true);
        expect_eq(extglobKindFromByte('*') == .ZeroOrMore, true);
        expect_eq(extglobKindFromByte('+') == .OneOrMore, true);
        expect_eq(extglobKindFromByte('@') == .ExactlyOne, true);
        expect_eq(extglobKindFromByte('!') == .Negated, true);
        expect_eq(extglobKindFromByte('x') == null, true);
        expect_eq(extglobPrefixByte(.Negated), 33 as u8);

        expect_eq(isNegatedExtglob("!(*.log)"), true);
        expect_eq(isNegatedExtglob("+(a|!(b))"), true);
        expect_eq(isNegatedExtglob("@(a|b)"), false);
    });
}
