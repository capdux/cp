import { test, expect_eq } from "cap:test";
import {
    ExcludeConfig,
    matchSimpleGlob,
    shouldInstrument,
} from "../../packages/test-exclude/src/index.cp";

fn main() {
    test("1. @cap/test-exclude — matchSimpleGlob handles *, ?, and literal paths", () => {
        expect_eq(matchSimpleGlob("*.cp", "index.cp"), true);
        expect_eq(matchSimpleGlob("*.cp", "src/index.cp"), false);
        expect_eq(matchSimpleGlob("src/a?.cp", "src/a1.cp"), true);
        expect_eq(matchSimpleGlob("src/a?.cp", "src/a12.cp"), false);
        expect_eq(matchSimpleGlob("src/a?.cp", "src/a/.cp"), false);
    });

    test("2. @cap/test-exclude — matchSimpleGlob handles ** globstar and leading ! negation", () => {
        expect_eq(matchSimpleGlob("src/**/*.cp", "src/index.cp"), true);
        expect_eq(matchSimpleGlob("src/**/*.cp", "src/a/b/index.cp"), true);
        expect_eq(matchSimpleGlob("**/test/*.cp", "test/unit.cp"), true);
        expect_eq(matchSimpleGlob("**/test/*.cp", "pkg/test/unit.cp"), true);
        expect_eq(matchSimpleGlob("!**/*.test.cp", "src/app.cp"), true);
        expect_eq(matchSimpleGlob("!**/*.test.cp", "src/app.test.cp"), false);
    });

    test("3. @cap/test-exclude — shouldInstrument auto-excludes node_modules and .git", () => {
        let empty: [1]string = undefined;
        let exts: [2]string = undefined;
        exts[0] = ".cp";
        exts[1] = ".ts";
        const cfg = ExcludeConfig{
            includes: empty[0..0],
            excludes: empty[0..0],
            extensions: exts[0..2],
        };

        expect_eq(shouldInstrument("src/app.cp", cfg), true);
        expect_eq(shouldInstrument("node_modules/pkg/index.ts", cfg), false);
        expect_eq(shouldInstrument("sub/node_modules/a.cp", cfg), false);
        expect_eq(shouldInstrument(".git/hooks/pre.cp", cfg), false);
        expect_eq(shouldInstrument("README.md", cfg), false);
    });

    test("4. @cap/test-exclude — shouldInstrument applies includes and excludes rules", () => {
        let incs: [1]string = undefined;
        incs[0] = "src/**";
        let excs: [2]string = undefined;
        excs[0] = "**/*.test.cp";
        excs[1] = "src/fixtures/**";
        let exts: [1]string = undefined;
        exts[0] = ".cp";

        const cfg = ExcludeConfig{
            includes: incs[0..1],
            excludes: excs[0..2],
            extensions: exts[0..1],
        };

        expect_eq(shouldInstrument("src/core/engine.cp", cfg), true);
        expect_eq(shouldInstrument("./src/core/engine.cp", cfg), true);
        expect_eq(shouldInstrument("src/core/engine.test.cp", cfg), false);
        expect_eq(shouldInstrument("src/fixtures/sample.cp", cfg), false);
        expect_eq(shouldInstrument("scripts/build.cp", cfg), false);
    });

    test("5. @cap/test-exclude — shouldInstrument supports ! re-include inside excludes", () => {
        let incs: [1]string = undefined;
        incs[0] = "src/**";
        let excs: [2]string = undefined;
        excs[0] = "src/gen/**";
        excs[1] = "!src/gen/keep.cp";
        let exts: [1]string = undefined;
        exts[0] = "cp";

        const cfg = ExcludeConfig{
            includes: incs[0..1],
            excludes: excs[0..2],
            extensions: exts[0..1],
        };

        expect_eq(shouldInstrument("src/gen/drop.cp", cfg), false);
        expect_eq(shouldInstrument("src/gen/keep.cp", cfg), true);
    });

    test("6. @cap/test-exclude — shouldInstrument handles empty path and negated includes", () => {
        let incs: [2]string = undefined;
        incs[0] = "lib/**";
        incs[1] = "!lib/internal/**";
        let empty: [1]string = undefined;

        const cfg = ExcludeConfig{
            includes: incs[0..2],
            excludes: empty[0..0],
            extensions: empty[0..0],
        };

        expect_eq(shouldInstrument("", cfg), false);
        expect_eq(shouldInstrument("lib/public/api.cp", cfg), true);
        expect_eq(shouldInstrument("lib/internal/secret.cp", cfg), false);
    });
}
