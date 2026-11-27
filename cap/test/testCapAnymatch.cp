import { test, expect_eq } from "cap:test";
import {
    matchPattern,
    anymatch,
    anymatchIndex,
    countGlobPatterns,
} from "../../packages/anymatch/src/index.cp";

fn main() {
    test("1. @cap/anymatch — Exact path, directory prefix & Windows backslash matching", () => {
        expect_eq(matchPattern("src/index.cp", "src/index.cp"), true);
        expect_eq(matchPattern("src/index.cp", "src\\index.cp"), true);
        expect_eq(matchPattern("src\\lib\\", "src/lib/util.cp"), true);
        expect_eq(matchPattern("src/lib", "src/library.cp"), false);
        expect_eq(matchPattern("src/index.cp", "src/other.cp"), false);
    });

    test("2. @cap/anymatch — Glob '*' '**' & '?' wildcard matching", () => {
        expect_eq(matchPattern("src/*.cp", "src/main.cp"), true);
        expect_eq(matchPattern("src/*.cp", "src/sub/main.cp"), false);
        expect_eq(matchPattern("src/**/*.cp", "src/main.cp"), true);
        expect_eq(matchPattern("src/**/*.cp", "src/a/b/main.cp"), true);
        expect_eq(matchPattern("src/**/*.cp", "src\\a\\b\\main.cp"), true);
        expect_eq(matchPattern("test/?.cp", "test/a.cp"), true);
        expect_eq(matchPattern("test/?.cp", "test/ab.cp"), false);
    });

    test("3. @cap/anymatch — Negation '!pattern' exclusion rules", () => {
        const rules: [3]string = [
            "src/**/*.cp",
            "!src/vendor/**",
            "!src/secret.cp",
        ];
        expect_eq(anymatch(&rules, "src/app.cp"), true);
        expect_eq(anymatch(&rules, "src/net/http.cp"), true);
        expect_eq(anymatch(&rules, "src/vendor/lib.cp"), false);
        expect_eq(anymatch(&rules, "src\\vendor\\sub\\x.cp"), false);
        expect_eq(anymatch(&rules, "src/secret.cp"), false);
    });

    test("4. @cap/anymatch — anymatchIndex returns exact index or -1", () => {
        const rules: [4]string = [
            "README.md",
            "docs/**",
            "src/*.cp",
            "!src/ignore.cp",
        ];
        expect_eq(anymatchIndex(&rules, "README.md"), 0);
        expect_eq(anymatchIndex(&rules, "docs/guide/intro.md"), 1);
        expect_eq(anymatchIndex(&rules, "src/index.cp"), 2);
        expect_eq(anymatchIndex(&rules, "src/ignore.cp"), -1);
        expect_eq(anymatchIndex(&rules, "other/file.txt"), -1);
    });

    test("5. @cap/anymatch — countGlobPatterns delegates to @cap/is-glob", () => {
        const rules: [5]string = [
            "README.md",
            "src/**/*.cp",
            "packages/@cap",
            "!*.tmp",
            "test/file?.cp",
        ];
        expect_eq(countGlobPatterns(&rules), 3);

        const literals: [2]string = ["a/b/c.txt", "Makefile"];
        expect_eq(countGlobPatterns(&literals), 0);
    });

    test("6. @cap/anymatch — Empty rules & empty path boundaries", () => {
        const one: [1]string = ["src/*.cp"];
        const empty_rules: []const string = one[0..0];

        expect_eq(matchPattern("", "src/a.cp"), false);
        expect_eq(matchPattern("src/*.cp", ""), false);
        expect_eq(matchPattern("", ""), false);
        expect_eq(anymatch(empty_rules, "src/a.cp"), false);
        expect_eq(anymatchIndex(empty_rules, "src/a.cp"), -1);
        expect_eq(anymatch(&one, ""), false);
        expect_eq(countGlobPatterns(empty_rules), 0);
    });
}
