// cap/test/testCapMicromatch.cp
// Comprehensive unit tests for @cap/micromatch (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    PicoOptions,
    mmIsMatch,
    mmAll,
    mmAny,
    mmSome,
    mmEvery,
    mmFilter,
    mmNot,
    mmContains,
} from "../../packages/micromatch/src/index.cp";

fn main() {
    test("1. @cap/micromatch — mmFilter & mmNot batch filtering with ! negation", () => {
        const paths: [5]string = [
            "src/index.cp",
            "src/net/http.cp",
            "src/net/http.test.cp",
            "docs/guide.md",
            "src/keep.test.cp",
        ];
        const rules: [3]string = [
            "src/**/*.cp",
            "!**/*.test.cp",
            "src/keep.test.cp",
        ];
        const opts = PicoOptions.defaults();

        let matched: [8]usize = undefined;
        const mCount = mmFilter(paths[0..5], rules[0..3], opts, matched[0..8]);
        expect_eq(mCount, 3 as usize);
        expect_eq(matched[0], 0 as usize);
        expect_eq(matched[1], 1 as usize);
        expect_eq(matched[2], 4 as usize);

        let rejected: [8]usize = undefined;
        const rCount = mmNot(paths[0..5], rules[0..3], opts, rejected[0..8]);
        expect_eq(rCount, 2 as usize);
        expect_eq(rejected[0], 2 as usize);
        expect_eq(rejected[1], 3 as usize);
    });

    test("2. @cap/micromatch — mmAll & mmAny logical semantics", () => {
        const opts = PicoOptions.defaults();
        const allRules: [3]string = ["src/**/*.cp", "**/http.*", "!*.test.cp"];
        expect_eq(mmAll("src/net/http.cp", allRules[0..3], opts), true);
        expect_eq(mmAll("src/net/tcp.cp", allRules[0..3], opts), false);
        expect_eq(mmAll("http.test.cp", allRules[0..3], opts), false);

        const anyRules: [2]string = ["*.ts", "*.cp"];
        expect_eq(mmAny("app.cp", anyRules[0..2], opts), true);
        expect_eq(mmAny("app.ts", anyRules[0..2], opts), true);
        expect_eq(mmAny("app.rs", anyRules[0..2], opts), false);

        // Negation exclusion in mmAny
        const exclRules: [2]string = ["src/**/*.cp", "!src/internal/**"];
        expect_eq(mmAny("src/public/api.cp", exclRules[0..2], opts), true);
        expect_eq(mmAny("src/internal/secret.cp", exclRules[0..2], opts), false);
    });

    test("3. @cap/micromatch — mmSome & mmEvery collection predicates", () => {
        const opts = PicoOptions.defaults();
        const cpFiles: [3]string = ["src/a.cp", "src/b.cp", "src/c.cp"];
        const mixedFiles: [3]string = ["src/a.cp", "README.md", "src/c.cp"];
        const pat: [1]string = ["src/*.cp"];

        expect_eq(mmEvery(cpFiles[0..3], pat[0..1], opts), true);
        expect_eq(mmEvery(mixedFiles[0..3], pat[0..1], opts), false);
        expect_eq(mmSome(mixedFiles[0..3], pat[0..1], opts), true);

        const noMatchPat: [1]string = ["*.zig"];
        expect_eq(mmSome(mixedFiles[0..3], noMatchPat[0..1], opts), false);
    });

    test("4. @cap/micromatch — @cap/braces numeric & alpha range expansion (file-{1..3}.cp)", () => {
        const opts = PicoOptions.defaults();
        expect_eq(mmIsMatch("file-1.cp", "file-{1..3}.cp", opts), true);
        expect_eq(mmIsMatch("file-2.cp", "file-{1..3}.cp", opts), true);
        expect_eq(mmIsMatch("file-3.cp", "file-{1..3}.cp", opts), true);
        expect_eq(mmIsMatch("file-4.cp", "file-{1..3}.cp", opts), false);

        // Negated brace range
        expect_eq(mmIsMatch("file-2.cp", "!file-{1..3}.cp", opts), false);
        expect_eq(mmIsMatch("file-9.cp", "!file-{1..3}.cp", opts), true);

        // Alphabetic range + filter
        const shards: [4]string = ["shard-a.db", "shard-b.db", "shard-c.db", "shard-z.db"];
        const rangeRule: [1]string = ["shard-{a..c}.db"];
        let out: [4]usize = undefined;
        const n = mmFilter(shards[0..4], rangeRule[0..1], opts, out[0..4]);
        expect_eq(n, 3 as usize);
        expect_eq(out[0], 0 as usize);
        expect_eq(out[1], 1 as usize);
        expect_eq(out[2], 2 as usize);
    });

    test("5. @cap/micromatch — mmContains sub-path & fragment matching", () => {
        const opts = PicoOptions.defaults();
        expect_eq(mmContains("src/lib/router.cp", "lib", opts), true);
        expect_eq(mmContains("src/lib/router.cp", "lib/*.cp", opts), true);
        expect_eq(mmContains("src/lib/router.cp", "src/lib", opts), true);
        expect_eq(mmContains("src/lib/router.cp", "router", opts), true);
        expect_eq(mmContains("src/lib/router.cp", "missing", opts), false);
        expect_eq(mmContains("src/lib/file-2.cp", "file-{1..3}", opts), true);
        expect_eq(mmContains("src/lib/router.cp", "!missing", opts), true);
        expect_eq(mmContains("src/lib/router.cp", "!lib", opts), false);
    });

    test("6. @cap/micromatch — nocase/dot/basename options & empty list safety", () => {
        let ncOpts = PicoOptions.defaults();
        ncOpts.nocase = true;
        expect_eq(mmIsMatch("SRC/FILE-2.CP", "src/file-{1..3}.cp", ncOpts), true);

        let dotOpts = PicoOptions.defaults();
        expect_eq(mmIsMatch(".github/ci.yml", "**/*.yml", dotOpts), false);
        dotOpts.dot = true;
        expect_eq(mmIsMatch(".github/ci.yml", "**/*.yml", dotOpts), true);

        let baseOpts = PicoOptions.defaults();
        baseOpts.basename = true;
        expect_eq(mmIsMatch("a/b/c/mod-2.cp", "mod-{1..3}.cp", baseOpts), true);

        // Empty lists & empty patterns never panic
        const opts = PicoOptions.defaults();
        const onePath: [1]string = ["a.cp"];
        const onePat: [1]string = ["*.cp"];
        let out: [4]usize = undefined;

        expect_eq(mmIsMatch("a.cp", "", opts), false);
        expect_eq(mmAll("a.cp", onePat[0..0], opts), false);
        expect_eq(mmAny("a.cp", onePat[0..0], opts), false);
        expect_eq(mmSome(onePath[0..0], onePat[0..1], opts), false);
        expect_eq(mmEvery(onePath[0..0], onePat[0..1], opts), false);
        expect_eq(mmFilter(onePath[0..0], onePat[0..1], opts, out[0..4]), 0 as usize);
        expect_eq(mmFilter(onePath[0..1], onePat[0..0], opts, out[0..4]), 0 as usize);
        expect_eq(mmNot(onePath[0..1], onePat[0..0], opts, out[0..4]), 1 as usize);
        expect_eq(mmContains("", "a", opts), false);
        expect_eq(mmContains("a", "", opts), false);
    });
}
