import { test, expect_eq } from "cap:test";
import { IgnoreMatcher } from "../../packages/@cap/ignore/src/index.cp";

fn main() {
    test("1. @cap/ignore — Basic wildcards, comments, and blank lines", () => {
        let ig = IgnoreMatcher.new();
        let added = ig.addLines("# Build outputs\n\n*.log\ndist/\n");
        expect_eq(added, 2);
        expect_eq(ig.ignores("debug.log"), true);
        expect_eq(ig.ignores("src/app.log"), true);
        expect_eq(ig.ignores("src/main.cp"), false);
    });

    test("2. @cap/ignore — Directory-only trailing slash rules", () => {
        let ig = IgnoreMatcher.new();
        * = ig.addRule("build/");
        expect_eq(ig.ignores("build"), false);
        expect_eq(ig.ignores("build/"), true);
        expect_eq(ig.ignores("build/output.bin"), true);
        expect_eq(ig.ignores("pkg/build/a.o"), true);
    });

    test("3. @cap/ignore — Leading slash anchored vs unanchored rules", () => {
        let ig = IgnoreMatcher.new();
        * = ig.addRule("/root.txt");
        * = ig.addRule("anywhere.tmp");
        expect_eq(ig.ignores("root.txt"), true);
        expect_eq(ig.ignores("sub/root.txt"), false);
        expect_eq(ig.ignores("anywhere.tmp"), true);
        expect_eq(ig.ignores("a/b/anywhere.tmp"), true);
    });

    test("4. @cap/ignore — Negation (!) re-inclusion in rule order", () => {
        let ig = IgnoreMatcher.new();
        * = ig.addLines("*.log\n!important.log\n");
        expect_eq(ig.ignores("error.log"), true);
        expect_eq(ig.ignores("important.log"), false);
        expect_eq(ig.ignores("sub/important.log"), false);
    });

    test("5. @cap/ignore — Batch filter and Windows path normalization", () => {
        let ig = IgnoreMatcher.new();
        * = ig.addLines("node_modules/\n*.o\n");
        let paths: [4]string = ["src\\main.cp", "node_modules\\pkg\\a.js", "obj\\foo.o", "README.md"];
        let kept: [4]usize = undefined;
        let n = ig.filter(paths[0..4], kept[0..4]);
        expect_eq(n, 2);
        expect_eq(kept[0], 0);
        expect_eq(kept[1], 3);
    });

    test("6. @cap/ignore — Case-insensitive mode and empty safety", () => {
        let ig = IgnoreMatcher.newCaseInsensitive();
        * = ig.addRule("*.BAK");
        expect_eq(ig.ignores("data.bak"), true);
        expect_eq(ig.ignores("DATA.BAK"), true);
        expect_eq(ig.ignores(""), false);
    });
}
