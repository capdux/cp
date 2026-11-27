// cap/test/testCapGlobParent.cp
// Comprehensive unit tests for @cap/glob-parent

import { test, expect_eq } from "cap:test";
import {
    isGlob,
    globParent,
    splitGlobBase,
    escapeGlob,
} from "../../packages/glob-parent/src/index.cp";

fn main() {
    test("1. @cap/glob-parent — isGlob standard wildcards, char classes & braces", () => {
        expect_eq(isGlob("*.cp"), true);
        expect_eq(isGlob("src/a?.cp"), true);
        expect_eq(isGlob("file[0-9].txt"), true);
        expect_eq(isGlob("a/{b,c}/d"), true);
        expect_eq(isGlob("a/{1..5}.txt"), true);
        expect_eq(isGlob("plain/path/file.cp"), false);
        expect_eq(isGlob(""), false);
    });

    test("2. @cap/glob-parent — isGlob extglobs, escapes & unmatched brackets", () => {
        expect_eq(isGlob("!(foo|bar).js"), true);
        expect_eq(isGlob("+(a|b)"), true);
        expect_eq(isGlob("@(foo|bar)"), true);
        expect_eq(isGlob("*(abc)"), true);
        expect_eq(isGlob("?(abc)"), true);

        expect_eq(isGlob("a\\*b"), false);
        expect_eq(isGlob("a\\[0\\]b"), false);
        expect_eq(isGlob("a\\{b,c\\}"), false);
        expect_eq(isGlob("a[unclosed"), false);
        expect_eq(isGlob("a{unclosed,b"), false);
        expect_eq(isGlob("a{single}b"), false);
        expect_eq(isGlob("!plain.txt"), false);
    });

    test("3. @cap/glob-parent — globParent relative, absolute, nested & negation patterns", () => {
        expect_eq(globParent("path/to/*.js"), "path/to");
        expect_eq(globParent("src/components/**/*.cp"), "src/components");
        expect_eq(globParent("*.js"), ".");
        expect_eq(globParent("/a/b/{c,d}/e"), "/a/b");
        expect_eq(globParent("/*.js"), "/");
        expect_eq(globParent("!src/*.cp"), "src");
        expect_eq(globParent("src/!(test)/*.cp"), "src");
    });

    test("4. @cap/glob-parent — globParent non-glob paths & root edge cases", () => {
        expect_eq(globParent("a/b/c.txt"), "a/b");
        expect_eq(globParent("file.txt"), ".");
        expect_eq(globParent("/file.txt"), "/");
        expect_eq(globParent("/"), "/");
        expect_eq(globParent(""), ".");
    });

    test("5. @cap/glob-parent — splitGlobBase splits base & dynamic glob suffix", () => {
        const s1 = splitGlobBase("src/a/*.cp");
        expect_eq(s1.base, "src/a");
        expect_eq(s1.glob, "*.cp");
        expect_eq(s1.is_glob, true);

        const s2 = splitGlobBase("*.cp");
        expect_eq(s2.base, ".");
        expect_eq(s2.glob, "*.cp");
        expect_eq(s2.is_glob, true);

        const s3 = splitGlobBase("/etc/**/*.conf");
        expect_eq(s3.base, "/etc");
        expect_eq(s3.glob, "**/*.conf");
        expect_eq(s3.is_glob, true);

        const s4 = splitGlobBase("a/b/c.txt");
        expect_eq(s4.base, "a/b");
        expect_eq(s4.glob, "c.txt");
        expect_eq(s4.is_glob, false);
    });

    test("6. @cap/glob-parent — escapeGlob neutralizes all glob metacharacters", () => {
        let out1: [128]u8 = undefined;
        const esc1 = escapeGlob("src/[id]/*(a,b)?!\\x", out1[0..128]);
        expect_eq(esc1, "src/\\[id\\]/\\*\\(a,b\\)\\?\\!\\\\x");
        expect_eq(isGlob(esc1), false);

        let out2: [64]u8 = undefined;
        expect_eq(escapeGlob("plain/file.cp", out2[0..64]), "plain/file.cp");
    });
}
