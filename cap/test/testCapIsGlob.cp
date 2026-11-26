import { test, expect_eq } from "cap:test";
import {
    isGlob,
    isGlobDefault,
    hasUnescapedWildcard,
} from "../../packages/@cap/is-glob/src/index.cp";

fn main() {
    test("1. @cap/is-glob — wildcards *, **, ? & leading negation !", () => {
        expect_eq(isGlobDefault("*.cp"), true);
        expect_eq(isGlobDefault("src/**/*.cp"), true);
        expect_eq(isGlobDefault("file?.txt"), true);
        expect_eq(isGlobDefault("!ignore.log"), true);
        expect_eq(hasUnescapedWildcard("src/*.cp"), true);
        expect_eq(hasUnescapedWildcard("src/?.cp"), true);
        expect_eq(hasUnescapedWildcard("src/index.cp"), false);
    });

    test("2. @cap/is-glob — paired brackets [...] & braces {...}", () => {
        expect_eq(isGlobDefault("[a-z].cp"), true);
        expect_eq(isGlobDefault("src/{a,b}.cp"), true);
        expect_eq(isGlobDefault("file-{1..5}.txt"), true);
        expect_eq(isGlobDefault("[]"), false);
        expect_eq(isGlobDefault("{}"), false);
    });

    test("3. @cap/is-glob — reuses @cap/is-extglob for !(..), @(..), +(..)", () => {
        expect_eq(isGlobDefault("!(foo|bar)"), true);
        expect_eq(isGlobDefault("src/@(a|b).cp"), true);
        expect_eq(isGlobDefault("+(foo|*(bar))"), true);
        expect_eq(isGlobDefault("?(opt).js"), true);
    });

    test("4. @cap/is-glob — strict=true vs strict=false on unclosed brackets/braces", () => {
        expect_eq(isGlob("[a-z", true), false);
        expect_eq(isGlob("[a-z", false), true);
        expect_eq(isGlob("{a,b", true), false);
        expect_eq(isGlob("{a,b", false), true);
        expect_eq(isGlob("foo(bar)", true), false);
        expect_eq(isGlob("foo(bar)", false), true);
    });

    test("5. @cap/is-glob — backslash escapes return false", () => {
        expect_eq(isGlobDefault("\\*.cp"), false);
        expect_eq(isGlobDefault("\\?.cp"), false);
        expect_eq(isGlobDefault("\\[a-z\\]"), false);
        expect_eq(isGlobDefault("\\{a,b\\}"), false);
        expect_eq(isGlobDefault("\\!(foo|bar)"), false);
        expect_eq(hasUnescapedWildcard("\\*\\?"), false);
        // Double backslash escapes the backslash itself, so '*' is unescaped
        expect_eq(isGlobDefault("\\\\*.cp"), true);
    });

    test("6. @cap/is-glob — plain paths & empty string edge cases", () => {
        expect_eq(isGlobDefault(""), false);
        expect_eq(isGlob("", false), false);
        expect_eq(isGlobDefault("src/index.cp"), false);
        expect_eq(isGlobDefault("packages/@cap/is-glob/cap.json"), false);
        expect_eq(isGlobDefault("hello!world.txt"), false);
    });
}
