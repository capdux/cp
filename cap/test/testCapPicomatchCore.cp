import { test, expect_eq } from "cap:test";
import {
    ScanResult,
    PicomatchOptions,
    scan,
    isMatch,
    matchExtglob,
} from "../../packages/@cap/picomatch-core/src/index.cp";

fn eqStr(a: string, b: string): bool {
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
    test("1. @cap/picomatch-core — scan splits base, glob, prefix, and negation", () => {
        let r1: ScanResult = scan("src/net/*.cp");
        expect_eq(eqStr(r1.prefix, ""), true);
        expect_eq(eqStr(r1.input, "src/net/*.cp"), true);
        expect_eq(eqStr(r1.base, "src/net"), true);
        expect_eq(eqStr(r1.glob, "*.cp"), true);
        expect_eq(r1.is_glob, true);
        expect_eq(r1.negated, false);

        let r2: ScanResult = scan("!./packages/**/*.cp");
        expect_eq(eqStr(r2.prefix, "!./"), true);
        expect_eq(eqStr(r2.base, "packages"), true);
        expect_eq(eqStr(r2.glob, "**/*.cp"), true);
        expect_eq(r2.is_glob, true);
        expect_eq(r2.negated, true);

        let r3: ScanResult = scan("./static/path/file.cp");
        expect_eq(eqStr(r3.prefix, "./"), true);
        expect_eq(eqStr(r3.base, "static/path/file.cp"), true);
        expect_eq(eqStr(r3.glob, ""), true);
        expect_eq(r3.is_glob, false);
        expect_eq(r3.negated, false);

        // `!(a|b)` is an extglob, not a negation prefix.
        let r4: ScanResult = scan("!(foo|bar)");
        expect_eq(eqStr(r4.prefix, ""), true);
        expect_eq(eqStr(r4.base, ""), true);
        expect_eq(eqStr(r4.glob, "!(foo|bar)"), true);
        expect_eq(r4.is_glob, true);
        expect_eq(r4.negated, false);
    });

    test("2. @cap/picomatch-core — isMatch basic wildcards and negation", () => {
        let opts: PicomatchOptions = PicomatchOptions.defaults();
        expect_eq(isMatch("src/main.cp", "src/*.cp", opts), true);
        expect_eq(isMatch("src/sub/main.cp", "src/**/*.cp", opts), true);
        expect_eq(isMatch("src/sub/main.rs", "src/**/*.cp", opts), false);
        expect_eq(isMatch("src/main.cp", "!src/*.rs", opts), true);
        expect_eq(isMatch("src/main.rs", "!src/*.rs", opts), false);
        expect_eq(isMatch("foo.cp", "./*.cp", opts), true);
        expect_eq(isMatch("./foo.cp", "./*.cp", opts), true);
    });

    test("3. @cap/picomatch-core — dot option controls hidden files", () => {
        let def_opts: PicomatchOptions = PicomatchOptions.defaults();
        let dot_opts: PicomatchOptions = PicomatchOptions {
            dot: true,
            nocase: false,
            match_base: false,
            bash: false,
        };

        expect_eq(isMatch(".env", "*", def_opts), false);
        expect_eq(isMatch(".env", ".*", def_opts), true);
        expect_eq(isMatch(".env", "*", dot_opts), true);
        expect_eq(isMatch("src/.hidden/a.cp", "src/**/*.cp", def_opts), false);
        expect_eq(isMatch("src/.hidden/a.cp", "src/**/*.cp", dot_opts), true);
        expect_eq(isMatch("src/.hidden/a.cp", "src/.hidden/*.cp", def_opts), true);
    });

    test("4. @cap/picomatch-core — match_base option matches basename only", () => {
        let def_opts: PicomatchOptions = PicomatchOptions.defaults();
        let base_opts: PicomatchOptions = PicomatchOptions {
            dot: false,
            nocase: false,
            match_base: true,
            bash: false,
        };

        expect_eq(isMatch("a/b/c/index.cp", "*.cp", def_opts), false);
        expect_eq(isMatch("a/b/c/index.cp", "*.cp", base_opts), true);
        expect_eq(isMatch("a/b/c/index.rs", "*.cp", base_opts), false);
        // When pattern contains `/`, match_base does not strip directory.
        expect_eq(isMatch("a/b/c/index.cp", "c/*.cp", base_opts), false);
    });

    test("5. @cap/picomatch-core — nocase option performs case-insensitive matching", () => {
        let def_opts: PicomatchOptions = PicomatchOptions.defaults();
        let nc_opts: PicomatchOptions = PicomatchOptions {
            dot: false,
            nocase: true,
            match_base: false,
            bash: false,
        };

        expect_eq(isMatch("README.MD", "*.md", def_opts), false);
        expect_eq(isMatch("README.MD", "*.md", nc_opts), true);
        expect_eq(isMatch("Src/Net/Http.CP", "src/**/*.@(cp|zig)", nc_opts), true);
        expect_eq(isMatch("Src/Net/Http.PY", "src/**/*.@(cp|zig)", nc_opts), false);
    });

    test("6. @cap/picomatch-core — matchExtglob supports @(a|b), !(a|b), ?(a|b), +(a|b), *(a|b)", () => {
        expect_eq(matchExtglob("foo", "@(foo|bar)"), true);
        expect_eq(matchExtglob("bar", "@(foo|bar)"), true);
        expect_eq(matchExtglob("baz", "@(foo|bar)"), false);

        expect_eq(matchExtglob("baz", "!(foo|bar)"), true);
        expect_eq(matchExtglob("foo", "!(foo|bar)"), false);
        expect_eq(matchExtglob("src/main.cp", "src/!(test|spec).cp"), true);
        expect_eq(matchExtglob("src/test.cp", "src/!(test|spec).cp"), false);

        expect_eq(matchExtglob("file.cp", "file?(.min).cp"), true);
        expect_eq(matchExtglob("file.min.cp", "file?(.min).cp"), true);
        expect_eq(matchExtglob("file.max.cp", "file?(.min).cp"), false);

        expect_eq(matchExtglob("abab", "+(ab|cd)"), true);
        expect_eq(matchExtglob("", "+(ab|cd)"), false);
        expect_eq(matchExtglob("", "*(ab|cd)"), true);
        expect_eq(matchExtglob("cdab", "*(ab|cd)"), true);
    });
}
