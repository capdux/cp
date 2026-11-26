// cap/test/testCapGlobMatch.cp
// Comprehensive unit tests for @cap/glob-match

import { test, expect_eq } from "cap:test";
import {
    isMatch,
    isMatchAny,
    isIgnored,
    hasGlobMagic,
} from "../../packages/@cap/glob-match/src/index.cp";

fn main() {
    test("1. @cap/glob-match — Single-segment * does not cross '/' & ? matching", () => {
        expect_eq(isMatch("*.cp", "main.cp"), true);
        expect_eq(isMatch("*.cp", "src/main.cp"), false);
        expect_eq(isMatch("src/*.cp", "src/main.cp"), true);
        expect_eq(isMatch("src/*.cp", "src/sub/main.cp"), false);
        expect_eq(isMatch("file?.txt", "file1.txt"), true);
        expect_eq(isMatch("file?.txt", "file/.txt"), false);
        expect_eq(isMatch("file?.txt", "file12.txt"), false);
    });

    test("2. @cap/glob-match — Cross-directory ** (src/**/*.cp, **/*.md, a/**/b)", () => {
        expect_eq(isMatch("src/**/*.cp", "src/index.cp"), true);
        expect_eq(isMatch("src/**/*.cp", "src/a/b/c/index.cp"), true);
        expect_eq(isMatch("src/**/*.cp", "lib/index.cp"), false);

        expect_eq(isMatch("**/*.md", "README.md"), true);
        expect_eq(isMatch("**/*.md", "docs/guide/intro.md"), true);
        expect_eq(isMatch("**/*.md", "docs/guide/intro.txt"), false);

        expect_eq(isMatch("a/**/b", "a/b"), true);
        expect_eq(isMatch("a/**/b", "a/x/y/b"), true);
        expect_eq(isMatch("a/**/b", "a/xb"), false);
    });

    test("3. @cap/glob-match — Character classes [a-z], [!0-9], [^0-9] & escapes", () => {
        expect_eq(isMatch("v[0-9].txt", "v3.txt"), true);
        expect_eq(isMatch("v[0-9].txt", "va.txt"), false);
        expect_eq(isMatch("log_[!0-9].txt", "log_a.txt"), true);
        expect_eq(isMatch("log_[!0-9].txt", "log_5.txt"), false);
        expect_eq(isMatch("log_[^a-z].txt", "log_9.txt"), true);
        expect_eq(isMatch("a\\*b", "a*b"), true);
        expect_eq(isMatch("a\\*b", "axxxb"), false);
    });

    test("4. @cap/glob-match — isIgnored with !negation override & # comments", () => {
        let rules: [5]string = undefined;
        rules[0] = "# Build & logs";
        rules[1] = "";
        rules[2] = "*.log";
        rules[3] = "!important.log";
        rules[4] = "dist/";

        expect_eq(isIgnored(rules[0..5], "app.log"), true);
        expect_eq(isIgnored(rules[0..5], "logs/error.log"), true);
        expect_eq(isIgnored(rules[0..5], "important.log"), false);
        expect_eq(isIgnored(rules[0..5], "logs/important.log"), false);
        expect_eq(isIgnored(rules[0..5], "dist/bundle.js"), true);
        expect_eq(isIgnored(rules[0..5], "src/index.cp"), false);

        let pats: [2]string = undefined;
        pats[0] = "*.rs";
        pats[1] = "src/**/*.cp";
        expect_eq(isMatchAny(pats[0..2], "src/a/b.cp"), true);
        expect_eq(isMatchAny(pats[0..2], "src/a/b.go"), false);
    });

    test("5. @cap/glob-match — hasGlobMagic metacharacter detection", () => {
        expect_eq(hasGlobMagic("src/*.cp"), true);
        expect_eq(hasGlobMagic("file?.txt"), true);
        expect_eq(hasGlobMagic("v[0-9]"), true);
        expect_eq(hasGlobMagic("{a,b}"), true);
        expect_eq(hasGlobMagic("src/index.cp"), false);
        expect_eq(hasGlobMagic("literal\\*star"), false);
    });

    test("6. @cap/glob-match — Empty strings & ReDoS-safe pathological long pattern", () => {
        expect_eq(isMatch("", ""), true);
        expect_eq(isMatch("", "a"), false);
        expect_eq(isMatch("*", ""), true);
        expect_eq(isMatch("**", ""), true);

        // Pathological multi-star pattern that causes exponential blowup in naive recursion.
        const pat = "*a*a*a*a*a*a*a*a*a*a*b";
        const txt = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        expect_eq(isMatch(pat, txt), false);
    });
}
