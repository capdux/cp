// cap/test/testCapPicomatch.cp
// Comprehensive unit tests for @cap/picomatch (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    PicoOptions,
    isMatch,
    isMatchWithOptions,
    isMatchAny,
    scanBase,
} from "../../packages/picomatch/src/index.cp";

fn main() {
    test("1. @cap/picomatch — basic *, ?, multi-segment & Windows backslash normalization", () => {
        expect_eq(isMatch("foo.cp", "*.cp"), true);
        expect_eq(isMatch("foo.ts", "*.cp"), false);
        expect_eq(isMatch("a/foo.cp", "*.cp"), false);
        expect_eq(isMatch("src/foo.cp", "src/*.cp"), true);
        expect_eq(isMatch("src/a/foo.cp", "src/*.cp"), false);

        // Single-char '?' does not cross '/'
        expect_eq(isMatch("file1.cp", "file?.cp"), true);
        expect_eq(isMatch("file12.cp", "file?.cp"), false);
        expect_eq(isMatch("a/b", "a?b"), false);

        // Windows backslash path normalization via @cap/normalize-path
        expect_eq(isMatch("src\\lib\\main.cp", "src/lib/*.cp"), true);
        expect_eq(isMatch("src\\a\\b\\app.cp", "src/**/*.cp"), true);
    });

    test("2. @cap/picomatch — globstar ** matching (**/foo, a/**/b, a/**, noglobstar)", () => {
        // **/foo matches root and nested
        expect_eq(isMatch("foo", "**/foo"), true);
        expect_eq(isMatch("a/foo", "**/foo"), true);
        expect_eq(isMatch("a/b/c/foo", "**/foo"), true);
        expect_eq(isMatch("a/b/bar", "**/foo"), false);

        // a/**/b matches 0, 1, or many intermediate dirs
        expect_eq(isMatch("a/b", "a/**/b"), true);
        expect_eq(isMatch("a/x/b", "a/**/b"), true);
        expect_eq(isMatch("a/x/y/z/b", "a/**/b"), true);
        expect_eq(isMatch("a/x/y/c", "a/**/b"), false);

        // a/** matches anything under a/
        expect_eq(isMatch("a/file.cp", "a/**"), true);
        expect_eq(isMatch("a/x/y/file.cp", "a/**"), true);
        expect_eq(isMatch("b/file.cp", "a/**"), false);

        // noglobstar disables cross-directory **
        let opts = PicoOptions.defaults();
        opts.noglobstar = true;
        expect_eq(isMatchWithOptions("a/x/y/b", "a/**/b", opts), false);
        expect_eq(isMatchWithOptions("a/x/b", "a/**/b", opts), true);
    });

    test("3. @cap/picomatch — character classes [a-z]/[!0-9]/[^a-z] & brace expansion {ts,cp,zig}", () => {
        expect_eq(isMatch("a.cp", "[a-z].cp"), true);
        expect_eq(isMatch("Z.cp", "[a-z].cp"), false);
        expect_eq(isMatch("x.cp", "[!0-9].cp"), true);
        expect_eq(isMatch("5.cp", "[!0-9].cp"), false);
        expect_eq(isMatch("9.cp", "[^a-z].cp"), true);
        expect_eq(isMatch("m.cp", "[^a-z].cp"), false);

        // Brace expansion
        expect_eq(isMatch("src/app.ts", "src/*.{ts,cp,zig}"), true);
        expect_eq(isMatch("src/app.cp", "src/*.{ts,cp,zig}"), true);
        expect_eq(isMatch("src/app.zig", "src/*.{ts,cp,zig}"), true);
        expect_eq(isMatch("src/app.rs", "src/*.{ts,cp,zig}"), false);

        // Nested brace expansion
        expect_eq(isMatch("lib/net/http.hpp", "{src,lib}/**/*.{c,{h,hpp}}"), true);
        expect_eq(isMatch("src/core.c", "{src,lib}/**/*.{c,{h,hpp}}"), true);
        expect_eq(isMatch("test/core.c", "{src,lib}/**/*.{c,{h,hpp}}"), false);
    });

    test("4. @cap/picomatch — dotfile protection, opts.dot, opts.nocase & opts.basename", () => {
        // Default dotfile protection
        expect_eq(isMatch(".env", "*"), false);
        expect_eq(isMatch("src/.env", "src/*"), false);
        expect_eq(isMatch("a/.git/config", "a/**/config"), false);
        expect_eq(isMatch(".env", ".*"), true);
        expect_eq(isMatch("a/.env", "**/.env"), true);

        // opts.dot = true allows matching dotfiles
        let dotOpts = PicoOptions.defaults();
        dotOpts.dot = true;
        expect_eq(isMatchWithOptions(".env", "*", dotOpts), true);
        expect_eq(isMatchWithOptions("a/.git/config", "a/**/config", dotOpts), true);

        // opts.nocase = true
        let ncOpts = PicoOptions.defaults();
        ncOpts.nocase = true;
        expect_eq(isMatchWithOptions("SRC/README.MD", "src/*.md", ncOpts), true);
        expect_eq(isMatch("SRC/README.MD", "src/*.md"), false);

        // opts.basename = true
        let baseOpts = PicoOptions.defaults();
        baseOpts.basename = true;
        expect_eq(isMatchWithOptions("a/b/c/index.cp", "*.cp", baseOpts), true);
        expect_eq(isMatchWithOptions("a/b/c/index.ts", "*.cp", baseOpts), false);
    });

    test("5. @cap/picomatch — leading ! / !! negation & isMatchAny combined filtering", () => {
        expect_eq(isMatch("foo.cp", "!*.ts"), true);
        expect_eq(isMatch("foo.ts", "!*.ts"), false);
        expect_eq(isMatch("foo.ts", "!!*.ts"), true);
        expect_eq(isMatch("foo.cp", "!!*.ts"), false);

        const rules: [3]string = [
            "src/**/*.cp",
            "!src/**/*.test.cp",
            "src/keep.test.cp",
        ];
        const opts = PicoOptions.defaults();
        expect_eq(isMatchAny("src/net/http.cp", rules[0..3], opts), true);
        expect_eq(isMatchAny("src/net/http.test.cp", rules[0..3], opts), false);
        expect_eq(isMatchAny("src/keep.test.cp", rules[0..3], opts), true);
        expect_eq(isMatchAny("lib/other.cp", rules[0..3], opts), false);

        // Negative-only rule list
        const negOnly: [1]string = ["!*.md"];
        expect_eq(isMatchAny("main.cp", negOnly[0..1], opts), true);
        expect_eq(isMatchAny("README.md", negOnly[0..1], opts), false);
    });

    test("6. @cap/picomatch — scanBase static prefix extraction & edge cases", () => {
        let buf: [128]u8 = undefined;

        const n1 = scanBase("src/lib/**/*.cp", buf[0..128]);
        expect_eq(buf[0..n1], "src/lib");

        const n2 = scanBase("src\\lib\\**\\*.cp", buf[0..128]);
        expect_eq(buf[0..n2], "src/lib");

        const n3 = scanBase("packages/@cap/ipaddr/src/*.cp", buf[0..128]);
        expect_eq(buf[0..n3], "packages/@cap/ipaddr/src");

        const n4 = scanBase("src/a/!(foo)/bar", buf[0..128]);
        expect_eq(buf[0..n4], "src/a");

        const n5 = scanBase("!src/lib/**/*.cp", buf[0..128]);
        expect_eq(buf[0..n5], "src/lib");

        const n6 = scanBase("src/static.cp", buf[0..128]);
        expect_eq(buf[0..n6], "src/static.cp");

        // Leading glob or empty pattern -> 0
        expect_eq(scanBase("**/*.cp", buf[0..128]), 0 as usize);
        expect_eq(scanBase("*.cp", buf[0..128]), 0 as usize);
        expect_eq(scanBase("", buf[0..128]), 0 as usize);
        expect_eq(isMatch("foo.cp", ""), false);
    });
}
